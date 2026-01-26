#include "PluginScanCoordinator.hpp"

#include <iostream>

namespace magda {

PluginScanCoordinator::PluginScanCoordinator() {
    loadBlacklist();
}

PluginScanCoordinator::~PluginScanCoordinator() {
    // Invalidate any pending async callbacks
    validFlag_->store(false);

    // Ensure we don't process any callbacks during destruction
    isScanning_ = false;
    isRecovering_ = false;
    recoverySequence_++;

    // Stop timer first
    stopTimer();

    // Give the worker process a moment to terminate gracefully
    // Don't call killWorkerProcess() if we already finished - it may cause thread issues
    // The ChildProcessCoordinator base class destructor will handle cleanup
}

juce::File PluginScanCoordinator::getScannerExecutable() const {
    // On macOS, the scanner should be in the app bundle
    auto appBundle = juce::File::getSpecialLocation(juce::File::currentApplicationFile);

#if JUCE_MAC
    // Look in the app bundle's MacOS folder
    auto scanner = appBundle.getChildFile("Contents/MacOS/magda_plugin_scanner");
    if (scanner.existsAsFile()) {
        return scanner;
    }

    // Fallback: look next to the executable (for debug builds)
    scanner = appBundle.getParentDirectory().getChildFile("magda_plugin_scanner");
    if (scanner.existsAsFile()) {
        return scanner;
    }
#elif JUCE_WINDOWS
    auto scanner = appBundle.getParentDirectory().getChildFile("magda_plugin_scanner.exe");
    if (scanner.existsAsFile()) {
        return scanner;
    }
#else
    auto scanner = appBundle.getParentDirectory().getChildFile("magda_plugin_scanner");
    if (scanner.existsAsFile()) {
        return scanner;
    }
#endif

    std::cerr << "[ScanCoordinator] Scanner executable not found!" << std::endl;
    std::cerr << "[ScanCoordinator] Searched: " << appBundle.getFullPathName() << std::endl;
    return {};
}

bool PluginScanCoordinator::launchScannerProcess() {
    auto scannerExe = getScannerExecutable();

    if (!scannerExe.existsAsFile()) {
        std::cerr << "[ScanCoordinator] Plugin scanner executable not found" << std::endl;
        return false;
    }

    std::cout << "[ScanCoordinator] Launching scanner: " << scannerExe.getFullPathName()
              << std::endl;

    // Launch the scanner process
    // The ChildProcessCoordinator::launchWorkerProcess takes:
    // - File: the executable to launch
    // - String: a unique ID for this type of worker
    // - int: timeout in milliseconds (0 = no timeout)
    // - int: pipe timeout in milliseconds
    // Use 10 second timeout to avoid blocking the message thread indefinitely
    if (!launchWorkerProcess(scannerExe, "magda-plugin-scanner", 10000, 5000)) {
        std::cerr << "[ScanCoordinator] Failed to launch scanner process" << std::endl;
        return false;
    }

    std::cout << "[ScanCoordinator] Scanner process launched successfully" << std::endl;
    return true;
}

void PluginScanCoordinator::startScan(juce::AudioPluginFormatManager& formatManager,
                                      ProgressCallback progressCallback,
                                      CompletionCallback completionCallback) {
    if (isScanning_) {
        std::cout << "[ScanCoordinator] Scan already in progress" << std::endl;
        return;
    }

    formatManager_ = &formatManager;
    progressCallback_ = progressCallback;
    completionCallback_ = completionCallback;
    foundPlugins_.clear();
    failedPlugins_.clear();
    currentFormatIndex_ = 0;
    formatsToScan_.clear();
    isRecovering_ = false;
    consecutiveFailures_ = 0;
    recoverySequence_++;  // Invalidate any stale recovery callbacks
    currentPluginBeingScanned_.clear();

    // Collect formats to scan
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto* format = formatManager.getFormat(i);
        if (format) {
            juce::String formatName = format->getName();
            // Only scan VST3 and AudioUnit
            if (formatName.containsIgnoreCase("VST3") ||
                formatName.containsIgnoreCase("AudioUnit")) {
                formatsToScan_.add(formatName);
            }
        }
    }

    if (formatsToScan_.isEmpty()) {
        std::cout << "[ScanCoordinator] No scannable formats found" << std::endl;
        if (completionCallback) {
            completionCallback(true, foundPlugins_, failedPlugins_);
        }
        return;
    }

    std::cout << "[ScanCoordinator] Starting scan for formats: "
              << formatsToScan_.joinIntoString(", ") << std::endl;

    isScanning_ = true;

    // Launch the scanner process
    if (!launchScannerProcess()) {
        std::cerr << "[ScanCoordinator] Failed to launch scanner" << std::endl;
        finishScan(false);
        return;
    }

    // Start scanning the first format
    scanNextFormat();

