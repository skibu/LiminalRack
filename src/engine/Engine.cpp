#include <algorithm>
#include <set>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <tuple>
#include <pthread.h>

#include <engine/Engine.hpp>
#include <settings.hpp>
#include <system.hpp>
#include <context.hpp>
#include <patch.hpp>
#include <plugin.hpp>
#include <mutex.hpp>
#include <simd/common.hpp>


namespace rack {
namespace engine {


inline void cpuPause() {
#if defined ARCH_X64
	_mm_pause();
#elif defined ARCH_ARM64
    // Originally used __yield() from arm_acle.h, but appears that gcc on RasPi
    // does not support it. Therefore, using inline asm instead as described in
    // https://stackoverflow.com/questions/70069855/is-there-a-yield-intrinsic-on-arm
	//__yield();
    asm volatile("yield");
#endif
}


/** Multiple-phase barrier based on C++ mutexes, as a reference.
*/
struct Barrier {
	size_t threads = 0;
	size_t count = 0;
	size_t phase = 0;

	std::mutex mutex;
	std::condition_variable cv;

	void setThreads(size_t threads) {
		this->threads = threads;
	}

	void wait() {
		std::unique_lock<std::mutex> lock(mutex);
		size_t currentPhase = phase;

		// Check if we're the last thread.
		if (++count >= threads) {
			// Advance phase and reset count
			count = 0;
			phase++;
			// Notify all other threads
			cv.notify_all();
			return;
		}

		// Unlock and wait on phase to change
		cv.wait(lock, [&] {
			return phase != currentPhase;
		});
	}
};


/** Multiple-phase barrier based on spin-locking.
*/
struct SpinBarrier {
	size_t threads = 0;
	std::atomic<size_t> count{0};
	std::atomic<size_t> phase{0};

	void setThreads(size_t threads) {
		this->threads = threads;
	}

	void wait() {
		size_t currentPhase = phase.load(std::memory_order_acquire);

		if (count.fetch_add(1, std::memory_order_acq_rel) + 1 >= threads) {
			// Reset count
			count.store(0, std::memory_order_release);
			// Advance phase, which notifies all other threads, which are all spinning
			phase.fetch_add(1, std::memory_order_release);
			return;
		}

		// Spin until the phase is changed by the last thread
		while (true) {
			if (phase.load(std::memory_order_acquire) != currentPhase)
				return;
			// std::this_thread::yield();
			cpuPause();
		}
	}
};


/** Barrier that spin-locks until yield() is called, and then all threads switch to a mutex.
yield() should be called if it is likely that all threads will block for a while and continuing to spin-lock is unnecessary.
Saves CPU power after yield is called.
*/
struct HybridBarrier {
	size_t threads = 0;
	std::atomic<size_t> count{0};
	std::atomic<size_t> phase{0};
	std::atomic<bool> yielded{false};

	std::mutex mutex;
	std::condition_variable cv;

	void setThreads(size_t threads) {
		this->threads = threads;
	}

	void yield() {
		yielded.store(true, std::memory_order_release);
	}

	void wait() {
		size_t currentPhase = phase.load(std::memory_order_acquire);

		// Check if we're the last thread
		if (count.fetch_add(1, std::memory_order_acq_rel) + 1 >= threads) {
			// Reset count
			count.store(0, std::memory_order_release);

			// If yielded, advance phase and notify all other threads
			if (yielded.load(std::memory_order_acquire)) {
				std::unique_lock<std::mutex> lock(mutex);
				yielded.store(false, std::memory_order_release);
				phase.fetch_add(1, std::memory_order_release);
				cv.notify_all();
				return;
			}

			// Advance phase, which notifies all other threads, which are all spinning
			phase.fetch_add(1, std::memory_order_release);
			return;
		}

		// Spin until the phase is changed by the last thread, or yield() is called
		while (true) {
			if (phase.load(std::memory_order_acquire) != currentPhase)
				return;
			if (yielded.load(std::memory_order_acquire))
				break;
			// std::this_thread::yield();
			cpuPause();
		}


		// yield() was called, so use cv to wait on phase to be changed by the last thread
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [&] {
			return phase.load(std::memory_order_acquire) != currentPhase;
		});
	}
};


struct Engine::Internal {
	std::vector<Module*> modules;
	/** Sorted by (inputModule, inputId) tuple */
	std::vector<Cable*> cables;
	std::set<ParamHandle*> paramHandles;
	Module* masterModule = NULL;

	// moduleId
	std::map<int64_t, Module*> modulesCache;
	// cableId
	std::map<int64_t, Cable*> cablesCache;
	// (moduleId, paramId)
	std::map<std::tuple<int64_t, int>, ParamHandle*> paramHandlesCache;

	float sampleRate = 0.f;
	float sampleTime = 0.f;
	int64_t frame = 0;
	int64_t block = 0;
	int64_t blockFrame = 0;
	double blockTime = 0.0;
	int blockFrames = 0;

	// Meter
	int meterCount = 0;
	double meterTotal = 0.0;
	double meterMax = 0.0;
	double meterLastTime = -INFINITY;
	double meterLastAverage = 0.0;
	double meterLastMax = 0.0;

