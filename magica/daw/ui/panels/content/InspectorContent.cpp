#include "InspectorContent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"
#include "../../utils/TimelineUtils.hpp"

namespace magica::daw::ui {

InspectorContent::InspectorContent() {
    setName("Inspector");

    // Setup title
    titleLabel_.setText("Inspector", juce::dontSendNotification);
    titleLabel_.setFont(FontManager::getInstance().getUIFont(14.0f));
    titleLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addAndMakeVisible(titleLabel_);

    // No selection label
    noSelectionLabel_.setText("No selection", juce::dontSendNotification);
    noSelectionLabel_.setFont(FontManager::getInstance().getUIFont(12.0f));
    noSelectionLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    noSelectionLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(noSelectionLabel_);

    // ========================================================================
    // Track properties section
    // ========================================================================

    // Track name
    trackNameLabel_.setText("Name", juce::dontSendNotification);
    trackNameLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    trackNameLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(trackNameLabel_);

    trackNameValue_.setFont(FontManager::getInstance().getUIFont(12.0f));
    trackNameValue_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    trackNameValue_.setColour(juce::Label::backgroundColourId,
                              DarkTheme::getColour(DarkTheme::SURFACE));
    trackNameValue_.setEditable(true);
    trackNameValue_.onTextChange = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackName(selectedTrackId_,
                                                             trackNameValue_.getText());
        }
    };
    addChildComponent(trackNameValue_);

    // Mute button
    muteButton_.setButtonText("M");
    muteButton_.setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    muteButton_.setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::STATUS_WARNING));
    muteButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getTextColour());
    muteButton_.setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    muteButton_.setClickingTogglesState(true);
    muteButton_.onClick = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackMuted(selectedTrackId_,
                                                              muteButton_.getToggleState());
        }
    };
    addChildComponent(muteButton_);

    // Solo button
    soloButton_.setButtonText("S");
    soloButton_.setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    soloButton_.setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    soloButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getTextColour());
    soloButton_.setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    soloButton_.setClickingTogglesState(true);
    soloButton_.onClick = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackSoloed(selectedTrackId_,
                                                               soloButton_.getToggleState());
        }
    };
    addChildComponent(soloButton_);

    // Gain slider
    gainLabel_.setText("Gain", juce::dontSendNotification);
    gainLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    gainLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(gainLabel_);

    gainSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    gainSlider_.setRange(0.0, 1.0, 0.01);
    gainSlider_.setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    gainSlider_.setColour(juce::Slider::thumbColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    gainSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackVolume(
                selectedTrackId_, static_cast<float>(gainSlider_.getValue()));
        }
    };
    addChildComponent(gainSlider_);

    // Pan slider
    panLabel_.setText("Pan", juce::dontSendNotification);
    panLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    panLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(panLabel_);

    panSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    panSlider_.setRange(-1.0, 1.0, 0.01);
    panSlider_.setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    panSlider_.setColour(juce::Slider::thumbColourId, DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    panSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackPan(
                selectedTrackId_, static_cast<float>(panSlider_.getValue()));
        }
    };
    addChildComponent(panSlider_);

    // ========================================================================
    // Clip properties section
    // ========================================================================

    // Clip name
    clipNameLabel_.setText("Name", juce::dontSendNotification);
    clipNameLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    clipNameLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(clipNameLabel_);

    clipNameValue_.setFont(FontManager::getInstance().getUIFont(12.0f));
    clipNameValue_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    clipNameValue_.setColour(juce::Label::backgroundColourId,
                             DarkTheme::getColour(DarkTheme::SURFACE));
    clipNameValue_.setEditable(true);
    clipNameValue_.onTextChange = [this]() {
        if (selectedClipId_ != magica::INVALID_CLIP_ID) {
            magica::ClipManager::getInstance().setClipName(selectedClipId_,
                                                           clipNameValue_.getText());
        }
    };
    addChildComponent(clipNameValue_);

    // Clip type
    clipTypeLabel_.setText("Type", juce::dontSendNotification);
    clipTypeLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    clipTypeLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(clipTypeLabel_);

    clipTypeValue_.setFont(FontManager::getInstance().getUIFont(12.0f));
    clipTypeValue_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addChildComponent(clipTypeValue_);

    // Clip start
    clipStartLabel_.setText("Start", juce::dontSendNotification);
    clipStartLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    clipStartLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(clipStartLabel_);

    clipStartValue_.setFont(FontManager::getInstance().getUIFont(12.0f));
    clipStartValue_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addChildComponent(clipStartValue_);

    // Clip length
    clipLengthLabel_.setText("Length", juce::dontSendNotification);
    clipLengthLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    clipLengthLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(clipLengthLabel_);

    clipLengthValue_.setFont(FontManager::getInstance().getUIFont(12.0f));
    clipLengthValue_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addChildComponent(clipLengthValue_);

    // Loop toggle
    clipLoopToggle_.setButtonText("Loop");
    clipLoopToggle_.setColour(juce::ToggleButton::textColourId, DarkTheme::getTextColour());
    clipLoopToggle_.setColour(juce::ToggleButton::tickColourId,
                              DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    clipLoopToggle_.onClick = [this]() {
        if (selectedClipId_ != magica::INVALID_CLIP_ID) {
            magica::ClipManager::getInstance().setClipLoopEnabled(selectedClipId_,
                                                                  clipLoopToggle_.getToggleState());
        }
    };
    addChildComponent(clipLoopToggle_);

    // Loop length
    clipLoopLengthLabel_.setText("Loop Length", juce::dontSendNotification);
    clipLoopLengthLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    clipLoopLengthLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    addChildComponent(clipLoopLengthLabel_);

    clipLoopLengthSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    clipLoopLengthSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    clipLoopLengthSlider_.setRange(0.25, 64.0, 0.25);
    clipLoopLengthSlider_.setColour(juce::Slider::trackColourId,
                                    DarkTheme::getColour(DarkTheme::SURFACE));
    clipLoopLengthSlider_.setColour(juce::Slider::thumbColourId,
                                    DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    clipLoopLengthSlider_.onValueChange = [this]() {
        if (selectedClipId_ != magica::INVALID_CLIP_ID) {
            magica::ClipManager::getInstance().setClipLoopLength(selectedClipId_,
                                                                 clipLoopLengthSlider_.getValue());
        }
    };
    addChildComponent(clipLoopLengthSlider_);

    // Register as listeners
    magica::TrackManager::getInstance().addListener(this);
    magica::ClipManager::getInstance().addListener(this);
    magica::SelectionManager::getInstance().addListener(this);

    // Check if there's already a selection
    currentSelectionType_ = magica::SelectionManager::getInstance().getSelectionType();
    selectedTrackId_ = magica::SelectionManager::getInstance().getSelectedTrack();
    selectedClipId_ = magica::SelectionManager::getInstance().getSelectedClip();
    updateSelectionDisplay();
}

InspectorContent::~InspectorContent() {
    magica::TrackManager::getInstance().removeListener(this);
    magica::ClipManager::getInstance().removeListener(this);
    magica::SelectionManager::getInstance().removeListener(this);
}

void InspectorContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());
}

