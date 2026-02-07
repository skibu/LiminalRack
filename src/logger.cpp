#include <mutex>

#include <common.hpp>
#include <asset.hpp>
#include <system.hpp>
#include <settings.hpp>
#include <string>
#include <sched.h>
#include <thread>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

namespace rack {
namespace logger {

// The default logging level
static Level systemLogLevel = INFO_LEVEL;

std::string logPath;
static FILE* outputFile = NULL;
static std::mutex mutex;
static bool truncated = false;
const static long maxSize = 1000 * 1000 * 1000; // 1 GB
const static bool enableColors = true;

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

void archiveOldLogIfNeeded() {
    // If no log file, nothing to do
    if (!system::exists(logPath)) return;

    // If not in debug mode, nothing to do
    if (getLogLevel() > DEBUG_LEVEL) return;

    // Can't handle windows for now
    if (APP_OS == "win") return;

    // Create log archive directory (if needed)
    const std::string oldLogsDir = system::getDirectory(logPath) + "/oldLogs";
    system::createDirectory(oldLogsDir);

    // Archive old log file.
    // Determine new log filename, appending create timestamp of file
    const std::string logFilename = system::getFilename(logPath);
    std::string getCreateDateTimeCommand;
    if (APP_OS == "mac") {
        // The -f %SB outputs create date in a custom format specified by -t
        getCreateDateTimeCommand =
            "stat -t \"Date_%F_Time_%H-%M-%S\" -f \"%SB\" \"" + logPath + "\"";
    } else {
        // Assume Linux. Use %w to get birth time (creation time)
        getCreateDateTimeCommand =
            "echo \"Date_\""  // Prefix with "Date_"
            "`stat -c %w \"" +
            logPath +
            "\" | "
            "sed 's/[:]/-/g' | "       // Replace : with -
            "sed 's/[ ]/_Time_/g' | "  // Replace space with _Time_
            "cut -c1-24`";             // Trim to seconds
    }
    const std::string createTime = system::executeCommand(
        getCreateDateTimeCommand);  // Trim to milliseconds
    std::string archiveLogFilename =
        oldLogsDir + "/" + logFilename + "_" + createTime;

    // Actually move the old log file
    system::rename(logPath, archiveLogFilename);
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

        // First archive old log if needed
        archiveOldLogIfNeeded();

        // Open log file for writing
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
    "TRCE", // shortened to be same as INFO for better alignment
	"DBUG", // shortened to be same as INFO for better alignment
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

const char* CYAN = "\x1B[36m";
const char* YELLOW = "\x1B[33m";
const char* RED = "\x1B[31m";
const char* MAGENTA = "\x1B[35m";
const char* WHITE = "\x1B[37m";
const char* GREEN = "\x1B[32m";
const char* GRAY = "\x1B[90m";
const char* BLUE = "\x1B[34m";
const char* RESET_COLOR = "\x1B[0m";

static const char* levelColors[] = {
	BLUE,    // trace - blue
	MAGENTA, // debug - magenta
	WHITE,   // info - white
	YELLOW,  // warn - yellow
	RED,     // error - red
	RED,     // fatal - red
};

static const char* bracketColor() { return enableColors ? GRAY : ""; }
static const char* timeColor() { return enableColors ? CYAN : ""; }
static const char* levelColor(Level level) { return enableColors ? levelColors[level] : ""; }
static const char* threadColor() { return enableColors ? CYAN : ""; }
static const char* fileColor() { return enableColors ? MAGENTA : ""; }
static const char* resetColor() { return enableColors ? RESET_COLOR : ""; }

// This is where the stack trace formatter is configured. The
// stack trace is logged for WARN or higher level logs.
// See https://github.com/jeremy-rifkin/cpptrace?tab=readme-ov-file#formatting
const auto loggingStackTraceFormatter =
    cpptrace::formatter{}
        .paths(cpptrace::formatter::path_mode::basename) // Only show filename
        .symbols(cpptrace::formatter::symbol_mode::pretty)  // Function names
        .addresses(cpptrace::formatter::address_mode::none) // Ugly so eliminate
        .colors(cpptrace::formatter::color_mode::always) // Always use colors
        .snippets(true) // Show source code snippets for each frame
        .snippet_context(2) // Show 2 lines above and below the line in the frame
        .header("Stack trace:");  // Add a header before stack trace

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

    // Determine core ID to output
    std::string coreStr = "";
#ifdef __linux__
    // Only output core ID for more verbose levels
    if (level < INFO_LEVEL) {
        int coreId = sched_getcpu();
        coreStr = "Core " + std::to_string(coreId) + " ";
    }
#endif

    // Determine thread name to output
    std::string threadStr = "";
    // Could only output core thread name for more verbose levels but
    // for now always output it
    if (level <= FATAL_LEVEL) {
        auto threadId = std::this_thread::get_id();
        std::ostringstream oss;
        oss << threadId;
        threadStr = "Thr:" + oss.str() + " ";
    }

    // Outline context info
    std::fprintf(outputFile, "%s%7.03f %s%s %s%s%s%s%s:%d %s%s-%s",
        timeColor(), nowTime,                              // time
        levelColor(level), levelLabels[level],             // level
        threadColor(), threadStr.c_str(), coreStr.c_str(), // thread/core
        fileColor(), filename, line, func,                 // file info
        bracketColor(), resetColor());                     // right bracket   

    // Print the actual log message and a newline
    std::vfprintf(outputFile, format, args);
    std::fprintf(outputFile, "\n");

    // If WARN or higher level then also output stack trace so that can
    // understand context of the warning/error
    if (level >= WARN_LEVEL) {
        // Skip extra level on Linux due to extra stack frame from
        // __logging_hook
        int levelsSkip =
            (APP_OS == "lin") ? 2 : 1;
        auto stackTraceStr = loggingStackTraceFormatter.format(
            cpptrace::generate_trace(levelsSkip));
        std::fprintf(outputFile, "%s\n-----\n", stackTraceStr.c_str());
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
