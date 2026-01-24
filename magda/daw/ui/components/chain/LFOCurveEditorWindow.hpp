#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "LFOCurveEditor.hpp"
#include "core/ModInfo.hpp"

namespace magda::daw::ui {

/**
 * @brief Popup window for larger LFO curve editing
 *
 * Provides a resizable window with a larger curve editor for detailed waveform editing.
 * The curve editor includes integrated phase indicator animation.
 */
class LFOCurveEditorWindow : public juce::DocumentWindow {
  public:
    LFOCurveEditorWindow(magda::ModInfo* modInfo, std::function<void()> onWaveformChanged,
                         std::function<void()> onDragPreview = nullptr);
    ~LFOCurveEditorWindow() override = default;

    void closeButtonPressed() override;

    // Get the curve editor for syncing
    magda::LFOCurveEditor& getCurveEditor() {
        return curveEditor_;
    }

  private:
    magda::LFOCurveEditor curveEditor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOCurveEditorWindow)
};

}  // namespace magda::daw::ui