void InspectorContent::resized() {
    auto bounds = getLocalBounds().reduced(10);

    titleLabel_.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);  // Spacing

    if (currentSelectionType_ == magica::SelectionType::None) {
        // Center the no-selection label
        noSelectionLabel_.setBounds(bounds);
    } else if (currentSelectionType_ == magica::SelectionType::Track) {
        // Track properties layout
        trackNameLabel_.setBounds(bounds.removeFromTop(16));
        trackNameValue_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(12);

        // Mute/Solo row
        auto buttonRow = bounds.removeFromTop(28);
        muteButton_.setBounds(buttonRow.removeFromLeft(40));
        buttonRow.removeFromLeft(8);
        soloButton_.setBounds(buttonRow.removeFromLeft(40));
        bounds.removeFromTop(12);

        // Gain
        gainLabel_.setBounds(bounds.removeFromTop(16));
        gainSlider_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(12);

        // Pan
        panLabel_.setBounds(bounds.removeFromTop(16));
        panSlider_.setBounds(bounds.removeFromTop(24));
    } else if (currentSelectionType_ == magica::SelectionType::Clip) {
        // Clip properties layout
        clipNameLabel_.setBounds(bounds.removeFromTop(16));
        clipNameValue_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(12);

        // Type (read-only)
        clipTypeLabel_.setBounds(bounds.removeFromTop(16));
        clipTypeValue_.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(12);

        // Start time (read-only for now)
        clipStartLabel_.setBounds(bounds.removeFromTop(16));
        clipStartValue_.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(12);

        // Length (read-only for now)
        clipLengthLabel_.setBounds(bounds.removeFromTop(16));
        clipLengthValue_.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(12);

        // Loop toggle
        clipLoopToggle_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(8);

        // Loop length
        clipLoopLengthLabel_.setBounds(bounds.removeFromTop(16));
        clipLoopLengthSlider_.setBounds(bounds.removeFromTop(24));
    }
}

