#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>

namespace magica {

struct ArrangementSection {
    double startTime;
    double endTime;
    juce::String name;
    juce::Colour colour;
    
    ArrangementSection(double start, double end, const juce::String& sectionName, juce::Colour sectionColour = juce::Colours::blue)
        : startTime(start), endTime(end), name(sectionName), colour(sectionColour) {}
};

class ArrangementTimelineComponent : public juce::Component {
public:
    ArrangementTimelineComponent();
    ~ArrangementTimelineComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // Timeline properties
    void setTimelineLength(double lengthInSeconds);
    void setZoom(double pixelsPerSecond);
    void setPlayheadPosition(double position);

    // Section management
    void addSection(const juce::String& name, double startTime, double endTime, juce::Colour colour = juce::Colours::blue);
    void removeSection(int index);
    void clearSections();
    
    // Callbacks
    std::function<void(double)> onPlayheadPositionChanged;
    std::function<void(int, const ArrangementSection&)> onSectionChanged;
    std::function<void(const juce::String&, double, double)> onSectionAdded;

private:
    std::vector<std::unique_ptr<ArrangementSection>> sections;
    
    double timelineLength = 120.0;
    double zoom = 10.0; // pixels per second
    double playheadPosition = 0.0;
    
    int selectedSectionIndex = -1;
    bool isDraggingSection = false;
    bool isDraggingEdge = false;
    bool isDraggingStart = false; // true for start edge, false for end edge
    
    // Helper methods
    double pixelToTime(int pixel) const;
    int timeToPixel(double time) const;
    int findSectionAtPosition(int x, int y) const;
    bool isOnSectionEdge(int x, int sectionIndex, bool& isStartEdge) const;
    void drawSection(juce::Graphics& g, const ArrangementSection& section, bool isSelected) const;
    juce::String getDefaultSectionName() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementTimelineComponent)
};

} // namespace magica 