	// Parameter smoothing
	Module* smoothModule = NULL;
	int smoothParamId = 0;
	float smoothValue = 0.f;

	/** Mutex that guards the Engine state, such as settings, Modules, and Cables.
	Writers lock when mutating the engine's state or stepping the block.
	Readers lock when using the engine's state.
	*/
	SharedMutex mutex;

	/** Mutex that guards stepBlock() so it's not called simultaneously.
	*/
	std::mutex blockMutex;

	int threadCount = 0;
	std::vector<EngineWorker> workers;
	rack::engine::SpinBarrier engineBarrier;
	rack::engine::HybridBarrier workerBarrier;
	std::atomic<int> workerModuleIndex;

	// For worker threads
	Context* context;

	bool fallbackRunning = false;
	std::thread fallbackThread;
	std::mutex fallbackMutex;
	std::condition_variable fallbackCv;

    // Internal methods
    float getParamSmoothValue(Module* module, int paramId) const;
    void setParamSmoothValue(Module* module, int paramId, float value);
    bool hasCable(Cable* cable);

    /** Returns a vector of cable IDs in the rack. Share-locks. */
    size_t getCableIds(int64_t* cableIds, size_t len);

    /** Returns a vector of cable IDs in the rack. Share-locks. */
    std::vector<int64_t> getCableIds();
};


float Engine::Internal::getParamSmoothValue(Module* module, int paramId) const {
	if (smoothModule == module && smoothParamId == paramId)
		return smoothValue;
	return module->params[paramId].getValue();
}


void Engine::Internal::setParamSmoothValue(Module* module, int paramId,
                                           float value) {
    // If another param is being smoothed, jump value
    if (smoothModule && !(smoothModule == module && smoothParamId == paramId)) {
        smoothModule->params[smoothParamId].setValue(smoothValue);
    }
    smoothParamId = paramId;
    smoothValue = value;
    // Set this last so the above values are valid as soon as it is set
    smoothModule = module;
}

bool Engine::Internal::hasCable(Cable* cable) {
	SharedLock<SharedMutex> lock(mutex);
	// TODO Performance could be improved by searching cablesCache, but more testing would be needed to make sure it's always valid.
	auto it = std::find(cables.begin(), cables.end(), cable);
	return it != cables.end();
}


size_t Engine::Internal::getCableIds(int64_t* cableIds, size_t len) {
	SharedLock<SharedMutex> lock(mutex);
	size_t i = 0;
	for (Cable* c : cables) {
		if (i >= len)
			break;
		cableIds[i] = c->id;
		i++;
	}
	return i;
}


std::vector<int64_t> Engine::Internal::getCableIds() {
	SharedLock<SharedMutex> lock(mutex);
	std::vector<int64_t> cableIds;
	cableIds.reserve(cables.size());
	for (Cable* c : cables) {
		cableIds.push_back(c->id);
	}
	return cableIds;
}


static void Engine_updateExpander_NoLock(Engine* that, Module* module,
                                         uint8_t side) {
    Module::Expander& expander = module->getExpander(side);

    if (expander.moduleId >= 0) {
        // Check if moduleId has changed from current module
        if (!expander.module || expander.module->id != expander.moduleId) {
            Module* expanderModule = that->getModule_NoLock(expander.moduleId);
            module->setExpanderModule(expanderModule, side);
        }
    } else {
        // Check if moduleId has unset module
        if (expander.module) {
            module->setExpanderModule(NULL, side);
        }
    }
}

static void Engine_relaunchWorkers(Engine* that, int threadCount) {
	if (threadCount == that->getInternal()->threadCount)
		return;

    std::vector<EngineWorker>& workers = that->getInternal()->workers;

	if (that->getInternal()->threadCount > 0) {
		// Stop engine workers
		for (EngineWorker& worker : workers) {
			worker.requestStop();
		}
		that->getInternal()->engineBarrier.wait();

		// Join and destroy engine workers
		for (EngineWorker& worker : workers) {
			worker.join();
		}
		workers.resize(0);
	}

	// Configure engine
	that->getInternal()->threadCount = threadCount;

	// Set barrier counts
	that->getInternal()->engineBarrier.setThreads(threadCount);
	that->getInternal()->workerBarrier.setThreads(threadCount);

	if (threadCount > 0) {
		// Create and start engine workers
		that->getInternal()->workers.resize(threadCount - 1);
		for (int id = 1; id < threadCount; id++) {
			EngineWorker& worker = that->getInternal()->workers[id - 1];
			worker.id = id;
			worker.engine = that;
			worker.start();
		}
	}
}


static void Engine_stepWorker(Engine* that, int threadId) {
	int modulesLen = that->getInternal()->modules.size();

	// Build ProcessArgs
	Module::ProcessArgs processArgs;
	processArgs.sampleRate = that->getSampleRate();
	processArgs.sampleTime = that->getSampleTime();
	processArgs.frame = that->getFrame();

	// Step each module
	while (true) {
		// Choose next module
		// First-come-first serve module-to-thread allocation algorithm
		int i = that->getInternal()->workerModuleIndex.fetch_add(1);
		if (i >= modulesLen)
			break;

		Module* module = that->getModules()[i];
		module->doProcess(processArgs);
	}
}

static void Engine_stepFrameCables(Engine* that) {
    auto finitize = [](float x) { return std::isfinite(x) ? x : 0.f; };

    // Iterate each cable input group, since `cables` is sorted by input
    auto cables = that->getInternal()->cables;
    auto firstIt = cables.begin();
    while (firstIt != cables.end()) {
        Cable* firstCable = *firstIt;
        Input* input = &firstCable->inputModule->inputs[firstCable->inputId];

        // Find end of input group
        auto endIt = firstIt;
        while (++endIt != cables.end()) {
            Cable* endCable = *endIt;
            // Check inputId first since it changes more frequently between
            // cables
            if (!(endCable->inputId == firstCable->inputId &&
                  endCable->inputModule == firstCable->inputModule))
                break;
        }

        // Since stackable inputs are uncommon, only use stackable input logic
        // if there are multiple cables in input group.
        if (endIt - firstIt == 1) {
            Output* output =
                &firstCable->outputModule->outputs[firstCable->outputId];
            // Copy all voltages from output to input
            for (uint8_t c = 0; c < output->channels; c++) {
                input->voltages[c] = finitize(output->voltages[c]);
            }
            // Set higher channel voltages to 0
            for (uint8_t c = output->channels; c < input->channels; c++) {
                input->voltages[c] = 0.f;
            }
            input->channels = output->channels;
        } else {
            // Calculate max output channels
            uint8_t channels = 0;
            for (auto it = firstIt; it < endIt; ++it) {
                Cable* cable = *it;
                Output* output = &cable->outputModule->outputs[cable->outputId];
                channels = std::max(channels, output->channels);
            }

            // Clear input channels, including old channels
            for (uint8_t c = 0; c < std::max(channels, input->channels); c++) {
                input->voltages[c] = 0.f;
            }
            input->channels = channels;

            // Sum outputs of cables
            for (auto it = firstIt; it < endIt; ++it) {
                Cable* cable = *it;
                Output* output = &cable->outputModule->outputs[cable->outputId];

                // Sum monophonic value to all input channels
                if (output->channels == 1) {
                    float value = finitize(output->voltages[0]);
                    for (uint8_t c = 0; c < channels; c++) {
                        input->voltages[c] += value;
                    }
                }
                // Sum polyphonic values to each input channel
                else {
                    for (uint8_t c = 0; c < output->channels; c++) {
                        input->voltages[c] += finitize(output->voltages[c]);
                    }
                }
            }
        }

        firstIt = endIt;
    }
}

/** Steps a single frame
 */
static void Engine_stepFrame(Engine* that) {
    // Param smoothing
    Module* smoothModule = that->getInternal()->smoothModule;
    if (smoothModule) {
        int smoothParamId = that->getInternal()->smoothParamId;
        float smoothValue = that->getInternal()->smoothValue;
        Param* smoothParam = &smoothModule->params[smoothParamId];
        float value = smoothParam->value;
        // Use decay rate of roughly 1 graphics frame
        const float smoothLambda = 60.f;
        float newValue =
            value + (smoothValue - value) * smoothLambda * that->getSampleTime();
        if (value == newValue) {
            // Snap to actual smooth value if the value doesn't change enough
            // (due to the granularity of floats)
            smoothParam->setValue(smoothValue);
            that->getInternal()->smoothModule = nullptr;
            that->getInternal()->smoothParamId = 0;
        } else {
            smoothParam->setValue(newValue);
        }
    }

    // Step modules along with workers
    that->getInternal()->workerModuleIndex = 0;
    that->getInternal()->engineBarrier.wait();
    Engine_stepWorker(that, 0);
    that->getInternal()->workerBarrier.wait();

    Engine_stepFrameCables(that);   

    // Flip messages for each module
    for (Module* module : that->getModules()) {
        if (module->leftExpander.messageFlipRequested) {
            std::swap(module->leftExpander.producerMessage,
                      module->leftExpander.consumerMessage);
            module->leftExpander.messageFlipRequested = false;
        }
        if (module->rightExpander.messageFlipRequested) {
            std::swap(module->rightExpander.producerMessage,
                      module->rightExpander.consumerMessage);
            module->rightExpander.messageFlipRequested = false;
        }
    }

    that->incrementFrame();
}

static void Engine_refreshParamHandleCache(Engine* that) {
    // Clear cache
    auto cache = that->getInternal()->paramHandlesCache;
    cache.clear();

    // Add active ParamHandles to cache
    for (ParamHandle* paramHandle : that->getInternal()->paramHandles) {
        if (paramHandle->moduleId >= 0) {
            cache[std::make_tuple(
                paramHandle->moduleId, paramHandle->paramId)] = paramHandle;
        }
    }
}

Engine::Engine() {
	internal_ = new Internal;

	internal_->context = contextGet();
	setSuggestedSampleRate(0.f);
}


Engine::~Engine() {
	// Stop fallback thread if running
	{
		std::lock_guard<std::mutex> lock(internal_->fallbackMutex);
		internal_->fallbackRunning = false;
		internal_->fallbackCv.notify_all();
	}
	if (internal_->fallbackThread.joinable())
		internal_->fallbackThread.join();

	// Shut down workers
	Engine_relaunchWorkers(this, 0);

	// Clear modules, cables, etc
	clear();

	// Make sure there are no cables or modules in the rack on destruction.
	// If this happens, a module must have failed to remove itself before the RackWidget was destroyed.
	assert(internal_->cables.empty());
	assert(internal_->modules.empty());
	assert(internal_->paramHandles.empty());

	assert(internal_->modulesCache.empty());
	assert(internal_->cablesCache.empty());
	assert(internal_->paramHandlesCache.empty());

	delete internal_;
}

Engine::Internal* Engine::getInternal() const {
    return internal_;
} 

void Engine::clear() {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	clear_NoLock();
}


void Engine::clear_NoLock() {
	// Copy lists because we'll be removing while iterating
	std::set<ParamHandle*> paramHandles = internal_->paramHandles;
	for (ParamHandle* paramHandle : paramHandles) {
		removeParamHandle_NoLock(paramHandle);
		// Don't delete paramHandle because they're normally owned by Module subclasses
	}
	std::vector<Cable*> cables = internal_->cables;
	for (Cable* cable : cables) {
		removeCable_NoLock(cable);
		delete cable;
	}
	std::vector<Module*> modules = internal_->modules;
	for (Module* module : modules) {
		removeModule_NoLock(module);
		delete module;
	}
}


void Engine::stepBlock(int frames) {
	// Start timer before locking
	double startTime = system::getTime();

	std::lock_guard<std::mutex> stepLock(internal_->blockMutex);
	SharedLock<SharedMutex> lock(internal_->mutex);
    
	// Configure thread
	system::resetFpuFlags();

	internal_->blockFrame = internal_->frame;
	internal_->blockTime = system::getTime();
	internal_->blockFrames = frames;

	// Update expander pointers
	for (Module* module : internal_->modules) {
		Engine_updateExpander_NoLock(this, module, 0);
		Engine_updateExpander_NoLock(this, module, 1);
	}

	// Launch workers
	Engine_relaunchWorkers(this, settings::threadCount);

	// Step individual frames
	for (int i = 0; i < frames; i++) {
		Engine_stepFrame(this);
	}

	yieldWorkers();

	internal_->block++;

	// Stop timer
	double endTime = system::getTime();
	double meter = (endTime - startTime) / (frames * internal_->sampleTime);
	internal_->meterTotal += meter;
	internal_->meterMax = std::fmax(internal_->meterMax, meter);
	internal_->meterCount++;

	// Update meter values. Note that these values are only used for
    // Windows machines. For others, CPU usage is obtained from the system.
	const double meterUpdateDuration = 1.0;
	if (startTime - internal_->meterLastTime >= meterUpdateDuration) {
		internal_->meterLastAverage = internal_->meterTotal / internal_->meterCount;
		internal_->meterLastMax = internal_->meterMax;
		internal_->meterLastTime = startTime;
		internal_->meterCount = 0;
		internal_->meterTotal = 0.0;
		internal_->meterMax = 0.0;
	}
}


void Engine::setMasterModule(Module* module) {
	if (module == internal_->masterModule)
		return;
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	setMasterModule_NoLock(module);
}


void Engine::setMasterModule_NoLock(Module* module) {
	if (module == internal_->masterModule)
		return;

	if (internal_->masterModule) {
		// Dispatch UnsetMasterEvent
		Module::UnsetMasterEvent e;
		internal_->masterModule->onUnsetMaster(e);
	}

	internal_->masterModule = module;

	if (internal_->masterModule) {
		// Dispatch SetMasterEvent
		Module::SetMasterEvent e;
		internal_->masterModule->onSetMaster(e);
	}

	// Wake up fallback thread if master module was unset
	if (!internal_->masterModule) {
		internal_->fallbackCv.notify_all();
	}
}


Module* Engine::getMasterModule() {
	return internal_->masterModule;
}


float Engine::getSampleRate() {
	return internal_->sampleRate;
}

float Engine::getSampleTime() {
    return internal_->sampleTime;
}


void Engine::setSampleRate(float sampleRate) {
	if (sampleRate == internal_->sampleRate)
		return;
	std::lock_guard<SharedMutex> lock(internal_->mutex);

	internal_->sampleRate = sampleRate;
	internal_->sampleTime = 1.f / sampleRate;
	// Dispatch SampleRateChangeEvent
	Module::SampleRateChangeEvent e;
	e.sampleRate = internal_->sampleRate;
	e.sampleTime = internal_->sampleTime;
	for (Module* module : internal_->modules) {
		module->onSampleRateChange(e);
	}
}


void Engine::setSuggestedSampleRate(float suggestedSampleRate) {
	if (settings::sampleRate > 0) {
		setSampleRate(settings::sampleRate);
	}
	else if (suggestedSampleRate > 0) {
		setSampleRate(suggestedSampleRate);
	}
	else {
		// Fallback sample rate
		setSampleRate(44100.f);
	}
}


void Engine::yieldWorkers() {
	internal_->workerBarrier.yield();
}


int64_t Engine::getFrame() {
	return internal_->frame;
}


void Engine::incrementFrame() const{
    internal_->frame++;
}


int64_t Engine::getBlock() {
	return internal_->block;
}


int64_t Engine::getBlockFrame() {
	return internal_->blockFrame;
}


double Engine::getBlockTime() {
	return internal_->blockTime;
}


int Engine::getBlockFrames() {
	return internal_->blockFrames;
}


double Engine::getBlockDuration() {
	return internal_->blockFrames * internal_->sampleTime;
}


double Engine::getMeterAverage() {
#if defined ARCH_WIN
    // Windows-specific implementation
    return 100.0 * internal_->meterLastAverage;
#else
    // Other platforms get value using command line
    return system::getSystemCpuPercentage();
#endif
}


double Engine::getMeterMax() {
#if defined ARCH_WIN
    // Windows-specific implementation
    return 100.0 * internal_->meterLastMax;
#else
    // Other platforms get value using command line
    return system::getSystemCpuPercentage();
#endif
}


std::vector<Module*> Engine::getModules() const {
    return internal_->modules;
}


size_t Engine::getModuleIds(int64_t* moduleIds, size_t len) {
	SharedLock<SharedMutex> lock(internal_->mutex);
	size_t i = 0;
	for (Module* m : internal_->modules) {
		if (i >= len)
			break;
		moduleIds[i] = m->id;
		i++;
	}
	return i;
}


std::vector<int64_t> Engine::getModuleIds() {
	SharedLock<SharedMutex> lock(internal_->mutex);
	std::vector<int64_t> moduleIds;
	moduleIds.reserve(internal_->modules.size());
	for (Module* m : internal_->modules) {
		moduleIds.push_back(m->id);
	}
	return moduleIds;
}


void Engine::addModule(Module* module) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	addModule_NoLock(module);
}