void InspectorContent::onActivated() {
    // Refresh from current selection
    currentSelectionType_ = magica::SelectionManager::getInstance().getSelectionType();
    selectedTrackId_ = magica::SelectionManager::getInstance().getSelectedTrack();
    selectedClipId_ = magica::SelectionManager::getInstance().getSelectedClip();
    updateSelectionDisplay();
}

void InspectorContent::onDeactivated() {
    // Nothing to do
}

// ============================================================================
// TrackManagerListener
// ============================================================================

void InspectorContent::tracksChanged() {
    // Track may have been deleted
    if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
        const auto* track = magica::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (!track) {
            selectedTrackId_ = magica::INVALID_TRACK_ID;
            updateSelectionDisplay();
        }
    }
}

void InspectorContent::trackPropertyChanged(int trackId) {
    if (static_cast<magica::TrackId>(trackId) == selectedTrackId_) {
        updateFromSelectedTrack();
    }
}

void InspectorContent::trackSelectionChanged(magica::TrackId trackId) {
    if (currentSelectionType_ == magica::SelectionType::Track) {
        selectedTrackId_ = trackId;
        updateFromSelectedTrack();
    }
}

// ============================================================================
// ClipManagerListener
// ============================================================================

void InspectorContent::clipsChanged() {
    // Clip may have been deleted
    if (selectedClipId_ != magica::INVALID_CLIP_ID) {
        const auto* clip = magica::ClipManager::getInstance().getClip(selectedClipId_);
        if (!clip) {
            selectedClipId_ = magica::INVALID_CLIP_ID;
            updateSelectionDisplay();
        }
    }
}

void InspectorContent::clipPropertyChanged(magica::ClipId clipId) {
    if (clipId == selectedClipId_) {
        updateFromSelectedClip();
    }
}

void InspectorContent::clipSelectionChanged(magica::ClipId clipId) {
    if (currentSelectionType_ == magica::SelectionType::Clip) {
        selectedClipId_ = clipId;
        updateFromSelectedClip();
    }
}

// ============================================================================
// SelectionManagerListener
// ============================================================================

void InspectorContent::selectionTypeChanged(magica::SelectionType newType) {
    currentSelectionType_ = newType;

    // Update the appropriate selection ID
    switch (newType) {
        case magica::SelectionType::Track:
            selectedTrackId_ = magica::SelectionManager::getInstance().getSelectedTrack();
            selectedClipId_ = magica::INVALID_CLIP_ID;
            break;

        case magica::SelectionType::Clip:
            selectedClipId_ = magica::SelectionManager::getInstance().getSelectedClip();
            selectedTrackId_ = magica::INVALID_TRACK_ID;
            break;

        default:
            selectedTrackId_ = magica::INVALID_TRACK_ID;
            selectedClipId_ = magica::INVALID_CLIP_ID;
            break;
    }

    updateSelectionDisplay();
}

// ============================================================================
// Update Methods
// ============================================================================

