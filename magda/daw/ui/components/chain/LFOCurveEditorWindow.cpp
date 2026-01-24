#include "LFOCurveEditorWindow.hpp"

#include "ui/themes/DarkTheme.hpp"

namespace magda::daw::ui {

LFOCurveEditorWindow::LFOCurveEditorWindow(magda::ModInfo* modInfo,
                                           std::function<void()> onWaveformChanged,
                                           std::function<void()> onDragPreview)
    : DocumentWindow("LFO Curve Editor", DarkTheme::getColour(DarkTheme::BACKGROUND),
                     DocumentWindow::closeButton) {
    // Configure the curve editor
    curveEditor_.setModInfo(modInfo);
    curveEditor_.setCurveColour(DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    curveEditor_.onWaveformChanged = std::move(onWaveformChanged);
    curveEditor_.onDragPreview = std::move(onDragPreview);

    // Set as content with padding for edge points
    setContentNonOwned(&curveEditor_, true);

    // Window settings
    setSize(400, 250);
    setResizable(true, true);
    setResizeLimits(300, 150, 800, 500);
    setUsingNativeTitleBar(false);
    setVisible(true);
    setAlwaysOnTop(true);

    // Center on screen
    centreWithSize(getWidth(), getHeight());
}

void LFOCurveEditorWindow::closeButtonPressed() {
    setVisible(false);
}

}  // namespace magda::daw::ui