void Engine::addModule_NoLock(Module* module) {
	assert(module);
	// Check that the module is not already added
	auto it = std::find(internal_->modules.begin(), internal_->modules.end(), module);
	assert(it == internal_->modules.end());
	// Set ID if unset or collides with an existing ID
	while (module->id < 0 || internal_->modulesCache.find(module->id) != internal_->modulesCache.end()) {
		// Randomly generate ID
		module->id = random::u64() % (1ull << 53);
	}
	// Add module
	internal_->modules.push_back(module);
	internal_->modulesCache[module->id] = module;
	// Dispatch AddEvent
	Module::AddEvent eAdd;
	module->onAdd(eAdd);
	// Dispatch SampleRateChangeEvent
	Module::SampleRateChangeEvent eSrc;
	eSrc.sampleRate = internal_->sampleRate;
	eSrc.sampleTime = internal_->sampleTime;
	module->onSampleRateChange(eSrc);
	// Update ParamHandles' module pointers
	for (ParamHandle* paramHandle : internal_->paramHandles) {
		if (paramHandle->moduleId == module->id)
			paramHandle->module = module;
	}
}


void Engine::removeModule(Module* module) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	removeModule_NoLock(module);
}


void Engine::removeModule_NoLock(Module* module) {
	assert(module);
	// Check that the module actually exists
	auto it = std::find(internal_->modules.begin(), internal_->modules.end(), module);
	assert(it != internal_->modules.end());
	// Dispatch RemoveEvent
	Module::RemoveEvent eRemove;
	module->onRemove(eRemove);
	// Update ParamHandles' module pointers
	for (ParamHandle* paramHandle : internal_->paramHandles) {
		if (paramHandle->moduleId == module->id)
			paramHandle->module = NULL;
	}
	// Unset master module
	if (getMasterModule() == module) {
		setMasterModule_NoLock(NULL);
	}
	// If a param is being smoothed on this module, stop smoothing it immediately
	if (module == internal_->smoothModule) {
		internal_->smoothModule = NULL;
	}
	// Check that all cables are disconnected
	for (Cable* cable : internal_->cables) {
		assert(cable->inputModule != module);
		assert(cable->outputModule != module);
	}
	// Update expanders of other modules
	for (Module* m : internal_->modules) {
		for (uint8_t side = 0; side < 2; side++) {
			Module::Expander& expander = m->getExpander(!side);
			if (expander.moduleId == module->id) {
				expander.moduleId = -1;
			}
			if (expander.module == module) {
				m->setExpanderModule(NULL, !side);
			}
		}
	}
	// Update expanders of this module
	for (uint8_t side = 0; side < 2; side++) {
		Module::Expander& expander = module->getExpander(side);
		expander.moduleId = -1;
		module->setExpanderModule(NULL, side);
	}
	// Remove module
	internal_->modulesCache.erase(module->id);
	internal_->modules.erase(it);
}


