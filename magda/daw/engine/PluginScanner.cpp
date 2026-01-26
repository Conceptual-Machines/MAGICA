#include "PluginScanner.hpp"

#include <iostream>

namespace magda {

PluginScanner::PluginScanner() : juce::Thread("Plugin Scanner") {
    loadBlacklist();
}

PluginScanner::~PluginScanner() {
    abortScan();
}

void PluginScanner::startScan(juce::AudioPluginFormatManager& formatManager,
                              ProgressCallback progressCallback,
                              CompletionCallback completionCallback) {
    if (isThreadRunning()) {
        std::cout << "Scan already in progress" << std::endl;
        return;
    }

    formatManager_ = &formatManager;
    progressCallback_ = progressCallback;
    completionCallback_ = completionCallback;
    foundPlugins_.clear();
    failedPlugins_.clear();

    startThread();
}

void PluginScanner::abortScan() {
    signalThreadShouldExit();
    stopThread(5000);
}

void PluginScanner::run() {
    std::cout << "Plugin scan started on background thread" << std::endl;

    if (!formatManager_) {
        std::cerr << "No format manager set" << std::endl;
        return;
    }

    auto blacklist = getBlacklistedPlugins();
    juce::KnownPluginList tempKnownList;

    // Scan each format
    for (int i = 0; i < formatManager_->getNumFormats() && !threadShouldExit(); ++i) {
        auto* format = formatManager_->getFormat(i);
        if (!format)
            continue;

        juce::String formatName = format->getName();

        // Only scan VST3 and AudioUnit
        if (!formatName.containsIgnoreCase("VST3") && !formatName.containsIgnoreCase("AudioUnit")) {
            continue;
        }

        std::cout << "Scanning format: " << formatName << std::endl;

        // Report starting this format
        if (progressCallback_) {
            juce::MessageManager::callAsync([this, formatName]() {
                if (progressCallback_) {
                    progressCallback_(0.0f, "Starting " + formatName + " scan...");
                }
            });
        }

        auto searchPath = format->getDefaultLocationsToSearch();

        // Dead man's pedal - if we crash, this file tells us which plugin was being scanned
        juce::File deadMansPedal =
            juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("MAGDA")
                .getChildFile("scanning_" + formatName + ".txt");
        deadMansPedal.getParentDirectory().createDirectory();

        // Check if there's a dead man's pedal from a previous crash
        if (deadMansPedal.existsAsFile()) {
            juce::String crashedPlugin = deadMansPedal.loadFileAsString().trim();
            if (crashedPlugin.isNotEmpty() && !blacklist.contains(crashedPlugin)) {
                std::cout << "Previous crash detected on: " << crashedPlugin << std::endl;
                blacklistPlugin(crashedPlugin);
                blacklist.add(crashedPlugin);
            }
        }

        tempKnownList.clear();

        juce::PluginDirectoryScanner scanner(tempKnownList, *format, searchPath, true,
                                             deadMansPedal, false);

        juce::String nextPlugin;
        int scanned = 0;

        while (scanner.scanNextFile(true, nextPlugin) && !threadShouldExit()) {
            // Skip blacklisted plugins
            if (blacklist.contains(nextPlugin)) {
                std::cout << "Skipping blacklisted: " << nextPlugin << std::endl;
                continue;
            }

            scanned++;
            float progress = scanner.getProgress();

            // Report progress on message thread
            if (progressCallback_) {
                juce::MessageManager::callAsync([this, progress, nextPlugin]() {
                    if (progressCallback_) {
                        progressCallback_(progress, nextPlugin);
                    }
                });
            }
        }

        std::cout << "Scanned " << scanned << " " << formatName << " plugins" << std::endl;

        // Copy found plugins to our list
        for (const auto& desc : tempKnownList.getTypes()) {
            foundPlugins_.add(desc);
            std::cout << "Found: " << desc.name << " (" << desc.pluginFormatName << ")"
                      << std::endl;
        }

        // Record failed plugins
        auto failed = scanner.getFailedFiles();
        for (const auto& failedFile : failed) {
            std::cout << "Failed: " << failedFile << std::endl;
            failedPlugins_.add(failedFile);
        }

        // Clean up dead man's pedal
        deadMansPedal.deleteFile();
    }

    if (threadShouldExit()) {
        std::cout << "Plugin scan aborted" << std::endl;
        return;
    }

    std::cout << "Plugin scan complete. Found " << foundPlugins_.size() << " plugins, "
              << failedPlugins_.size() << " failed." << std::endl;

    // Notify completion on message thread
    auto plugins = foundPlugins_;
    auto failed = failedPlugins_;
    auto callback = completionCallback_;

    juce::MessageManager::callAsync([callback, plugins, failed]() {
        if (callback) {
            callback(true, plugins, failed);
        }
    });
}

juce::StringArray PluginScanner::getBlacklistedPlugins() const {
    return blacklistedPlugins_;
}

void PluginScanner::clearBlacklist() {
    blacklistedPlugins_.clear();
    saveBlacklist();
}

void PluginScanner::blacklistPlugin(const juce::String& pluginPath) {
    if (!blacklistedPlugins_.contains(pluginPath)) {
        blacklistedPlugins_.add(pluginPath);
        saveBlacklist();
    }
}

juce::File PluginScanner::getBlacklistFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("MAGDA")
        .getChildFile("plugin_blacklist.txt");
}

void PluginScanner::loadBlacklist() {
    auto file = getBlacklistFile();
    if (file.existsAsFile()) {
        juce::StringArray lines;
        file.readLines(lines);
        for (const auto& line : lines) {
            auto trimmed = line.trim();
            if (trimmed.isNotEmpty()) {
                blacklistedPlugins_.add(trimmed);
            }
        }
        std::cout << "Loaded " << blacklistedPlugins_.size() << " blacklisted plugins" << std::endl;
    }
}

void PluginScanner::saveBlacklist() {
    auto file = getBlacklistFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(blacklistedPlugins_.joinIntoString("\n"));
}

}  // namespace magda
