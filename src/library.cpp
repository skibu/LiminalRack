#include <thread>
#include <mutex>
#include <condition_variable>

#include <library.hpp>
#include <settings.hpp>
#include <app/common.hpp>
#include <network.hpp>
#include <system.hpp>
#include <context.hpp>
#include <window/Window.hpp>
#include <asset.hpp>
#include <settings.hpp>
#include <plugin.hpp>
#include <string.hpp>


namespace rack {
namespace library {


static std::mutex appUpdateMutex_;
static std::mutex updateMutex_;
static std::mutex timeoutMutex_;
static std::condition_variable updateCv_;

/** Whether plugins are currently downloading. */
static bool isSyncing_;

/** Whether the UI should ask the user to restart after updating plugins. */
static bool restartRequested_;

/** Whether the UI should refresh the plugin updates menu. */
static bool refreshRequested_;

std::string appVersion_;
std::string appDownloadUrl_;
std::string appChangelogUrl_;

std::string loginStatus_;
// Info about each plugin that has an update available
std::map<std::string, UpdateInfo> updateInfos_;
std::string updateStatus_;
std::string updateSlug_;
float updateProgress_ = 0.f;

void init() {
    // Don't start update thread if auto updates are disabled
    if (!settings::autoCheckUpdates) return;

    // Dev mode is typically used when Rack or plugins are compiled from source,
    // so updating might overwrite assets.
    if (settings::devMode) return;

    // Safe mode disables plugin loading, so Rack will unnecessarily try to sync
    // all plugins.
    if (settings::safeMode) return;

    std::thread t([&]() {
        system::setThreadName("Library");
        // Wait a few seconds before updating in case library is destroyed
        // immediately afterwards
        {
            std::unique_lock<std::mutex> lock(timeoutMutex_);
            if (updateCv_.wait_for(lock, std::chrono::duration<double>(4.0)) !=
                std::cv_status::timeout)
                return;
        }

        checkAppUpdate();
        checkUpdates();
    });
    t.detach();
}

void destroy() {
    // Wait until all library threads are finished
    updateCv_.notify_all();
    std::lock_guard<std::mutex> timeoutLock(timeoutMutex_);
    std::lock_guard<std::mutex> appUpdateLock(appUpdateMutex_);
    std::lock_guard<std::mutex> updateLock(updateMutex_);

    // Clear globals in case init() is called again
    loginStatus_ = "";
    updateInfos_.clear();
}

void checkAppUpdate() {
	if (!appUpdateMutex_.try_lock())
		return;
	DEFER({appUpdateMutex_.unlock();});

    INFO("Checking for plugin library updates...");

    // Get latest version info of Rack from VCV Rack API
	std::string versionUrl = API_URL + "/version";
	json_t* reqJ = json_object();
	json_object_set_new(reqJ, "edition", json_string(APP_EDITION.c_str()));
	DEFER({json_decref(reqJ);});

    // Get the JSON response
	json_t* resJ = network::requestJson(network::METHOD_GET, versionUrl, reqJ);
	if (!resJ) {
		WARN("Request for version failed");
		return;
	}
	DEFER({json_decref(resJ);});

	json_t* versionJ = json_object_get(resJ, "version");
	if (versionJ) {
		std::string appVersion = json_string_value(versionJ);
		// Check if app version is more recent than current version
		if (string::Version(APP_VERSION) < string::Version(appVersion))
			library::appVersion_ = appVersion;
	}

	json_t* changelogUrlJ = json_object_get(resJ, "changelogUrl");
	if (changelogUrlJ)
		appChangelogUrl_ = json_string_value(changelogUrlJ);

	json_t* downloadUrlsJ = json_object_get(resJ, "downloadUrls");
	if (downloadUrlsJ) {
		std::string arch = APP_OS + "-" + APP_CPU;
		json_t* downloadUrlJ = json_object_get(downloadUrlsJ, arch.c_str());
		if (downloadUrlJ)
			appDownloadUrl_ = json_string_value(downloadUrlJ);
	}

    DEBUG("Done initiating the checking for plugin library updates");
}


bool isAppUpdateAvailable() {
	return (appVersion_ != "");
}


bool isLoggedIn() {
	return settings::token != "";
}


void logIn(std::string email, std::string password) {
	if (!updateMutex_.try_lock())
		return;
	DEFER({updateMutex_.unlock();});

	loginStatus_ = string::translate("library.loggingIn");
	json_t* reqJ = json_object();
	json_object_set_new(reqJ, "email", json_string(email.c_str()));
	json_object_set_new(reqJ, "password", json_string(password.c_str()));
	std::string url = API_URL + "/token";
	json_t* resJ = network::requestJson(network::METHOD_POST, url, reqJ);
	json_decref(reqJ);

	if (!resJ) {
		loginStatus_ = string::translate("library.noResponse");
		return;
	}
	DEFER({json_decref(resJ);});

	json_t* errorJ = json_object_get(resJ, "error");
	if (errorJ) {
		const char* errorStr = json_string_value(errorJ);
		loginStatus_ = errorStr;
		return;
	}

	json_t* tokenJ = json_object_get(resJ, "token");
	if (!tokenJ) {
		loginStatus_ = string::translate("library.noToken");
		return;
	}

	const char* tokenStr = json_string_value(tokenJ);
	settings::token = tokenStr;
	loginStatus_ = "";
	refreshRequested_ = true;
}


void logOut() {
	settings::token = "";
	updateInfos_.clear();
}


static network::CookieMap getTokenCookies() {
	network::CookieMap cookies;
	cookies["token"] = settings::token;
	return cookies;
}

/** Checks to see if updates are available for the user's plugins.
 * Any plugins can and should be updated are added to updateInfos_.
 * Can then call hasUpdates() to see if any updates were found.
 */
void checkUpdates() {
    if (!updateMutex_.try_lock()) return;
    DEFER({ updateMutex_.unlock(); });

    // If user not logged in can't check for updates
    if (settings::token.empty()) return;

    // Refuse to check for updates while updating plugins
    if (isSyncing_) return;

    updateStatus_ = string::translate("library.queryingUpdates");

    // Check user token
    std::string userUrl = API_URL + "/user";
    json_t* userResJ = network::requestJson(network::METHOD_GET, userUrl, NULL,
                                            getTokenCookies());
    if (!userResJ) {
        WARN("Request for user account failed");
        updateStatus_ = string::translate("library.queryAccountFailed");
        return;
    }
    DEFER({ json_decref(userResJ); });

    json_t* userErrorJ = json_object_get(userResJ, "error");
    if (userErrorJ) {
        std::string userError = json_string_value(userErrorJ);
        WARN("Request for user account error: %s", userError.c_str());
        // Unset token
        settings::token = "";
        refreshRequested_ = true;
        return;
    }

    // Get library manifests for all libraries on VCV Rack
    std::string manifestsUrl = API_URL + "/library/manifests";
    json_t* manifestsReq = json_object();
    json_object_set_new(manifestsReq, "version",
                        json_string(APP_VERSION_MAJOR.c_str()));
    json_t* manifestsFromVcvRackJ =
        network::requestJson(network::METHOD_GET, manifestsUrl, manifestsReq);
    json_decref(manifestsReq);
    if (!manifestsFromVcvRackJ) {
        WARN("Request for library manifests from VCV Rackfailed");
        updateStatus_ = string::translate("library.queryManifestsFailed");
        return;
    }
    DEFER({ json_decref(manifestsFromVcvRackJ); });

    // Get user's plugin libraries and modules that they have enabled
    std::string modulesUrl = API_URL + "/modules";
    json_t* modulesLoadedJ = network::requestJson(
        network::METHOD_GET, modulesUrl, NULL, getTokenCookies());
    if (!modulesLoadedJ) {
        WARN("Request for user's modules failed");
        updateStatus_ = string::translate("library.queryModulesFailed");
        return;
    }
    DEFER({ json_decref(modulesLoadedJ); });

    // For each plugin library (manufacturer) that user has modules loaded
    // from, check if an update is available.
    json_t* manifestListFromVcvRackJ =
        json_object_get(manifestsFromVcvRackJ, "manifests");
    json_t* pluginsLoadedJ = json_object_get(modulesLoadedJ, "modules");
    const char* modulesKey;
    json_t* modulesJ;

    json_object_foreach(pluginsLoadedJ, modulesKey, modulesJ) {
        // Name of the plugin library, like Core, Fundamental, Befaco etc.
        std::string pluginSlug = modulesKey;

        // Get the manifest for this plugin from VCV Rack server
        json_t* manifestFromVcvRackJ =
            json_object_get(manifestListFromVcvRackJ, pluginSlug.c_str());

        // If no manifest, skip plugin
        if (!manifestFromVcvRackJ) {
            continue;
        }

        // Don't replace existing UpdateInfo, even if version is newer.
        // This keeps things sane and ensures that only one version of each
        // plugin is downloaded to `plugins/` at a time.
        auto it = updateInfos_.find(pluginSlug);
        if (it != updateInfos_.end()) {
            continue;
        }

        // Info about a plugin that is to be updated
        UpdateInfo update;

        // Get name of the plugin from VCV Rack manifest
        json_t* nameJ = json_object_get(manifestFromVcvRackJ, "name");
        if (nameJ) update.name = json_string_value(nameJ);

        // Get version of the plugin from VCV Rack manifest
        json_t* versionJ = json_object_get(manifestFromVcvRackJ, "version");
        if (!versionJ) {
            WARN(
                "Loaded plugin %s has no version in manifest so cannot "
                "determine if can update it.",
                pluginSlug.c_str());
            continue;
        }
        update.version = json_string_value(versionJ);

        // Reject plugins with ABI mismatch
        if (!string::startsWith(update.version, APP_VERSION_MAJOR + ".")) {
            continue;
        }

        // For the plugin already loaded, check whether update is needed.
        // If the plugin was not actually successfully loaded, skip it.
        // Update is needed if the available plugin version is newer
        // than the loaded plugin version.
        plugin::Plugin* alreadyLoadedPlugin = plugin::getPlugin(pluginSlug);
        if (!alreadyLoadedPlugin ||
            update.version == alreadyLoadedPlugin->version ||
            string::Version(update.version) <
                string::Version(alreadyLoadedPlugin->version))
            continue;

        // Check that plugin is available for this arch
        json_t* archesJ = json_object_get(manifestFromVcvRackJ, "arches");
        if (!archesJ) continue;
        std::string arch = APP_OS + "-" + APP_CPU;
        json_t* archJ = json_object_get(archesJ, arch.c_str());
        if (!json_boolean_value(archJ)) continue;

        // Get changelog URL
        json_t* changelogUrlJ =
            json_object_get(manifestFromVcvRackJ, "changelogUrl");
        if (changelogUrlJ)
            update.changelogUrl = json_string_value(changelogUrlJ);

        // Get minRackVersion
        json_t* minRackVersionJ =
            json_object_get(manifestFromVcvRackJ, "minRackVersion");
        if (minRackVersionJ) {
            std::string minRackVersion = json_string_value(minRackVersionJ);
            // Check that Rack version is at least minRackVersion
            if (string::Version(APP_VERSION) <
                string::Version(minRackVersion)) {
                update.minRackVersion = minRackVersion;
            }
        }

        // Add update to updates map
        updateInfos_[pluginSlug] = update;
    }

    // Merge module whitelist
    {
        // Clone plugin slugs from settings to temporary whitelist.
        // This makes existing plugins entirely hidden if removed from user's
        // VCV account.
        std::map<std::string, settings::PluginWhitelist> moduleWhitelist;
        for (const auto& pluginPair : settings::moduleWhitelist) {
            std::string pluginSlug = pluginPair.first;
            moduleWhitelist[pluginSlug] = settings::PluginWhitelist();
        }

        // Iterate plugins
        const char* modulesKey;
        json_t* modulesJ;
        json_object_foreach(pluginsLoadedJ, modulesKey, modulesJ) {
            std::string pluginSlug = modulesKey;
            settings::PluginWhitelist& pw = moduleWhitelist[pluginSlug];

            // If value is "true", plugin is subscribed
            if (json_is_true(modulesJ)) {
                pw.subscribed = true;
                continue;
            }

            // Iterate modules in plugin
            size_t moduleIndex;
            json_t* moduleSlugJ;
            json_array_foreach(modulesJ, moduleIndex, moduleSlugJ) {
                std::string moduleSlug = json_string_value(moduleSlugJ);
                // Insert module in whitelist
                pw.moduleSlugs.insert(moduleSlug);
            }
        }

        settings::moduleWhitelist = moduleWhitelist;
    }

    updateStatus_ = "";
    refreshRequested_ = true;
}

bool hasUpdates() {
    for (auto& pair : updateInfos_) {
        if (!pair.second.downloaded) return true;
    }
    return false;
}

void syncUpdate(std::string slug) {
	if (!updateMutex_.try_lock())
		return;
	DEFER({updateMutex_.unlock();});

	if (settings::token.empty())
		return;

	isSyncing_ = true;
	DEFER({isSyncing_ = false;});

	// Get the UpdateInfo object
	auto it = updateInfos_.find(slug);
	if (it == updateInfos_.end())
		return;
	UpdateInfo update = it->second;

	// Don't update if not compatible with Rack version
	if (update.minRackVersion != "")
		return;

	updateSlug_ = slug;
	DEFER({updateSlug_ = "";});

	// Set progress to 0%
	updateProgress_ = 0.f;
	DEFER({updateProgress_ = 0.f;});

	INFO("Downloading plugin %s v%s for %s-%s", slug.c_str(), update.version.c_str(), APP_OS.c_str(), APP_CPU.c_str());

	// Get download URL
	std::string downloadUrl = API_URL + "/download";
	downloadUrl += "?slug=" + network::encodeUrl(slug);
	downloadUrl += "&version=" + network::encodeUrl(update.version);
	downloadUrl += "&arch=" + network::encodeUrl(APP_OS + "-" + APP_CPU);

	// Get file path
	std::string packageFilename = slug + "-" + update.version + "-" + APP_OS + "-" + APP_CPU + ".vcvplugin";
	std::string packagePath = system::join(plugin::pluginsPath, packageFilename);

	// Download plugin package
	if (!network::requestDownload(downloadUrl, packagePath, &updateProgress_, getTokenCookies())) {
		WARN("Plugin %s download was unsuccessful", slug.c_str());
		return;
	}

	// updateInfos could possibly change in the checkUpdates() thread, so re-get the UpdateInfo to modify it.
	it = updateInfos_.find(slug);
	if (it == updateInfos_.end())
		return;
	it->second.downloaded = true;
}


void syncUpdates() {
	if (settings::token.empty())
		return;

	// updateInfos could possibly change in the checkUpdates() thread, but checkUpdates() will not execute if syncUpdate() is running, so the chance of the updateInfos map being modified while iterating is rare.
	auto updateInfosClone = updateInfos_;
	for (auto& pair : updateInfosClone) {
		syncUpdate(pair.first);
	}
	restartRequested_ = true;
}

bool isSyncing() {
    return isSyncing_;
}


bool isRestartRequested() {
    return restartRequested_;
}

void clearRestartRequest() {
    restartRequested_ = false;
}

std::string getLoginStatus() {
    return loginStatus_;
}

std::string getUpdateStatus() {
    return updateStatus_;
}

std::string getAppVersion() {
    return appVersion_;
}

std::string getAppDownloadUrl() {
    return appDownloadUrl_;
}

std::string getAppChangelogUrl() {
    return appChangelogUrl_;
}

std::map<std::string, UpdateInfo>& getUpdateInfos() {
    return updateInfos_;
}

float getUpdateProgress() {
    return updateProgress_;
}

std::string getUpdateSlug() {
    return updateSlug_;
}


} // namespace library
} // namespace rack
