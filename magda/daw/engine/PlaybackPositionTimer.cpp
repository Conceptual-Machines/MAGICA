#include "PlaybackPositionTimer.hpp"

#include "AudioEngine.hpp"
#include "ui/state/TimelineController.hpp"
#include "ui/state/TimelineEvents.hpp"

namespace magda {

PlaybackPositionTimer::PlaybackPositionTimer(AudioEngine& engine, TimelineController& timeline)
    : engine_(engine), timeline_(timeline) {}

PlaybackPositionTimer::~PlaybackPositionTimer() {
    stopTimer();
}

void PlaybackPositionTimer::start() {
    startTimer(UPDATE_INTERVAL_MS);
}

void PlaybackPositionTimer::stop() {
    stopTimer();
}

bool PlaybackPositionTimer::isRunning() const {
    return isTimerRunning();
}

void PlaybackPositionTimer::timerCallback() {
    static int timerCallCount = 0;
    static bool lastPlaying = false;
    bool currentlyPlaying = engine_.isPlaying();

    if (currentlyPlaying != lastPlaying) {
        std::cout << ">>> PlaybackPositionTimer - playing state changed to: " << currentlyPlaying
                  << " (timer call #" << timerCallCount << ")" << std::endl;
        lastPlaying = currentlyPlaying;
    }
    timerCallCount++;

    // Update trigger state for transport-synced devices (tone generator, etc.)
    engine_.updateTriggerState();

    if (engine_.isPlaying()) {
        double position = engine_.getCurrentPosition();
        // Only update playback position (the moving cursor), not edit position
        timeline_.dispatch(SetPlaybackPositionEvent{position});
    }
}

}  // namespace magda