bool Engine::hasModule(Module* module) {
	SharedLock<SharedMutex> lock(internal_->mutex);
	// TODO Performance could be improved by searching modulesCache, but more testing would be needed to make sure it's always valid.
	auto it = std::find(internal_->modules.begin(), internal_->modules.end(), module);
	return it != internal_->modules.end();
}


Module* Engine::getModule(int64_t moduleId) {
	SharedLock<SharedMutex> lock(internal_->mutex);
	return getModule_NoLock(moduleId);
}


Module* Engine::getModule_NoLock(int64_t moduleId) {
	if (moduleId < 0)
		return NULL;
	auto it = internal_->modulesCache.find(moduleId);
	if (it == internal_->modulesCache.end())
		return NULL;
	return it->second;
}


void Engine::resetModule(Module* module) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	assert(module);

	Module::ResetEvent eReset;
	module->onReset(eReset);
}


void Engine::randomizeModule(Module* module) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	assert(module);

	Module::RandomizeEvent eRandomize;
	module->onRandomize(eRandomize);
}


void Engine::bypassModule(Module* module, bool bypassed) {
	assert(module);
	if (module->isBypassed() == bypassed)
		return;

	std::lock_guard<SharedMutex> lock(internal_->mutex);

	// Clear outputs and set to 1 channel
	for (Output& output : module->outputs) {
		// This zeros all voltages, but the channel is set to 1 if connected
		output.setChannels(0);
	}
	// Set bypassed state
	module->setBypassed(bypassed);
	if (bypassed) {
		// Dispatch BypassEvent
		Module::BypassEvent eBypass;
		module->onBypass(eBypass);
	}
	else {
		// Dispatch UnBypassEvent
		Module::UnBypassEvent eUnBypass;
		module->onUnBypass(eUnBypass);
	}
}