void InspectorContent::updateSelectionDisplay() {
    switch (currentSelectionType_) {
        case magica::SelectionType::None:
        case magica::SelectionType::TimeRange:
            showTrackControls(false);
            showClipControls(false);
            noSelectionLabel_.setVisible(true);
            break;

        case magica::SelectionType::Track:
            showClipControls(false);
            noSelectionLabel_.setVisible(false);
            updateFromSelectedTrack();
            break;

        case magica::SelectionType::Clip:
            showTrackControls(false);
            noSelectionLabel_.setVisible(false);
            updateFromSelectedClip();
            break;
    }

    resized();
    repaint();
}

void InspectorContent::updateFromSelectedTrack() {
    if (selectedTrackId_ == magica::INVALID_TRACK_ID) {
        showTrackControls(false);
        noSelectionLabel_.setVisible(true);
        return;
    }

    const auto* track = magica::TrackManager::getInstance().getTrack(selectedTrackId_);
    if (track) {
        trackNameValue_.setText(track->name, juce::dontSendNotification);
        muteButton_.setToggleState(track->muted, juce::dontSendNotification);
        soloButton_.setToggleState(track->soloed, juce::dontSendNotification);
        gainSlider_.setValue(track->volume, juce::dontSendNotification);
        panSlider_.setValue(track->pan, juce::dontSendNotification);

        showTrackControls(true);
        noSelectionLabel_.setVisible(false);
    } else {
        showTrackControls(false);
        noSelectionLabel_.setVisible(true);
    }

    resized();
    repaint();
}

void InspectorContent::updateFromSelectedClip() {
    if (selectedClipId_ == magica::INVALID_CLIP_ID) {
        showClipControls(false);
        noSelectionLabel_.setVisible(true);
        return;
    }

    const auto* clip = magica::ClipManager::getInstance().getClip(selectedClipId_);
    if (clip) {
        clipNameValue_.setText(clip->name, juce::dontSendNotification);
        clipTypeValue_.setText(magica::getClipTypeName(clip->type), juce::dontSendNotification);

        // TODO: Get tempo from TimelineController instead of hardcoding
        constexpr double BPM = 120.0;
        constexpr int BEATS_PER_BAR = 4;

        // Format start time as bars.beats.ticks
        auto startStr =
            magica::TimelineUtils::formatTimeAsBarsBeats(clip->startTime, BPM, BEATS_PER_BAR);
        clipStartValue_.setText(juce::String(startStr), juce::dontSendNotification);

        // Format length as bars and beats
        auto lengthStr =
            magica::TimelineUtils::formatDurationAsBarsBeats(clip->length, BPM, BEATS_PER_BAR);
        clipLengthValue_.setText(juce::String(lengthStr), juce::dontSendNotification);

        clipLoopToggle_.setToggleState(clip->internalLoopEnabled, juce::dontSendNotification);
        clipLoopLengthSlider_.setValue(clip->internalLoopLength, juce::dontSendNotification);

        showClipControls(true);
        noSelectionLabel_.setVisible(false);
    } else {
        showClipControls(false);
        noSelectionLabel_.setVisible(true);
    }

    resized();
    repaint();
}

void InspectorContent::showTrackControls(bool show) {
    trackNameLabel_.setVisible(show);
    trackNameValue_.setVisible(show);
    muteButton_.setVisible(show);
    soloButton_.setVisible(show);
    gainLabel_.setVisible(show);
    gainSlider_.setVisible(show);
    panLabel_.setVisible(show);
    panSlider_.setVisible(show);
}

void InspectorContent::showClipControls(bool show) {
    clipNameLabel_.setVisible(show);
    clipNameValue_.setVisible(show);
    clipTypeLabel_.setVisible(show);
    clipTypeValue_.setVisible(show);
    clipStartLabel_.setVisible(show);
    clipStartValue_.setVisible(show);
    clipLengthLabel_.setVisible(show);
    clipLengthValue_.setVisible(show);
    clipLoopToggle_.setVisible(show);
    clipLoopLengthLabel_.setVisible(show);
    clipLoopLengthSlider_.setVisible(show);
}

}  // namespace magica::daw::ui
