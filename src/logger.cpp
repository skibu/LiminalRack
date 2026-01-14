#include <mutex>

#include <common.hpp>
#include <asset.hpp>
#include <system.hpp>
#include <settings.hpp>
#include <string>
#include <sched.h>
#include <thread>

namespace rack {
namespace logger {

// The default logging level
static Level systemLogLevel = INFO_LEVEL;

std::string logPath;
static FILE* outputFile = NULL;
static std::mutex mutex;
static bool truncated = false;
const static long maxSize = 1000 * 1000 * 1000; // 1 GB

static bool fileEndsWith(FILE* file, std::string str) {
    // Seek to last `len` characters
    size_t len = str.size();
    std::fseek(file, -long(len), SEEK_END);
    std::vector<char> actual(len);
    if (std::fread(actual.data(), 1, len, file) != len) return false;
    return std::string(actual.data(), len) == str;
}

static bool isTruncated() {
	if (logPath.empty())
		return false;

	// Open existing log file
	FILE* file = std::fopen(logPath.c_str(), "r");
	if (!file)
		return false;
	DEFER({std::fclose(file);});

	if (fileEndsWith(file, "END"))
		return false;
	// legacy <=v1
	if (fileEndsWith(file, "Destroying logger\n"))
		return false;
	return true;
}


bool init() {
	if (outputFile)
		return true;

	std::lock_guard<std::mutex> lock(mutex);
	truncated = false;

	// Don't open a file in development mode.
	if (logPath.empty()) {
		outputFile = stderr;
	}
	else {
		truncated = isTruncated();

		outputFile = std::fopen(logPath.c_str(), "w");
		if (!outputFile) {
			std::fprintf(stderr, "Could not open log at %s\n", logPath.c_str());
			return false;
		}
	}

	return true;
}

void destroy() {
	std::lock_guard<std::mutex> lock(mutex);
	if (outputFile && outputFile != stderr) {
		// Print end token so we know if the logger exited cleanly.
		std::fprintf(outputFile, "END");
		std::fclose(outputFile);
	}
	outputFile = NULL;
}

static const char* const levelLabels[] = {
    "TRACE",
	"DEBUG",
	"INFO",
	"WARN",
    "ERROR",
	"FATAL",
};

void setLogLevel(Level level) {
    systemLogLevel = level;
}

Level getLogLevel() {
    return systemLogLevel;
}

void logLogLevel() {
    INFO("Log level=%s", levelLabels[systemLogLevel]);
}

static const int levelColors[] = {
	35,
	34,
	33,
	31,
};

static void logVa(Level level, const char* filename, int line, const char* func,
                  const char* format, va_list args) {
    if (!outputFile) return;

    // Record logging time before calling OS functions
    double nowTime = system::getTime();

    // Check if log size is full
    if (outputFile != stderr) {
        long pos = std::ftell(outputFile);
        if (pos >= maxSize) return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // If stderr then add color codes
    if (outputFile == stderr)
        std::fprintf(outputFile, "\x1B[%dm", levelColors[level]);

    // Determine core ID to output
    std::string core_str = "";
#ifdef __linux__
    // Only output core ID for more verbose levels
    if (level < INFO_LEVEL) {
        int core_id = sched_getcpu();
        core_str = "Core " + std::to_string(core_id) + " ";
    }
#endif

    // Determine thread name to output
    std::string thread_str = "";
    // Could only output core thread name for more verbose levels but
    // for now always output it
    if (level <= FATAL_LEVEL) {
        auto thread_id = std::this_thread::get_id();
        std::ostringstream oss;
        oss << thread_id;
        thread_str = "Thr:" + oss.str() + " ";
    }

    // Outline context info
    std::fprintf(outputFile, "[%.03f %s %s%s%s:%d %s] ", 
        nowTime, levelLabels[level], core_str.c_str(), thread_str.c_str(),
                filename, line, func);

    // Reset color
    if (outputFile == stderr) std::fprintf(outputFile, "\x1B[0m");

    // Print the actual log message and a newline
    std::vfprintf(outputFile, format, args);
    std::fprintf(outputFile, "\n");

    // If WARN or higher level then also output stack trace so that can
    // understand context of the warning/error
    if (level >= WARN_LEVEL) {
        std::string stackTrace = system::getStackTrace();
        std::fprintf(outputFile, "Stack trace:\n%s", stackTrace.c_str());
    }

    // Note: This adds around 10us, but it's important for logging to finish
    // writing the file, and logging is not used in performance critical
    // code.
    std::fflush(outputFile);
}

void log(Level level, const char* filename, int line, const char* func,
         const char* format, ...) {
    // If log level for the logging statement is below the level set for the
    // system then don't log
    if (level < systemLogLevel) return;

    va_list args;
    va_start(args, format);
    logVa(level, filename, line, (std::string(func) + "()").c_str(), format,
            args);
    va_end(args);
}

bool wasTruncated() {
	return truncated;
}


} // namespace logger
} // namespace rack
