#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <functional>

namespace magda {

/**
 * @brief IPC message types for plugin scanner communication
 */
namespace ScannerIPC {
constexpr const char* MSG_SCAN_FORMAT = "SCAN";
constexpr const char* MSG_PROGRESS = "PROG";
constexpr const char* MSG_PLUGIN_FOUND = "PLUG";
constexpr const char* MSG_SCAN_COMPLETE = "DONE";
constexpr const char* MSG_ERROR = "ERR";
constexpr const char* MSG_CURRENT_FILE = "FILE";
constexpr const char* MSG_QUIT = "QUIT";
}  // namespace ScannerIPC

/**
 * @brief Coordinates out-of-process plugin scanning
 *
 * Uses JUCE's ChildProcessCoordinator to launch the magda_plugin_scanner
 * executable and communicate with it via IPC. If the scanner crashes on
 * a problematic plugin, only the subprocess dies - the main app continues.
 */
class PluginScanCoordinator : private juce::ChildProcessCoordinator, private juce::Timer {
  public:
    PluginScanCoordinator();
    ~PluginScanCoordinator() override;

    /**
     * @brief Progress callback
     * @param progress 0.0-1.0 progress
     * @param currentPlugin Name of plugin currently being scanned
     */
    using ProgressCallback = std::function<void(float progress, const juce::String& currentPlugin)>;

    /**
     * @brief Completion callback
     * @param success True if scan completed without errors
     * @param plugins List of successfully scanned plugin descriptions
     * @param failedPlugins List of plugins that failed/crashed during scan
     */
    using CompletionCallback =
        std::function<void(bool success, const juce::Array<juce::PluginDescription>& plugins,
                           const juce::StringArray& failedPlugins)>;

    /**
     * @brief Start scanning for plugins using out-of-process scanner
     * @param formatManager The format manager with registered formats
     * @param progressCallback Called with progress updates (on message thread)
     * @param completionCallback Called when scan completes (on message thread)
     */
    void startScan(juce::AudioPluginFormatManager& formatManager,
                   const ProgressCallback& progressCallback,
                   const CompletionCallback& completionCallback);

    /**
     * @brief Abort the current scan
     */
    void abortScan();

    /**
     * @brief Check if a scan is in progress
     */
    bool isScanning() const {
        return isScanning_;
    }

    /**
     * @brief Get the list of plugins found during scanning
     */
    const juce::Array<juce::PluginDescription>& getFoundPlugins() const {
        return foundPlugins_;
    }

    /**
     * @brief Get list of plugins that failed during scanning
     */
    juce::StringArray getBlacklistedPlugins() const;

    /**
     * @brief Clear the blacklist to retry problematic plugins
     */
    void clearBlacklist();

    /**
     * @brief Add a plugin to the blacklist manually
     */
    void blacklistPlugin(const juce::String& pluginPath);

  private:
    // ChildProcessCoordinator overrides
    void handleMessageFromWorker(const juce::MemoryBlock& message) override;
    void handleConnectionLost() override;

    // Timer for timeout handling
    void timerCallback() override;

    // Internal scanning methods
    bool launchScannerProcess();
    void scanNextFormat();
    void sendScanCommand(const juce::String& formatName, const juce::String& searchPath,
                         const juce::StringArray& blacklist);
    void finishScan(bool success);

    // Find the scanner executable
    juce::File getScannerExecutable() const;

    // Blacklist management
    juce::File getBlacklistFile() const;
    void loadBlacklist();
    void saveBlacklist();

    // State
    bool isScanning_ = false;
    juce::AudioPluginFormatManager* formatManager_ = nullptr;
    ProgressCallback progressCallback_;
    CompletionCallback completionCallback_;

    // Scanning state
    int currentFormatIndex_ = 0;
    juce::String currentPluginBeingScanned_;
    juce::StringArray formatsToScan_;

    // Results
    juce::Array<juce::PluginDescription> foundPlugins_;
    juce::StringArray failedPlugins_;
    juce::StringArray blacklistedPlugins_;

    // Timeout tracking
    static constexpr int PLUGIN_TIMEOUT_MS = 30000;  // 30 seconds per plugin
    juce::int64 lastProgressTime_ = 0;

    // Recovery state to prevent multiple simultaneous recovery attempts
    bool isRecovering_ = false;
    int consecutiveFailures_ = 0;
    int recoverySequence_ = 0;  // Incremented each scan, used to invalidate stale callbacks
    static constexpr int MAX_CONSECUTIVE_FAILURES =
        3;  // Reduced - fail faster to avoid system strain
    static constexpr int RECOVERY_DELAY_MS =
        1000;  // Delay before relaunching after crash (reduced from 2000)

    // Validity flag for async callbacks - set to false in destructor
    std::shared_ptr<std::atomic<bool>> validFlag_ = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScanCoordinator)
};

}  // namespace magda
