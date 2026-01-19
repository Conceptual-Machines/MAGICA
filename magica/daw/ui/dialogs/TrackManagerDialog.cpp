#include "TrackManagerDialog.hpp"

#include "../themes/DarkTheme.hpp"

namespace magica {

// ============================================================================
// Content Component
// ============================================================================

class TrackManagerDialog::ContentComponent : public juce::Component,
                                             public juce::ListBoxModel,
                                             public ViewModeListener,
                                             public TrackManagerListener {
  public:
    ContentComponent() {
        // View mode selector
        viewModeLabel_.setText("View Mode:", juce::dontSendNotification);
        viewModeLabel_.setColour(juce::Label::textColourId,
                                 DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        addAndMakeVisible(viewModeLabel_);

        viewModeCombo_.addItem("Live", 1);
        viewModeCombo_.addItem("Arrange", 2);
        viewModeCombo_.addItem("Mix", 3);
        viewModeCombo_.addItem("Master", 4);

        auto currentMode = ViewModeController::getInstance().getViewMode();
        viewModeCombo_.setSelectedId(static_cast<int>(currentMode) + 1, juce::dontSendNotification);

        viewModeCombo_.onChange = [this]() {
            selectedMode_ = static_cast<ViewMode>(viewModeCombo_.getSelectedId() - 1);
            rebuildTrackList();
        };
        addAndMakeVisible(viewModeCombo_);

        selectedMode_ = currentMode;

        // Track list
        trackListBox_.setModel(this);
        trackListBox_.setRowHeight(28);
        trackListBox_.setColour(juce::ListBox::backgroundColourId,
                                DarkTheme::getColour(DarkTheme::SURFACE));
        addAndMakeVisible(trackListBox_);

        // Info label
        infoLabel_.setText("Toggle visibility for tracks in selected view mode",
                           juce::dontSendNotification);
        infoLabel_.setColour(juce::Label::textColourId,
                             DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
        infoLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(infoLabel_);

        // Register listeners
        ViewModeController::getInstance().addListener(this);
        TrackManager::getInstance().addListener(this);

        rebuildTrackList();
        setSize(400, 350);
    }

    ~ContentComponent() override {
        ViewModeController::getInstance().removeListener(this);
        TrackManager::getInstance().removeListener(this);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(10);

        auto topRow = bounds.removeFromTop(30);
        viewModeLabel_.setBounds(topRow.removeFromLeft(80));
        viewModeCombo_.setBounds(topRow.reduced(5, 2));

        bounds.removeFromTop(10);

        infoLabel_.setBounds(bounds.removeFromBottom(25));
        bounds.removeFromBottom(5);

        trackListBox_.setBounds(bounds);
    }

    // ViewModeListener
    void viewModeChanged(ViewMode mode, const AudioEngineProfile& /*profile*/) override {
        viewModeCombo_.setSelectedId(static_cast<int>(mode) + 1, juce::dontSendNotification);
        selectedMode_ = mode;
        rebuildTrackList();
    }

    // TrackManagerListener
    void tracksChanged() override {
        rebuildTrackList();
    }

    // ListBoxModel implementation (inline since it's simple)
    int getNumRows() override {
        return static_cast<int>(trackIds_.size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
                          bool rowIsSelected) override {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(trackIds_.size()))
            return;

        auto trackId = trackIds_[rowNumber];
        const auto* track = TrackManager::getInstance().getTrack(trackId);
        if (!track)
            return;

        // Background
        if (rowIsSelected) {
            g.fillAll(DarkTheme::getColour(DarkTheme::ACCENT_BLUE).withAlpha(0.3f));
        }

        // Checkbox area
        auto checkboxBounds = juce::Rectangle<int>(5, (height - 18) / 2, 18, 18);
        bool isVisible = track->isVisibleIn(selectedMode_);

        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRect(checkboxBounds, 1);

        if (isVisible) {
            g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
            g.fillRect(checkboxBounds.reduced(3));
        }

        // Track name
        g.setColour(DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        g.drawText(track->name, 30, 0, width - 35, height, juce::Justification::centredLeft);

        // Track type indicator
        g.setColour(DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
        juce::String typeStr = juce::String("[") + getTrackTypeName(track->type) + "]";
        g.drawText(typeStr, width - 80, 0, 75, height, juce::Justification::centredRight);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& /*e*/) override {
        if (row < 0 || row >= static_cast<int>(trackIds_.size()))
            return;

        auto trackId = trackIds_[row];
        const auto* track = TrackManager::getInstance().getTrack(trackId);
        if (!track)
            return;

        // Toggle visibility
        bool currentlyVisible = track->isVisibleIn(selectedMode_);
        TrackManager::getInstance().setTrackVisible(trackId, selectedMode_, !currentlyVisible);

        trackListBox_.repaint();
    }

  private:
    void rebuildTrackList() {
        trackIds_.clear();
        const auto& tracks = TrackManager::getInstance().getTracks();

        DBG("TrackManagerDialog: rebuilding list, found " << tracks.size() << " tracks");

        for (const auto& track : tracks) {
            trackIds_.push_back(track.id);
        }

        trackListBox_.updateContent();
        trackListBox_.repaint();

        if (trackIds_.empty()) {
            infoLabel_.setText("No tracks. Use Track > Add Track to create one.",
                               juce::dontSendNotification);
        } else {
            infoLabel_.setText("Click a track to toggle visibility in " +
                                   juce::String(getViewModeName(selectedMode_)) + " view",
                               juce::dontSendNotification);
        }
    }

    static const char* getViewModeName(ViewMode mode) {
        switch (mode) {
            case ViewMode::Live:
                return "Live";
            case ViewMode::Arrange:
                return "Arrange";
            case ViewMode::Mix:
                return "Mix";
            case ViewMode::Master:
                return "Master";
        }
        return "Unknown";
    }

    juce::Label viewModeLabel_;
    juce::ComboBox viewModeCombo_;
    juce::ListBox trackListBox_{juce::String(), this};
    juce::Label infoLabel_;

    ViewMode selectedMode_ = ViewMode::Arrange;
    std::vector<TrackId> trackIds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentComponent)
};

// ============================================================================
// TrackManagerDialog
// ============================================================================

TrackManagerDialog::TrackManagerDialog()
    : DialogWindow("Track Manager", DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND), true) {
    content_ = std::make_unique<ContentComponent>();
    setContentOwned(content_.release(), true);
    centreWithSize(400, 350);
    setResizable(true, true);
    setUsingNativeTitleBar(true);

    TrackManager::getInstance().addListener(this);
}

TrackManagerDialog::~TrackManagerDialog() {
    TrackManager::getInstance().removeListener(this);
}

void TrackManagerDialog::closeButtonPressed() {
    setVisible(false);
}

void TrackManagerDialog::tracksChanged() {
    // Content component handles this via its own listener
}

void TrackManagerDialog::show() {
    auto* dialog = new TrackManagerDialog();
    dialog->setVisible(true);
    dialog->toFront(true);
}

}  // namespace magica