json_t* Engine::moduleToJson(Module* module) {
	SharedLock<SharedMutex> lock(internal_->mutex);
	return module->toJson();
}


void Engine::moduleFromJson(Module* module, json_t* rootJ) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	module->fromJson(rootJ);
}


void Engine::prepareSaveModule(Module* module) {
	SharedLock<SharedMutex> lock(internal_->mutex);
	Module::SaveEvent e;
	module->onSave(e);
}


void Engine::prepareSave() {
	SharedLock<SharedMutex> lock(internal_->mutex);
	for (Module* module : internal_->modules) {
		Module::SaveEvent e;
		module->onSave(e);
	}
}


void Engine::addCable(Cable* cable) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	addCable_NoLock(cable);
}


void Engine::addCable_NoLock(Cable* cable) {
	assert(cable);
	// Check cable properties
	assert(cable->inputModule);
	assert(cable->outputModule);
	Input& input = cable->inputModule->inputs[cable->inputId];
	Output& output = cable->outputModule->outputs[cable->outputId];
	bool inputWasConnected = false;
	bool outputWasConnected = false;
	for (Cable* cable2 : internal_->cables) {
		// Check that the cable is not already added
		assert(cable2 != cable);
		// Check that cable isn't similar to another cable
		// assert(!(cable2->inputModule == cable->inputModule && cable2->inputId == cable->inputId && cable2->outputModule == cable->outputModule && cable2->outputId == cable->outputId));
		// Check if input is already connected to a cable
		if (cable2->inputModule == cable->inputModule && cable2->inputId == cable->inputId)
			inputWasConnected = true;
		// Check if output is already connected to a cable
		if (cable2->outputModule == cable->outputModule && cable2->outputId == cable->outputId)
			outputWasConnected = true;
	}
	// Set ID if unset or collides with an existing ID
	while (cable->id < 0 || internal_->cablesCache.find(cable->id) != internal_->cablesCache.end()) {
		// Generate random 52-bit ID
		cable->id = random::u64() % (1ull << 53);
	}
	// Add the cable
	internal_->cables.push_back(cable);
	// Sort cable by input so they are grouped in stepFrame()
	std::sort(internal_->cables.begin(), internal_->cables.end(), [](Cable* a, Cable* b) {
		return std::make_tuple(a->inputModule, a->inputId) < std::make_tuple(b->inputModule, b->inputId);
	});
	// Set default number of input/output channels
	if (!inputWasConnected) {
		input.channels = 1;
	}
	if (!outputWasConnected) {
		output.channels = 1;
	}
	// Add caches
	internal_->cablesCache[cable->id] = cable;
	// Dispatch input port event
	if (!inputWasConnected) {
		Module::PortChangeEvent e;
		e.connecting = true;
		e.type = Port::INPUT;
		e.portId = cable->inputId;
		cable->inputModule->onPortChange(e);
	}
	// Dispatch output port event if its state went from disconnected to connected.
	if (!outputWasConnected) {
		Module::PortChangeEvent e;
		e.connecting = true;
		e.type = Port::OUTPUT;
		e.portId = cable->outputId;
		cable->outputModule->onPortChange(e);
	}
}