    // Start timeout timer
    lastProgressTime_ = juce::Time::currentTimeMillis();
    startTimer(1000);  // Check every second
}

void PluginScanCoordinator::scanNextFormat() {
    if (currentFormatIndex_ >= formatsToScan_.size()) {
        std::cout << "[ScanCoordinator] All formats scanned" << std::endl;
        finishScan(true);
        return;
    }

    // Clear current plugin state (but NOT consecutiveFailures - that tracks across retries)
    currentPluginBeingScanned_.clear();

    juce::String formatName = formatsToScan_[currentFormatIndex_];
    std::cout << "[ScanCoordinator] Scanning format: " << formatName << std::endl;

    // Find the format
    juce::AudioPluginFormat* format = nullptr;
    for (int i = 0; i < formatManager_->getNumFormats(); ++i) {
        if (formatManager_->getFormat(i)->getName() == formatName) {
            format = formatManager_->getFormat(i);
            break;
        }
    }

    if (!format) {
        std::cerr << "[ScanCoordinator] Format not found: " << formatName << std::endl;
        currentFormatIndex_++;
        scanNextFormat();
        return;
    }

    // Get search paths
    auto searchPath = format->getDefaultLocationsToSearch();
    juce::String searchPathStr;
    for (int i = 0; i < searchPath.getNumPaths(); ++i) {
        if (i > 0)
            searchPathStr += ";";
        searchPathStr += searchPath[i].getFullPathName();
    }

    // Send scan command to worker
    sendScanCommand(formatName, searchPathStr, blacklistedPlugins_);
}

void PluginScanCoordinator::sendScanCommand(const juce::String& formatName,
                                            const juce::String& searchPath,
                                            const juce::StringArray& blacklist) {
    // Clear current plugin state before sending new command
    currentPluginBeingScanned_.clear();

    juce::MemoryBlock msg;
    juce::MemoryOutputStream stream(msg, false);

    stream.writeString(ScannerIPC::MSG_SCAN_FORMAT);
    stream.writeString(formatName);
    stream.writeString(searchPath);
    stream.writeInt(blacklist.size());
    for (const auto& plugin : blacklist) {
        stream.writeString(plugin);
    }

    std::cout << "[ScanCoordinator] Sending scan command for format: " << formatName << " with "
              << blacklist.size() << " blacklisted plugins" << std::endl;

    sendMessageToWorker(msg);
    lastProgressTime_ = juce::Time::currentTimeMillis();
}

void PluginScanCoordinator::handleMessageFromWorker(const juce::MemoryBlock& message) {
    juce::MemoryInputStream stream(message, false);
    juce::String msgType = stream.readString();

    if (msgType == ScannerIPC::MSG_PROGRESS) {
        float progress = stream.readFloat();
        lastProgressTime_ = juce::Time::currentTimeMillis();
        consecutiveFailures_ = 0;  // Scanner is making progress

        if (progressCallback_) {
            // Calculate overall progress including format index
            float overallProgress = (currentFormatIndex_ + progress) / formatsToScan_.size();
            progressCallback_(overallProgress, currentPluginBeingScanned_);
        }
    } else if (msgType == ScannerIPC::MSG_CURRENT_FILE) {
        currentPluginBeingScanned_ = stream.readString();
        lastProgressTime_ = juce::Time::currentTimeMillis();
        consecutiveFailures_ = 0;  // Scanner is making progress
        std::cout << "[ScanCoordinator] Scanning: " << currentPluginBeingScanned_ << std::endl;

        if (progressCallback_) {
            float overallProgress = static_cast<float>(currentFormatIndex_) / formatsToScan_.size();
            progressCallback_(overallProgress, currentPluginBeingScanned_);
        }
    } else if (msgType == ScannerIPC::MSG_PLUGIN_FOUND) {
        juce::PluginDescription desc;
        desc.name = stream.readString();
        desc.pluginFormatName = stream.readString();
        desc.manufacturerName = stream.readString();
        desc.version = stream.readString();
        desc.fileOrIdentifier = stream.readString();
        desc.uniqueId = stream.readInt();
        desc.isInstrument = stream.readBool();
        desc.category = stream.readString();

        foundPlugins_.add(desc);
        consecutiveFailures_ = 0;  // Successfully found a plugin, reset failure count
        std::cout << "[ScanCoordinator] Found: " << desc.name << " (" << desc.pluginFormatName
                  << ")" << std::endl;
    } else if (msgType == ScannerIPC::MSG_ERROR) {
        juce::String plugin = stream.readString();
        juce::String error = stream.readString();

        if (plugin.isNotEmpty()) {
            failedPlugins_.add(plugin);
            blacklistPlugin(plugin);
            std::cout << "[ScanCoordinator] Failed: " << plugin << " - " << error << std::endl;
        } else {
            std::cerr << "[ScanCoordinator] Error: " << error << std::endl;
        }
    } else if (msgType == ScannerIPC::MSG_SCAN_COMPLETE) {
        std::cout << "[ScanCoordinator] Format scan complete" << std::endl;
        currentFormatIndex_++;
        scanNextFormat();
    }
}

