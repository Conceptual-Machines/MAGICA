#include "TimelineComponent.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

TimelineComponent::TimelineComponent() {
    setSize(800, 40);
}

TimelineComponent::~TimelineComponent() = default;

void TimelineComponent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::TIMELINE_BACKGROUND));
    
    // Draw border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(getLocalBounds(), 1);
    
    drawTimeMarkers(g);
    drawPlayhead(g);
}

void TimelineComponent::resized() {
    // Zoom is now controlled by parent component for proper synchronization
    // No automatic zoom calculation here
}

void TimelineComponent::setTimelineLength(double lengthInSeconds) {
    timelineLength = lengthInSeconds;
    resized();
    repaint();
}

void TimelineComponent::setPlayheadPosition(double position) {
    playheadPosition = position;
    repaint();
}

void TimelineComponent::setZoom(double pixelsPerSecond) {
    zoom = pixelsPerSecond;
    repaint();
}

void TimelineComponent::mouseDown(const juce::MouseEvent& event) {
    double clickTime = pixelToTime(event.x);
    setPlayheadPosition(clickTime);
    
    // Notify parent of position change
    if (onPlayheadPositionChanged) {
        onPlayheadPositionChanged(clickTime);
    }
}

void TimelineComponent::mouseDrag(const juce::MouseEvent& event) {
    double dragTime = pixelToTime(event.x);
    setPlayheadPosition(dragTime);
    
    // Notify parent of position change
    if (onPlayheadPositionChanged) {
        onPlayheadPositionChanged(dragTime);
    }
}

double TimelineComponent::pixelToTime(int pixel) const {
    if (zoom > 0) {
        return pixel / zoom;
    }
    return 0.0;
}

int TimelineComponent::timeToPixel(double time) const {
    return static_cast<int>(time * zoom);
}

void TimelineComponent::drawTimeMarkers(juce::Graphics& g) {
    g.setColour(DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
    g.setFont(FontManager::getInstance().getUIFont(11.0f));
    
    // Calculate appropriate marker spacing based on zoom
    // We want markers to be spaced at least 30 pixels apart
    const int minPixelSpacing = 30;
    int markerInterval = 1; // Start with 1 second intervals
    
    // Adjust interval if markers would be too close
    while (timeToPixel(markerInterval) < minPixelSpacing && markerInterval < 60) {
        markerInterval *= (markerInterval < 10) ? 2 : 5; // 1,2,5,10,20,50...
    }
    
    // Draw time markers
    for (int i = 0; i <= timelineLength; i += markerInterval) {
        int x = timeToPixel(i);
        if (x >= 0 && x < getWidth()) {
            // Draw tick mark
            g.drawLine(x, getHeight() - 10, x, getHeight() - 2);
            
            // Draw time label
            int minutes = i / 60;
            int seconds = i % 60;
            juce::String timeStr = juce::String::formatted("%d:%02d", minutes, seconds);
            g.drawText(timeStr, x - 20, 5, 40, 20, juce::Justification::centred);
        }
    }
}

void TimelineComponent::drawPlayhead(juce::Graphics& g) {
    int playheadX = timeToPixel(playheadPosition);
    if (playheadX >= 0 && playheadX < getWidth()) {
        // Draw shadow for better visibility
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(playheadX + 1, 0, playheadX + 1, getHeight(), 5.0f);
        // Draw main playhead line
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(playheadX, 0, playheadX, getHeight(), 4.0f);
    }
}

} // namespace magica 