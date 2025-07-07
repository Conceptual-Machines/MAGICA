#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "../components/TrackComponent.hpp"
#include "../components/TimelineComponent.hpp"
#include "../components/ArrangementTimelineComponent.hpp"

namespace magica {

class MainView : public juce::Component, public juce::ScrollBar::Listener {
public:
    MainView();
    ~MainView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Zoom and scroll controls
    void setHorizontalZoom(double zoomFactor);
    void setVerticalZoom(double zoomFactor);
    void scrollToPosition(double timePosition);
    void scrollToTrack(int trackIndex);

    // Track management
    void addTrack();
    void removeTrack(int trackIndex);
    void selectTrack(int trackIndex);

    // Timeline controls
    void setTimelineLength(double lengthInSeconds);
    void setPlayheadPosition(double position);
    
    // Zoom accessors
    double getHorizontalZoom() const { return horizontalZoom; }

    // ScrollBar::Listener implementation
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

private:
    // Arrangement timeline viewport (horizontal scroll only)
    std::unique_ptr<juce::Viewport> arrangementViewport;
    std::unique_ptr<ArrangementTimelineComponent> arrangementTimeline;
    
    // Timeline viewport (horizontal scroll only)
    std::unique_ptr<juce::Viewport> timelineViewport;
    std::unique_ptr<TimelineComponent> timeline;
    
    // Track viewport (both horizontal and vertical scroll)
    std::unique_ptr<juce::Viewport> trackViewport;
    
    // Content component that goes inside the track viewport
    class TrackViewportContent;
    std::unique_ptr<TrackViewportContent> trackContent;

    // Track area
    class TrackArea;
    std::unique_ptr<TrackArea> trackArea;
    
    // Playhead component (always on top)
    class PlayheadComponent;
    std::unique_ptr<PlayheadComponent> playheadComponent;

    // Zoom and scroll state
    double horizontalZoom = 1.0;  // Pixels per second
    double verticalZoom = 1.0;    // Track height multiplier
    double timelineLength = 120.0; // Total timeline length in seconds
    double playheadPosition = 0.0;

    // Layout constants
    static constexpr int ARRANGEMENT_HEIGHT = 30;
    static constexpr int TIMELINE_HEIGHT = 80;
    static constexpr int DEFAULT_TRACK_HEIGHT = 80;
    static constexpr int MIN_TRACK_HEIGHT = 40;
    static constexpr int MAX_TRACK_HEIGHT = 200;

    // Helper methods
    void updateContentSizes();
    void syncHorizontalScrolling();
    void onTimelineScroll();
    void onTrackScroll();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

// Dedicated playhead component that always stays on top
class MainView::PlayheadComponent : public juce::Component {
public:
    PlayheadComponent(MainView& owner);
    ~PlayheadComponent() override;

    void paint(juce::Graphics& g) override;
    void setPlayheadPosition(double position);

private:
    MainView& owner;
    double playheadPosition = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayheadComponent)
};

// Content component for the track viewport
class MainView::TrackViewportContent : public juce::Component {
public:
    TrackViewportContent(MainView& owner);
    ~TrackViewportContent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    MainView& owner;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackViewportContent)
};

// Track area component
class MainView::TrackArea : public juce::Component {
public:
    TrackArea();
    ~TrackArea() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void addTrack();
    void removeTrack(int index);
    void selectTrack(int index);
    int getNumTracks() const;
    void updateTotalHeight();
    TrackComponent* getTrack(int index) const;

private:
    std::vector<std::unique_ptr<TrackComponent>> tracks;
    int selectedTrackIndex = -1;
    
    MainView* getParentMainView() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackArea)
};

} // namespace magica 