void PluginScanCoordinator::handleConnectionLost() {
    std::cout << "[ScanCoordinator] Connection to scanner lost" << std::endl;

    if (!isScanning_) {
        std::cout << "[ScanCoordinator] Not scanning, ignoring connection lost" << std::endl;
        return;
    }

    // Prevent multiple simultaneous recovery attempts
    if (isRecovering_) {
        std::cout << "[ScanCoordinator] Already recovering, ignoring duplicate connection lost"
                  << std::endl;
        return;
    }

    isRecovering_ = true;

    // Try to read the dead man's pedal file to identify the crashing plugin
    // JUCE's PluginDirectoryScanner writes the current plugin path to this file
    if (currentPluginBeingScanned_.isEmpty() && currentFormatIndex_ < formatsToScan_.size()) {
        juce::String formatName = formatsToScan_[currentFormatIndex_];
        juce::File deadMansPedal =
            juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("magda_scanner_current_" + formatName + ".txt");

        if (deadMansPedal.existsAsFile()) {
            // The file may be written in wide char format (UTF-16), try to read it
            juce::String content = deadMansPedal.loadFileAsString();
            if (content.isEmpty()) {
                // Try reading as raw data and converting from UTF-16
                juce::MemoryBlock data;
                if (deadMansPedal.loadFileAsData(data) && data.getSize() > 0) {
                    // Check for UTF-16 BOM or null bytes indicating wide chars
                    const char* rawData = static_cast<const char*>(data.getData());
                    bool isWideChar = false;
                    for (size_t i = 1; i < data.getSize() && i < 20; i += 2) {
                        if (rawData[i] == 0) {
                            isWideChar = true;
                            break;
                        }
                    }

                    if (isWideChar) {
                        // Convert from UTF-16LE
                        content = juce::String(juce::CharPointer_UTF16(
                            reinterpret_cast<const juce::CharPointer_UTF16::CharType*>(rawData)));
                    }
                }
            }

            content = content.trim();
            if (content.isNotEmpty()) {
                std::cout << "[ScanCoordinator] Dead man's pedal indicates crashing plugin: "
                          << content << std::endl;
                currentPluginBeingScanned_ = content;

                // Clear the dead man's pedal file to prevent re-reading stale data
                deadMansPedal.deleteFile();
            }
        }
    }

    // The scanner crashed - blacklist the current plugin ONLY if we have a valid one
    // (i.e., we were in the middle of scanning a specific plugin)
    if (currentPluginBeingScanned_.isNotEmpty()) {
        std::cout << "[ScanCoordinator] Blacklisting crashed plugin: " << currentPluginBeingScanned_
                  << std::endl;
        blacklistPlugin(currentPluginBeingScanned_);
        failedPlugins_.add(currentPluginBeingScanned_);
        currentPluginBeingScanned_.clear();
        consecutiveFailures_ = 0;  // Reset if we actually found a crashing plugin
    } else {
        // No plugin was being scanned - the scanner crashed during initialization
        // or between plugins. Increment failure counter.
        consecutiveFailures_++;
        std::cout << "[ScanCoordinator] Scanner crashed without identifying plugin (failure "
                  << consecutiveFailures_ << "/" << MAX_CONSECUTIVE_FAILURES << ")" << std::endl;
    }

    // Check if we've had too many consecutive failures without identifying a plugin
    if (consecutiveFailures_ >= MAX_CONSECUTIVE_FAILURES) {
        std::cout << "[ScanCoordinator] Too many consecutive failures, moving to next format"
                  << std::endl;
        consecutiveFailures_ = 0;
        currentFormatIndex_++;

        if (currentFormatIndex_ >= formatsToScan_.size()) {
            std::cout << "[ScanCoordinator] No more formats to scan, finishing" << std::endl;
            isRecovering_ = false;
            finishScan(foundPlugins_.size() > 0);
            return;
        }
    }

    // Report the crash to the progress callback
    if (progressCallback_) {
        progressCallback_(static_cast<float>(currentFormatIndex_) / formatsToScan_.size(),
                          "Scanner crashed, restarting...");
    }

    // Capture current sequence to detect if scan was aborted/restarted during delay
    int currentSequence = recoverySequence_;

    // Capture validity flag to safely check if object still exists
    auto validFlag = validFlag_;

    // Use a delayed callback to give the crashed process time to fully terminate
    juce::Timer::callAfterDelay(RECOVERY_DELAY_MS, [this, currentSequence, validFlag]() {
        // Check if this object was destroyed
        if (!validFlag->load()) {
            return;
        }

        // Check if this callback is stale (scan was aborted or restarted)
        if (currentSequence != recoverySequence_) {
            std::cout << "[ScanCoordinator] Stale recovery callback ignored" << std::endl;
            return;
        }

        isRecovering_ = false;

        if (!isScanning_) {
            std::cout << "[ScanCoordinator] Scan was aborted, not relaunching" << std::endl;
            return;
        }

        if (launchScannerProcess()) {
            std::cout << "[ScanCoordinator] Relaunched scanner, continuing scan" << std::endl;
            lastProgressTime_ = juce::Time::currentTimeMillis();
            scanNextFormat();
        } else {
            std::cout << "[ScanCoordinator] Failed to relaunch scanner, trying next format"
                      << std::endl;
            currentFormatIndex_++;
            if (currentFormatIndex_ < formatsToScan_.size() && launchScannerProcess()) {
                lastProgressTime_ = juce::Time::currentTimeMillis();
                scanNextFormat();
            } else {
                std::cout << "[ScanCoordinator] Cannot continue, finishing scan" << std::endl;
                finishScan(foundPlugins_.size() > 0);
            }
        }
    });
}

