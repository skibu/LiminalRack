#pragma once
#include <common.hpp>

#include <map>


namespace rack {
/** Synchronizes plugins with the VCV Library and handles VCV accounts with the
 * vcvrack.com API. And manages plugin updates.
 */
namespace library {


struct UpdateInfo {
	std::string name;
	std::string version;
	std::string changelogUrl;
	/** Only defined if plugin does not meet Rack version requirement */
	std::string minRackVersion;
	bool downloaded = false;
};

/** Checks to see if any of the plugin libraries need to be updated.
 * If any do, the user can then choose to download them. Uses a separate
 * thread so as to not block the main thread, especially since it can take
 * a while to check all the plugins for updates. Waits a few seconds before
 * checking to allow the application in case library is destroyed immediately
 * afterwards.
 */
PRIVATE void init();

/** Destroys all loaded plugins. */
PRIVATE void destroy();

/** Checks to see there is an update to the Rack application.
 * If there is then appVersion, appDownloadUrl, and appChangelogUrl
 * will be set appropriately. And isAppUpdateAvailable() can be used
 * to check if an update is available.
 */
PRIVATE void checkAppUpdate();

/** Returns true if there is an update available for the Rack application */
bool isAppUpdateAvailable();

/** Is the user logged in to VCV Rack */
bool isLoggedIn();

/** Logs the user in to VCV Rack */
PRIVATE void logIn(std::string email, std::string password);

/** Logs the user out of VCV Rack */
PRIVATE void logOut();

/** Checks to see if updates are available for the user's plugins.
 * Can then call hasUpdates() to see if any updates were found. 
 */
PRIVATE void checkUpdates();

/** Returns true if there are any plugin updates available and downloaded. */
PRIVATE bool hasUpdates();

/** Downloads and installs the update for the specified plugin slug. */
PRIVATE void syncUpdate(std::string slug);

/** Downloads and installs all available plugin updates. */
PRIVATE void syncUpdates();

PRIVATE bool isSyncing();

PRIVATE bool isRestartRequested();
PRIVATE void clearRestartRequest();

PRIVATE std::string getLoginStatus();
PRIVATE std::string getUpdateStatus();
PRIVATE std::string getAppVersion();
PRIVATE std::string getAppDownloadUrl();
PRIVATE std::string getAppChangelogUrl();
PRIVATE std::map<std::string, UpdateInfo>& getUpdateInfos();
PRIVATE float getUpdateProgress();
PRIVATE std::string getUpdateSlug();


} // namespace library
} // namespace rack
