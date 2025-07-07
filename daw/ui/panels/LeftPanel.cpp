#include "LeftPanel.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

LeftPanel::LeftPanel() {
    setName("Left Panel");
}

LeftPanel::~LeftPanel() = default;

void LeftPanel::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());
    
    // Draw a border
    g.setColour(DarkTheme::getBorderColour());
    g.drawRect(getLocalBounds(), 1);
    
    // Draw placeholder text
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(14.0f));
    g.drawText("Left Panel\n(Browser/Library)", getLocalBounds(), juce::Justification::centred);
}

void LeftPanel::resized() {
    // Layout will be implemented later
}

} // namespace magica 