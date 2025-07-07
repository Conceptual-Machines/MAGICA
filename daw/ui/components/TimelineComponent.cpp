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
    // Update zoom based on component width
    if (timelineLength > 0) {
        zoom = getWidth() / timelineLength;
    }
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
    
    // TODO: Notify transport of position change
}

void TimelineComponent::mouseDrag(const juce::MouseEvent& event) {
    double dragTime = pixelToTime(event.x);
    setPlayheadPosition(dragTime);
    
    // TODO: Notify transport of position change
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
    
    // Draw time markers every second
    for (int i = 0; i <= timelineLength; ++i) {
        int x = timeToPixel(i);
        if (x >= 0 && x < getWidth()) {
            // Draw tick mark
            g.drawLine(x, getHeight() - 10, x, getHeight() - 2);
            
            // Draw time label every 5 seconds
            if (i % 5 == 0) {
                int minutes = i / 60;
                int seconds = i % 60;
                juce::String timeStr = juce::String::formatted("%d:%02d", minutes, seconds);
                g.drawText(timeStr, x - 20, 5, 40, 20, juce::Justification::centred);
            }
        }
    }
}

void TimelineComponent::drawPlayhead(juce::Graphics& g) {
    int playheadX = timeToPixel(playheadPosition);
    if (playheadX >= 0 && playheadX < getWidth()) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(playheadX, 0, playheadX, getHeight(), 2.0f);
    }
}

} // namespace magica 