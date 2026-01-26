#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

#include <iostream>
#include <memory>

#include "core/ClipManager.hpp"
#include "core/ModulatorEngine.hpp"
#include "core/TrackManager.hpp"
#include "engine/TracktionEngineWrapper.hpp"
#include "ui/themes/DarkTheme.hpp"
#include "ui/themes/FontManager.hpp"
#include "ui/windows/MainWindow.hpp"

using namespace juce;

class MagdaDAWApplication : public JUCEApplication {
  private:
    std::unique_ptr<magda::TracktionEngineWrapper> daw_engine_;
    std::unique_ptr<magda::MainWindow> mainWindow_;
    std::unique_ptr<juce::LookAndFeel> lookAndFeel_;

  public:
    MagdaDAWApplication() = default;

    const String getApplicationName() override {
        return "MAGDA";
    }
    const String getApplicationVersion() override {
        return "1.0.0";
    }

    void initialise(const String& commandLine) override {
        // Check if we're being launched as a plugin scanner subprocess
        if (tracktion::PluginManager::startChildProcessPluginScan(commandLine)) {
            // This process is a plugin scanner - it will exit when done
            return;
        }

        // 1. Initialize fonts
        magda::FontManager::getInstance().initialize();

        // 2. Set up dark theme
        lookAndFeel_ = std::make_unique<juce::LookAndFeel_V4>();
        magda::DarkTheme::applyToLookAndFeel(*lookAndFeel_);
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel_.get());

        // 3. Initialize audio engine
        daw_engine_ = std::make_unique<magda::TracktionEngineWrapper>();
        if (!daw_engine_->initialize()) {
            std::cerr << "ERROR: Failed to initialize Tracktion Engine" << std::endl;
            quit();
            return;
        }

        std::cout << "✓ Audio engine initialized" << std::endl;

        // 4. Create main window with full UI (pass the audio engine)
        mainWindow_ = std::make_unique<magda::MainWindow>(daw_engine_.get());

        // 5. Create test clips for development (after singletons are initialized)
        magda::ClipManager::getInstance().createTestClips();

        std::cout << "🎵 MAGDA is ready!" << std::endl;
    }

    void shutdown() override {
        // Shutdown all singletons BEFORE JUCE cleanup to prevent static cleanup issues
        // This clears all JUCE objects (Strings, Colours, etc.) while JUCE is still alive
        magda::ModulatorEngine::getInstance().shutdown();  // Destroy timer
        magda::TrackManager::getInstance().shutdown();     // Clear tracks with JUCE objects
        magda::ClipManager::getInstance().shutdown();      // Clear clips with JUCE objects

        // Clear default LookAndFeel BEFORE destroying windows
        // This ensures components switch away from our custom L&F before we delete them
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);

        // Graceful shutdown - destroy UI
        mainWindow_.reset();

        // Now destroy engine
        daw_engine_.reset();

        // Destroy our custom LookAndFeel (no components reference it now)
        lookAndFeel_.reset();

        // Release fonts before JUCE's leak detector runs
        magda::FontManager::getInstance().shutdown();

        std::cout << "👋 MAGDA shutdown complete" << std::endl;
    }

    void systemRequestedQuit() override {
        quit();
    }
};

// JUCE application startup
START_JUCE_APPLICATION(MagdaDAWApplication)
