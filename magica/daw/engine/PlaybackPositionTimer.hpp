#pragma once

#include <juce_events/juce_events.h>

namespace magica {

class TracktionEngineWrapper;
class TimelineController;

/**
 * @brief Timer that polls the audio engine for playhead position updates
 *
 * This class periodically polls the TracktionEngineWrapper for the current
 * playback position and dispatches SetPlayheadPositionEvent to the
 * TimelineController, which then notifies all listeners.
 */
class PlaybackPositionTimer : private juce::Timer {
  public:
    PlaybackPositionTimer(TracktionEngineWrapper& engine, TimelineController& timeline);
    ~PlaybackPositionTimer() override;

    void start();
    void stop();
    bool isRunning() const;

  private:
    void timerCallback() override;

    TracktionEngineWrapper& engine_;
    TimelineController& timeline_;

    static constexpr int UPDATE_INTERVAL_MS = 30;  // ~33fps for smooth playhead
};

}  // namespace magica
