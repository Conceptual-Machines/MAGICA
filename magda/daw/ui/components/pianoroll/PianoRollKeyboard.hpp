#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace magda {

/**
 * Piano keyboard component for the piano roll.
 * Displays note names and responds to vertical scroll offset.
 * Supports vertical zoom by dragging up/down.
 */
class PianoRollKeyboard : public juce::Component {
  public:
    PianoRollKeyboard();
    ~PianoRollKeyboard() override = default;

    void paint(juce::Graphics& g) override;

    // Mouse interaction for zoom
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    // Configuration
    void setNoteHeight(int height);
    void setNoteRange(int minNote, int maxNote);
    void setScrollOffset(int offsetY);

    int getNoteHeight() const {
        return noteHeight_;
    }

    // Callbacks
    std::function<void(int, int, int)> onZoomChanged;  // newNoteHeight, anchorNote, anchorScreenY

  private:
    int noteHeight_ = 12;
    int minNote_ = 21;   // A0
    int maxNote_ = 108;  // C8
    int scrollOffsetY_ = 0;

    // Zoom drag state
    bool isZooming_ = false;
    int mouseDownX_ = 0;
    int mouseDownY_ = 0;
    int zoomStartHeight_ = 0;
    int zoomAnchorNote_ = 0;
    static constexpr int DRAG_THRESHOLD = 3;

    bool isBlackKey(int noteNumber) const;
    juce::String getNoteName(int noteNumber) const;
    int yToNoteNumber(int y) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollKeyboard)
};

}  // namespace magda
