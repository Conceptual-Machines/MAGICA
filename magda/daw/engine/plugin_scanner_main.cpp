/**
 * @file plugin_scanner_main.cpp
 * @brief Out-of-process plugin scanner executable
 *
 * This executable is launched by the main MAGDA application to scan
 * plugins in a separate process. If a plugin crashes during scanning,
 * only this process dies and the main app can recover gracefully.
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <fstream>
#include <iostream>

// Global log file for debugging - scanner stdout isn't visible when run as child process
static std::ofstream g_logFile;

static void initLog() {
    // Use /tmp for immediate accessibility - always writable
    g_logFile.open("/tmp/magda_scanner_debug.log", std::ios::out | std::ios::trunc);
}

static void log(const std::string& msg) {
    if (g_logFile.is_open()) {
        g_logFile << msg << std::endl;
        g_logFile.flush();
    }
    std::cout << msg << std::endl;
    std::cout.flush();
}

namespace ScannerIPC {
constexpr const char* MSG_SCAN_FORMAT = "SCAN";
constexpr const char* MSG_PROGRESS = "PROG";
constexpr const char* MSG_PLUGIN_FOUND = "PLUG";
constexpr const char* MSG_SCAN_COMPLETE = "DONE";
constexpr const char* MSG_ERROR = "ERR";
constexpr const char* MSG_CURRENT_FILE = "FILE";
constexpr const char* MSG_QUIT = "QUIT";
}  // namespace ScannerIPC

class PluginScannerWorker : public juce::ChildProcessWorker {
  public:
    PluginScannerWorker() {
        log("[Scanner] PluginScannerWorker constructor starting...");

        // Register plugin formats
#if JUCE_PLUGINHOST_VST3
        log("[Scanner] About to register VST3 format...");
        formatManager_.addFormat(std::make_unique<juce::VST3PluginFormat>());
        log("[Scanner] Registered VST3 format");
#endif
#if JUCE_PLUGINHOST_AU && JUCE_MAC
        log("[Scanner] About to register AudioUnit format...");
        formatManager_.addFormat(std::make_unique<juce::AudioUnitPluginFormat>());
        log("[Scanner] Registered AudioUnit format");
#endif
        log("[Scanner] PluginScannerWorker constructor complete");
    }

    void handleMessageFromCoordinator(const juce::MemoryBlock& message) override {
        try {
            log("[Scanner] Received message from coordinator");
            juce::MemoryInputStream stream(message, false);
            juce::String msgType = stream.readString();
            log("[Scanner] Message type: " + msgType.toStdString());

            if (msgType == ScannerIPC::MSG_QUIT) {
                log("[Scanner] Received QUIT message, exiting gracefully");
                juce::JUCEApplicationBase::quit();
                return;
            } else if (msgType == ScannerIPC::MSG_SCAN_FORMAT) {
                juce::String formatName = stream.readString();
                juce::String searchPathStr = stream.readString();

                // Read blacklist
                int blacklistSize = stream.readInt();
                juce::StringArray blacklist;
                for (int i = 0; i < blacklistSize; ++i) {
                    blacklist.add(stream.readString());
                }

                log("[Scanner] Scanning format: " + formatName.toStdString());
                log("[Scanner] Search path length: " + std::to_string(searchPathStr.length()));
                log("[Scanner] Blacklist size: " + std::to_string(blacklistSize));
                log("[Scanner] Calling scanFormat...");

                scanFormat(formatName, searchPathStr, blacklist);

                log("[Scanner] scanFormat returned, waiting for next message...");
            }
        } catch (const std::exception& e) {
            log(std::string("[Scanner] EXCEPTION: ") + e.what());
        } catch (...) {
            log("[Scanner] UNKNOWN EXCEPTION");
        }
    }

    void handleConnectionMade() override {
        log("[Scanner] Connected to main application");
    }

    void handleConnectionLost() override {
        log("[Scanner] Connection lost, exiting");
        juce::JUCEApplicationBase::quit();
    }

  private:
    juce::AudioPluginFormatManager formatManager_;
    juce::KnownPluginList knownList_;

    void scanFormat(const juce::String& formatName, const juce::String& searchPathStr,
                    const juce::StringArray& blacklist) {
        try {
            log("[Scanner] scanFormat() started for: " + formatName.toStdString());

            // Find the format
            log("[Scanner] Looking for format in " +
                std::to_string(formatManager_.getNumFormats()) + " registered formats");

            juce::AudioPluginFormat* format = nullptr;
            for (int i = 0; i < formatManager_.getNumFormats(); ++i) {
                auto* fmt = formatManager_.getFormat(i);
                log("[Scanner] Checking format " + std::to_string(i) + ": " +
                    (fmt ? fmt->getName().toStdString() : "null"));
                if (fmt && fmt->getName() == formatName) {
                    format = fmt;
                    break;
                }
            }

            if (!format) {
                log("[Scanner] Format not found: " + formatName.toStdString());
                sendError("", "Format not found: " + formatName);
                sendComplete();
                return;
            }

            log("[Scanner] Using format: " + format->getName().toStdString());

            juce::FileSearchPath searchPath(searchPathStr);
            log("[Scanner] Search path has " + std::to_string(searchPath.getNumPaths()) +
                " directories");

            for (int i = 0; i < searchPath.getNumPaths(); ++i) {
                log("[Scanner]   Path " + std::to_string(i) + ": " +
                    searchPath[i].getFullPathName().toStdString());
            }

            log("[Scanner] Creating dead mans pedal file...");

            // Use a dead mans pedal file to track current plugin
            juce::File deadMansPedal =
                juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("magda_scanner_current_" + formatName + ".txt");

            log("[Scanner] Dead mans pedal: " + deadMansPedal.getFullPathName().toStdString());

            knownList_.clear();
            log("[Scanner] Cleared known list, about to create PluginDirectoryScanner...");

            juce::PluginDirectoryScanner scanner(knownList_, *format, searchPath, true,
                                                 deadMansPedal, false);

            log("[Scanner] PluginDirectoryScanner created successfully!");
            juce::String nextPlugin;
            int scanned = 0;
            int skipped = 0;

            while (scanner.getNextPluginFileThatWillBeScanned().isNotEmpty()) {
                juce::String fileToScan = scanner.getNextPluginFileThatWillBeScanned();

                // Check blacklist BEFORE scanning
                if (blacklist.contains(fileToScan)) {
                    log("[Scanner] Skipping blacklisted: " + fileToScan.toStdString());
                    scanner.skipNextFile();
                    skipped++;
                    continue;
                }

                // Report current file BEFORE scanning (this gets sent to coordinator before
                // potential crash)
                sendCurrentFile(fileToScan);
                log("[Scanner] Scanning: " + fileToScan.toStdString());

                // Now actually scan the file
                if (!scanner.scanNextFile(true, nextPlugin)) {
                    break;  // No more files
                }

                // Report progress
                sendProgress(scanner.getProgress());
                scanned++;
            }

            log("[Scanner] Scanned " + std::to_string(scanned) + " plugins, skipped " +
                std::to_string(skipped));

            // Send all found plugins
            auto types = knownList_.getTypes();
            log("[Scanner] Found " + std::to_string(types.size()) + " valid plugins");

            for (const auto& desc : types) {
                sendPluginFound(desc);
            }

            // Report failed files
            auto failed = scanner.getFailedFiles();
            for (const auto& failedFile : failed) {
                log("[Scanner] Failed: " + failedFile.toStdString());
                sendError(failedFile, "Failed to scan");
            }

            log("[Scanner] Sending DONE message");
            sendComplete();
            log("[Scanner] DONE message sent, returning from scanFormat");
        } catch (const std::exception& e) {
            log(std::string("[Scanner] scanFormat EXCEPTION: ") + e.what());
            sendError("", juce::String("Exception: ") + e.what());
            sendComplete();
        } catch (...) {
            log("[Scanner] scanFormat UNKNOWN EXCEPTION");
            sendError("", "Unknown exception");
            sendComplete();
        }
    }

    void sendProgress(float progress) {
        juce::MemoryBlock msg;
        juce::MemoryOutputStream stream(msg, false);
        stream.writeString(ScannerIPC::MSG_PROGRESS);
        stream.writeFloat(progress);
        sendMessageToCoordinator(msg);
    }

    void sendCurrentFile(const juce::String& file) {
        juce::MemoryBlock msg;
        juce::MemoryOutputStream stream(msg, false);
        stream.writeString(ScannerIPC::MSG_CURRENT_FILE);
        stream.writeString(file);
        sendMessageToCoordinator(msg);
    }

    void sendPluginFound(const juce::PluginDescription& desc) {
        juce::MemoryBlock msg;
        juce::MemoryOutputStream stream(msg, false);
        stream.writeString(ScannerIPC::MSG_PLUGIN_FOUND);
        stream.writeString(desc.name);
        stream.writeString(desc.pluginFormatName);
        stream.writeString(desc.manufacturerName);
        stream.writeString(desc.version);
        stream.writeString(desc.fileOrIdentifier);
        stream.writeInt(desc.uniqueId);
        stream.writeBool(desc.isInstrument);
        stream.writeString(desc.category);
        sendMessageToCoordinator(msg);
    }

    void sendError(const juce::String& plugin, const juce::String& error) {
        juce::MemoryBlock msg;
        juce::MemoryOutputStream stream(msg, false);
        stream.writeString(ScannerIPC::MSG_ERROR);
        stream.writeString(plugin);
        stream.writeString(error);
        sendMessageToCoordinator(msg);
    }

    void sendComplete() {
        juce::MemoryBlock msg;
        juce::MemoryOutputStream stream(msg, false);
        stream.writeString(ScannerIPC::MSG_SCAN_COMPLETE);
        sendMessageToCoordinator(msg);
    }
};

//==============================================================================
class PluginScannerApplication : public juce::JUCEApplicationBase {
  public:
    const juce::String getApplicationName() override {
        return "MAGDA Plugin Scanner";
    }
    const juce::String getApplicationVersion() override {
        return "1.0.0";
    }
    bool moreThanOneInstanceAllowed() override {
        return true;
    }

    void initialise(const juce::String& commandLine) override {
        initLog();  // Initialize log file first
        log("[Scanner] Starting with args: " + commandLine.toStdString());

        worker_ = std::make_unique<PluginScannerWorker>();

        if (!worker_->initialiseFromCommandLine(commandLine, "magda-plugin-scanner")) {
            log("[Scanner] Failed to initialize from command line");
            setApplicationReturnValue(1);
            quit();
            return;
        }

        log("[Scanner] Initialized successfully, waiting for commands");
    }

    void shutdown() override {
        log("[Scanner] Shutting down");
        worker_.reset();
    }

    void systemRequestedQuit() override {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}
    void suspended() override {}
    void resumed() override {}
    void unhandledException(const std::exception*, const juce::String&, int) override {
        log("[Scanner] Unhandled exception - exiting");
    }

  private:
    std::unique_ptr<PluginScannerWorker> worker_;
};

//==============================================================================
START_JUCE_APPLICATION(PluginScannerApplication)