void PluginScanCoordinator::timerCallback() {
    if (!isScanning_) {
        stopTimer();
        return;
    }

    // Check for timeout
    juce::int64 elapsed = juce::Time::currentTimeMillis() - lastProgressTime_;
    if (elapsed > PLUGIN_TIMEOUT_MS) {
        std::cout << "[ScanCoordinator] Plugin scan timeout on: " << currentPluginBeingScanned_
                  << std::endl;

        // Blacklist the stuck plugin
        if (currentPluginBeingScanned_.isNotEmpty()) {
            blacklistPlugin(currentPluginBeingScanned_);
            failedPlugins_.add(currentPluginBeingScanned_);
        }

        // Kill the scanner - this will trigger handleConnectionLost which will restart
        killWorkerProcess();
        currentPluginBeingScanned_.clear();

        // handleConnectionLost will be called and will handle the restart
    }
}

void PluginScanCoordinator::abortScan() {
    // Set isScanning_ to false BEFORE killing the worker process
    // to prevent handleConnectionLost from trying to recover
    isScanning_ = false;
    isRecovering_ = false;
    recoverySequence_++;  // Invalidate any pending recovery callbacks

    stopTimer();
    killWorkerProcess();

    consecutiveFailures_ = 0;
    currentPluginBeingScanned_.clear();
}

void PluginScanCoordinator::finishScan(bool success) {
    std::cout << "[ScanCoordinator] finishScan called, success=" << success << std::endl;

    // IMPORTANT: Set isScanning_ to false BEFORE any cleanup
    // This ensures handleConnectionLost() will be ignored
    isScanning_ = false;
    isRecovering_ = false;

    // Invalidate callbacks to prevent any pending ones from interfering
    recoverySequence_++;

    // Stop timer
    stopTimer();

    // Send QUIT message to scanner so it exits gracefully
    // This is better than killWorkerProcess() which can cause thread cleanup issues
    juce::MemoryBlock quitMsg;
    juce::MemoryOutputStream quitStream(quitMsg, false);
    quitStream.writeString(ScannerIPC::MSG_QUIT);
    sendMessageToWorker(quitMsg);

    // Clear remaining state
    consecutiveFailures_ = 0;
    currentPluginBeingScanned_.clear();

    std::cout << "[ScanCoordinator] Scan finished. Found " << foundPlugins_.size() << " plugins, "
              << failedPlugins_.size() << " failed." << std::endl;

    // Copy callback to local to avoid issues if callback modifies our state
    auto callback = completionCallback_;
    completionCallback_ = nullptr;

    if (callback) {
        callback(success, foundPlugins_, failedPlugins_);
    }
}

// Blacklist management
juce::File PluginScanCoordinator::getBlacklistFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("MAGDA")
        .getChildFile("plugin_blacklist.txt");
}

juce::StringArray PluginScanCoordinator::getBlacklistedPlugins() const {
    return blacklistedPlugins_;
}

void PluginScanCoordinator::clearBlacklist() {
    blacklistedPlugins_.clear();
    saveBlacklist();
}

void PluginScanCoordinator::blacklistPlugin(const juce::String& pluginPath) {
    if (!blacklistedPlugins_.contains(pluginPath)) {
        blacklistedPlugins_.add(pluginPath);
        saveBlacklist();
    }
}

void PluginScanCoordinator::loadBlacklist() {
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
        std::cout << "[ScanCoordinator] Loaded " << blacklistedPlugins_.size()
                  << " blacklisted plugins" << std::endl;
    }
}

void PluginScanCoordinator::saveBlacklist() {
    auto file = getBlacklistFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(blacklistedPlugins_.joinIntoString("\n"));
}

}  // namespace magda