void Engine::removeCable(Cable* cable) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	removeCable_NoLock(cable);
}


void Engine::removeCable_NoLock(Cable* cable) {
	assert(cable);
	Input& input = cable->inputModule->inputs[cable->inputId];
	Output& output = cable->outputModule->outputs[cable->outputId];
	// Check that the cable is already added
	auto it = std::find(internal_->cables.begin(), internal_->cables.end(), cable);
	assert(it != internal_->cables.end());
	// Remove cable caches
	internal_->cablesCache.erase(cable->id);
	// Remove cable
	internal_->cables.erase(it);
	// Check if input/output is still connected to a cable
	bool inputIsConnected = false;
	bool outputIsConnected = false;
	for (Cable* cable2 : internal_->cables) {
		if (cable2->inputModule == cable->inputModule && cable2->inputId == cable->inputId) {
			inputIsConnected = true;
		}
		if (cable2->outputModule == cable->outputModule && cable2->outputId == cable->outputId) {
			outputIsConnected = true;
		}
	}
	// Set input as disconnected if disconnected from all cables
	if (!inputIsConnected) {
		input.channels = 0;
		// Clear input values
		for (uint8_t c = 0; c < PORT_MAX_CHANNELS; c++) {
			input.setVoltage(0.f, c);
		}
	}
	// Set output as disconnected if disconnected from all cables
	if (!outputIsConnected) {
		output.channels = 0;
		// Don't clear output values
	}
	// Dispatch input port event
	if (!inputIsConnected) {
		Module::PortChangeEvent e;
		e.connecting = false;
		e.type = Port::INPUT;
		e.portId = cable->inputId;
		cable->inputModule->onPortChange(e);
	}
	// Dispatch output port event
	if (!outputIsConnected) {
		Module::PortChangeEvent e;
		e.connecting = false;
		e.type = Port::OUTPUT;
		e.portId = cable->outputId;
		cable->outputModule->onPortChange(e);
	}
}


