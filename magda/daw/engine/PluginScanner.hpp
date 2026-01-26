#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <functional>

namespace magda {

/**
 * @brief Thread-based plugin scanner
 *
 * Scans plugins on a background thread to avoid blocking the UI.
 */
class PluginScanner : private juce::Thread {
  public:
    PluginScanner();
    ~PluginScanner() override;

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
     * @param failedPlugins List of plugins that failed during scan
     */
    using CompletionCallback =
        std::function<void(bool success, const juce::Array<juce::PluginDescription>& plugins,
                           const juce::StringArray& failedPlugins)>;

    /**
     * @brief Start scanning for plugins
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
        return isThreadRunning();
    }

    /**
     * @brief Get list of plugins that failed during scanning
     * These are persisted and will be skipped on future scans
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
    void run() override;

    juce::AudioPluginFormatManager* formatManager_ = nullptr;
    ProgressCallback progressCallback_;
    CompletionCallback completionCallback_;

    // Results (filled by background thread)
    juce::Array<juce::PluginDescription> foundPlugins_;
    juce::StringArray failedPlugins_;

    // Blacklist management
    juce::File getBlacklistFile() const;
    void loadBlacklist();
    void saveBlacklist();
    juce::StringArray blacklistedPlugins_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScanner)
};

}  // namespace magda
