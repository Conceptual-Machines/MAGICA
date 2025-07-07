#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magica {

class TimelineComponent : public juce::Component {
public:
    TimelineComponent();
    ~TimelineComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timeline controls
    void setTimelineLength(double lengthInSeconds);
    void setPlayheadPosition(double position);
    void setZoom(double pixelsPerSecond);

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    // Callback for playhead position changes
    std::function<void(double)> onPlayheadPositionChanged;

private:
    double timelineLength = 300.0;
    double playheadPosition = 0.0;
    double zoom = 1.0; // pixels per second
    
    // Helper methods
    double pixelToTime(int pixel) const;
    int timeToPixel(double time) const;
    void drawTimeMarkers(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};

} // namespace magica 