Cable* Engine::getCable(int64_t cableId) {
	if (cableId < 0)
		return NULL;
	SharedLock<SharedMutex> lock(internal_->mutex);
	auto it = internal_->cablesCache.find(cableId);
	if (it == internal_->cablesCache.end())
		return NULL;
	return it->second;
}


std::vector<int64_t> Engine::getCableIds() {
    return internal_->getCableIds();
}


void Engine::setParamValue(Module* module, int paramId, float value) {
	// If param is being smoothed, cancel smoothing.
	if (internal_->smoothModule == module && internal_->smoothParamId == paramId) {
		internal_->smoothModule = NULL;
		internal_->smoothParamId = 0;
	}
	module->params[paramId].setValue(value);
}


float Engine::getParamValue(Module* module, int paramId) {
	return module->params[paramId].getValue();
}


void Engine::addParamHandle(ParamHandle* paramHandle) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	// New ParamHandles must be blank.
	// This means we don't have to refresh the cache.
	assert(paramHandle->moduleId < 0);

	// Check that the ParamHandle is not already added
	auto it = internal_->paramHandles.find(paramHandle);
	assert(it == internal_->paramHandles.end());

	// Add it
	internal_->paramHandles.insert(paramHandle);
	// No need to refresh the cache because the moduleId is not set.
}


void Engine::removeParamHandle(ParamHandle* paramHandle) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	removeParamHandle_NoLock(paramHandle);
}


void Engine::removeParamHandle_NoLock(ParamHandle* paramHandle) {
	// Check that the ParamHandle is already added
	auto it = internal_->paramHandles.find(paramHandle);
	assert(it != internal_->paramHandles.end());

	// Remove it
	paramHandle->module = NULL;
	internal_->paramHandles.erase(it);
	Engine_refreshParamHandleCache(this);
}


ParamHandle* Engine::getParamHandle(int64_t moduleId, int paramId) {
	SharedLock<SharedMutex> lock(internal_->mutex);
	return getParamHandle_NoLock(moduleId, paramId);
}


ParamHandle* Engine::getParamHandle_NoLock(int64_t moduleId, int paramId) {
	auto it = internal_->paramHandlesCache.find(std::make_tuple(moduleId, paramId));
	if (it == internal_->paramHandlesCache.end())
		return NULL;
	return it->second;
}


ParamHandle* Engine::getParamHandle(Module* module, int paramId) {
	return getParamHandle(module->id, paramId);
}


void Engine::updateParamHandle(ParamHandle* paramHandle, int64_t moduleId, int paramId, bool overwrite) {
	std::lock_guard<SharedMutex> lock(internal_->mutex);
	updateParamHandle_NoLock(paramHandle, moduleId, paramId, overwrite);
}


void Engine::updateParamHandle_NoLock(ParamHandle* paramHandle, int64_t moduleId, int paramId, bool overwrite) {
	// Check that it exists
	auto it = internal_->paramHandles.find(paramHandle);
	assert(it != internal_->paramHandles.end());

	// Set IDs
	paramHandle->moduleId = moduleId;
	paramHandle->paramId = paramId;
	paramHandle->module = NULL;
	// At this point, the ParamHandle cache might be invalid.

	if (paramHandle->moduleId >= 0) {
		// Replace old ParamHandle, or reset the current ParamHandle
		ParamHandle* oldParamHandle = getParamHandle_NoLock(moduleId, paramId);
		if (oldParamHandle) {
			if (overwrite) {
				oldParamHandle->moduleId = -1;
				oldParamHandle->paramId = 0;
				oldParamHandle->module = NULL;
			}
			else {
				paramHandle->moduleId = -1;
				paramHandle->paramId = 0;
				paramHandle->module = NULL;
			}
		}
	}

	// Set module pointer if the above block didn't reset it
	if (paramHandle->moduleId >= 0) {
		paramHandle->module = getModule_NoLock(paramHandle->moduleId);
	}

	Engine_refreshParamHandleCache(this);
}


