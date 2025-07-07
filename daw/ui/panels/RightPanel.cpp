#include "RightPanel.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

RightPanel::RightPanel() {
    setName("Right Panel");
}

RightPanel::~RightPanel() = default;

void RightPanel::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());
    
    // Draw a border
    g.setColour(DarkTheme::getBorderColour());
    g.drawRect(getLocalBounds(), 1);
    
    // Draw placeholder text
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(14.0f));
    g.drawText("Right Panel\n(Inspector/Properties)", getLocalBounds(), juce::Justification::centred);
}

void RightPanel::resized() {
    // Layout will be implemented later
}

} // namespace magica 