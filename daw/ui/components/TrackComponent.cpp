#include "TrackComponent.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

TrackComponent::TrackComponent() {
    setSize(800, 80);
}

TrackComponent::~TrackComponent() = default;

void TrackComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Background
    if (selected) {
        g.setColour(DarkTheme::getColour(DarkTheme::TRACK_SELECTED));
    } else {
        g.setColour(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    }
    g.fillRect(bounds);
    
    // Border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(bounds, 1);
    
    // Control area background
    auto controlArea = bounds.removeFromLeft(CONTROL_AREA_WIDTH);
    g.setColour(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
    g.fillRect(controlArea);
    
    // Track name
    g.setColour(DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    g.setFont(FontManager::getInstance().getUIFontMedium(14.0f));
    auto nameArea = controlArea.removeFromTop(25).reduced(8, 2);
    g.drawText(trackName, nameArea, juce::Justification::left);
    
    // Draw control buttons (simplified for now)
    auto buttonArea = controlArea.removeFromTop(30).reduced(8, 2);
    
    // Mute button
    auto muteButton = buttonArea.removeFromLeft(BUTTON_SIZE);
    g.setColour(muted ? juce::Colours::red : DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    g.fillRoundedRectangle(muteButton.toFloat(), 3.0f);
    g.setColour(DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    g.setFont(FontManager::getInstance().getUIFont(10.0f));
    g.drawText("M", muteButton, juce::Justification::centred);
    
    buttonArea.removeFromLeft(4); // spacing
    
    // Solo button
    auto soloButton = buttonArea.removeFromLeft(BUTTON_SIZE);
    g.setColour(solo ? juce::Colours::yellow : DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    g.fillRoundedRectangle(soloButton.toFloat(), 3.0f);
    g.setColour(DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    g.drawText("S", soloButton, juce::Justification::centred);
    
    // Track content area (where audio clips would go)
    auto contentArea = bounds;
    g.setColour(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    g.fillRect(contentArea);
    
    // Grid lines for beats/bars (simplified)
    g.setColour(DarkTheme::getColour(DarkTheme::GRID_LINE));
    for (int x = 0; x < contentArea.getWidth(); x += 50) {
        g.drawVerticalLine(contentArea.getX() + x, 
                          contentArea.getY(), 
                          contentArea.getBottom());
    }
}

void TrackComponent::resized() {
    // Layout will be handled in paint for now
}

void TrackComponent::setTrackName(const juce::String& name) {
    trackName = name;
    repaint();
}

juce::String TrackComponent::getTrackName() const {
    return trackName;
}

void TrackComponent::setSelected(bool shouldBeSelected) {
    if (selected != shouldBeSelected) {
        selected = shouldBeSelected;
        repaint();
    }
}

bool TrackComponent::isSelected() const {
    return selected;
}

void TrackComponent::setMuted(bool shouldBeMuted) {
    if (muted != shouldBeMuted) {
        muted = shouldBeMuted;
        repaint();
    }
}

void TrackComponent::setSolo(bool shouldBeSolo) {
    if (solo != shouldBeSolo) {
        solo = shouldBeSolo;
        repaint();
    }
}

void TrackComponent::setVolume(float newVolume) {
    volume = juce::jlimit(0.0f, 1.0f, newVolume);
}

void TrackComponent::setPan(float newPan) {
    pan = juce::jlimit(-1.0f, 1.0f, newPan);
}

} // namespace magica 