json_t* Engine::toJson() {
	SharedLock<SharedMutex> lock(internal_->mutex);
	json_t* rootJ = json_object();

	// modules
	json_t* modulesJ = json_array();
	for (Module* module : internal_->modules) {
		json_t* moduleJ = module->toJson();
		json_array_append_new(modulesJ, moduleJ);
	}
	json_object_set_new(rootJ, "modules", modulesJ);

	// cables
	json_t* cablesJ = json_array();
	for (Cable* cable : internal_->cables) {
		json_t* cableJ = cable->toJson();
		json_array_append_new(cablesJ, cableJ);
	}
	json_object_set_new(rootJ, "cables", cablesJ);

	// masterModule
	if (internal_->masterModule) {
		json_object_set_new(rootJ, "masterModuleId", json_integer(internal_->masterModule->id));
	}

	return rootJ;
}


void Engine::fromJson(json_t* rootJ) {
	clear();

	// modules
	// We can't instantiate modules before clearing because some modules add ParamHandles upon construction.
	// We also can't lock while instantiating modules because they call addParamHandle() which locks.
	std::vector<Module*> modules;
	json_t* modulesJ = json_object_get(rootJ, "modules");
	if (!modulesJ)
		return;
	size_t moduleIndex;
	json_t* moduleJ;
	json_array_foreach(modulesJ, moduleIndex, moduleJ) {
		// Get model
		plugin::Model* model;
		try {
			model = plugin::modelFromJson(moduleJ);
		}
		catch (Exception& e) {
			WARN("Cannot load model: %s", e.what());
			continue;
		}

		// Create module
		DEBUG("Creating module %s", model->getFullName().c_str());
		Module* module = model->createModule();
		assert(module);

		try {
			module->fromJson(moduleJ);

			// Before 1.0, the module ID was the index in the "modules" array
			if (module->id < 0) {
				module->id = moduleIndex;
			}
		}
		catch (Exception& e) {
			WARN("Cannot load module: %s", e.what());
			delete module;
			continue;
		}

		modules.push_back(module);
	}

	std::lock_guard<SharedMutex> lock(internal_->mutex);

	// Add modules
	for (Module* module : modules) {
		addModule_NoLock(module);
	}

	// cables
	json_t* cablesJ = json_object_get(rootJ, "cables");
	// Before 1.0, cables were called wires
	if (!cablesJ)
		cablesJ = json_object_get(rootJ, "wires");
	if (!cablesJ)
		return;
	size_t cableIndex;
	json_t* cableJ;
	json_array_foreach(cablesJ, cableIndex, cableJ) {
		// cable
		Cable* cable = new Cable;

		try {
			cable->fromJson(cableJ);

			// Before 1.0, the cable ID was the index in the "cables" array
			if (cable->id < 0) {
				cable->id = cableIndex;
			}

			addCable_NoLock(cable);
		}
		catch (Exception& e) {
			WARN("Cannot load cable: %s", e.what());
			delete cable;
			continue;
		}
	}

	// masterModule
	json_t* masterModuleIdJ = json_object_get(rootJ, "masterModuleId");
	if (masterModuleIdJ) {
		Module* masterModule = getModule_NoLock(json_integer_value(masterModuleIdJ));
		setMasterModule_NoLock(masterModule);
	}
}


void EngineWorker::run() {
	// Configure thread
	contextSet(engine->getInternal()->context);
	system::setThreadName(string::f("Worker %d", id));
	system::resetFpuFlags();

	while (true) {
		engine->getInternal()->engineBarrier.wait();
		if (!running)
			return;
		Engine_stepWorker(engine, id);
		engine->getInternal()->workerBarrier.wait();
	}
}


static void Engine_fallbackRun(Engine* that) {
	system::setThreadName("Engine fallback");
	contextSet(that->getInternal()->context);

	while (that->getInternal()->fallbackRunning) {
		if (!that->getMasterModule()) {
			// Step blocks and wait
			double start = system::getTime();

            // Uses a number of audio frames to determine max and average meter/CPU loads.
            // Originally this value was a really tiny timeslice of 1/60th of a second,
            // But for that short of a time the average and max values were always going to
            // be the same. Therefore now using a full second's worth of audio frames.
			int frames = std::floor(that->getSampleRate() * 1 / 1 /* was 60 */);
			that->stepBlock(frames);
			double end = system::getTime();

			double duration = frames * that->getSampleTime() - (end - start);
			if (duration > 0.0) {
				std::this_thread::sleep_for(std::chrono::duration<double>(duration));
			}
		}
		else {
			// Wait for master module to be unset, or for the request to stop running
			std::unique_lock<std::mutex> lock(that->getInternal()->fallbackMutex);
			that->getInternal()->fallbackCv.wait(lock, [&]() {
				return !that->getInternal()->fallbackRunning || !that->getMasterModule();
			});
		}
	}
}


float Engine::getParamSmoothValue(Module* module, int paramId) {
    return internal_->getParamSmoothValue(module, paramId);
}

void Engine::setParamSmoothValue(Module* module, int paramId, float value) {
    internal_->setParamSmoothValue(module, paramId, value);
}

void Engine::startFallbackThread() {
	if (internal_->fallbackThread.joinable())
		return;

	internal_->fallbackRunning = true;
	internal_->fallbackThread = std::thread(Engine_fallbackRun, this);
}


} // namespace engine
} // namespace rack
