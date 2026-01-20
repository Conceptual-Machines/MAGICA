#include "EmptyContent.hpp"

#include "../../themes/DarkTheme.hpp"

namespace magda::daw::ui {

EmptyContent::EmptyContent() {
    setName("Empty");
}

void EmptyContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());
}

void EmptyContent::resized() {
    // Nothing to layout
}

}  // namespace magda::daw::ui
