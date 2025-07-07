#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "../components/TrackComponent.hpp"
#include "../components/TimelineComponent.hpp"

namespace magica {

class MainView : public juce::Component {
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

private:
    // Main viewport for scrolling
    std::unique_ptr<juce::Viewport> viewport;
    
    // Content component that goes inside the viewport
    class ViewportContent;
    std::unique_ptr<ViewportContent> content;

    // Timeline at the top
    std::unique_ptr<TimelineComponent> timeline;

    // Track area
    class TrackArea;
    std::unique_ptr<TrackArea> trackArea;

    // Zoom and scroll state
    double horizontalZoom = 1.0;  // Pixels per second
    double verticalZoom = 1.0;    // Track height multiplier
    double timelineLength = 300.0; // Total timeline length in seconds
    double playheadPosition = 0.0;

    // Layout constants
    static constexpr int TIMELINE_HEIGHT = 40;
    static constexpr int DEFAULT_TRACK_HEIGHT = 80;
    static constexpr int MIN_TRACK_HEIGHT = 40;
    static constexpr int MAX_TRACK_HEIGHT = 200;

    // Helper methods
    void updateContentSize();
    void updateViewportPosition();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

// Content component for the viewport
class MainView::ViewportContent : public juce::Component {
public:
    ViewportContent(MainView& owner);
    ~ViewportContent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    MainView& owner;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewportContent)
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

private:
    std::vector<std::unique_ptr<TrackComponent>> tracks;
    int selectedTrackIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackArea)
};

} // namespace magica 