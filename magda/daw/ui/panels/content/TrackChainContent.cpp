#include "TrackChainContent.hpp"

#include <cmath>

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"
#include "../../themes/MixerMetrics.hpp"
#include "core/DeviceInfo.hpp"

namespace magda::daw::ui {

//==============================================================================
// GainMeterComponent - Vertical gain slider with peak meter background
//==============================================================================
class GainMeterComponent : public juce::Component,
                           public juce::Label::Listener,
                           private juce::Timer {
  public:
    GainMeterComponent() {
        // Editable label for dB value
        dbLabel_.setFont(FontManager::getInstance().getUIFont(9.0f));
        dbLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
        dbLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        dbLabel_.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        dbLabel_.setColour(juce::Label::outlineWhenEditingColourId,
                           DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        dbLabel_.setColour(juce::Label::backgroundWhenEditingColourId,
                           DarkTheme::getColour(DarkTheme::BACKGROUND));
        dbLabel_.setJustificationType(juce::Justification::centred);
        dbLabel_.setEditable(false, true, false);  // Single-click to edit
        dbLabel_.addListener(this);
        addAndMakeVisible(dbLabel_);

        updateLabel();

        // Start timer for mock meter animation
        startTimerHz(30);
    }

    ~GainMeterComponent() override {
        stopTimer();
    }

    void setGainDb(double db, juce::NotificationType notification = juce::sendNotification) {
        db = juce::jlimit(-60.0, 6.0, db);
        if (std::abs(gainDb_ - db) > 0.01) {
            gainDb_ = db;
            updateLabel();
            repaint();
            if (notification != juce::dontSendNotification && onGainChanged) {
                onGainChanged(gainDb_);
            }
        }
    }

    double getGainDb() const {
        return gainDb_;
    }

    // Mock meter level (0-1) - in real implementation this would come from audio processing
    void setMeterLevel(float level) {
        meterLevel_ = juce::jlimit(0.0f, 1.0f, level);
        repaint();
    }

    std::function<void(double)> onGainChanged;

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();
        auto meterArea = bounds.removeFromTop(bounds.getHeight() - 14).reduced(2);

        // Background
        g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
        g.fillRoundedRectangle(meterArea.toFloat(), 2.0f);

        // Meter fill (from bottom up)
        float fillHeight = meterLevel_ * meterArea.getHeight();
        auto fillArea = meterArea.removeFromBottom(static_cast<int>(fillHeight));

        // Gradient from green (low) to yellow to red (high)
        juce::ColourGradient gradient(
            juce::Colour(0xff2ecc71), 0.0f, static_cast<float>(meterArea.getBottom()),
            juce::Colour(0xffe74c3c), 0.0f, static_cast<float>(meterArea.getY()), false);
        gradient.addColour(0.7, juce::Colour(0xfff39c12));  // Yellow at 70%
        g.setGradientFill(gradient);
        g.fillRect(fillArea);

        // Gain position indicator (horizontal line)
        float gainNormalized = static_cast<float>((gainDb_ + 60.0) / 66.0);  // -60 to +6 dB
        int gainY =
            meterArea.getY() + static_cast<int>((1.0f - gainNormalized) * meterArea.getHeight());
        g.setColour(DarkTheme::getTextColour());
        g.drawHorizontalLine(gainY, static_cast<float>(meterArea.getX()),
                             static_cast<float>(meterArea.getRight()));

        // Small triangles on sides to show gain position
        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(meterArea.getX()), static_cast<float>(gainY - 3),
                             static_cast<float>(meterArea.getX()), static_cast<float>(gainY + 3),
                             static_cast<float>(meterArea.getX() + 4), static_cast<float>(gainY));
        g.fillPath(triangle);

        triangle.clear();
        triangle.addTriangle(
            static_cast<float>(meterArea.getRight()), static_cast<float>(gainY - 3),
            static_cast<float>(meterArea.getRight()), static_cast<float>(gainY + 3),
            static_cast<float>(meterArea.getRight() - 4), static_cast<float>(gainY));
        g.fillPath(triangle);

        // Border
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        auto fullMeterArea = getLocalBounds().removeFromTop(getHeight() - 14).reduced(2);
        g.drawRoundedRectangle(fullMeterArea.toFloat(), 2.0f, 1.0f);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        dbLabel_.setBounds(bounds.removeFromBottom(14));
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isLeftButtonDown()) {
            dragging_ = true;
            setGainFromY(e.y);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging_) {
            setGainFromY(e.y);
        }
    }

    void mouseUp(const juce::MouseEvent&) override {
        dragging_ = false;
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        // Reset to unity (0 dB)
        setGainDb(0.0);
    }

    // Label::Listener
    void labelTextChanged(juce::Label* label) override {
        if (label == &dbLabel_) {
            auto text = dbLabel_.getText().trim();
            // Remove "dB" suffix if present
            if (text.endsWithIgnoreCase("db")) {
                text = text.dropLastCharacters(2).trim();
            }
            double newDb = text.getDoubleValue();
            setGainDb(newDb);
        }
    }

  private:
    double gainDb_ = 0.0;
    float meterLevel_ = 0.0f;
    float peakLevel_ = 0.0f;
    bool dragging_ = false;
    juce::Label dbLabel_;

    void updateLabel() {
        if (gainDb_ <= -60.0) {
            dbLabel_.setText("-inf", juce::dontSendNotification);
        } else {
            dbLabel_.setText(juce::String(gainDb_, 1), juce::dontSendNotification);
        }
    }

    void setGainFromY(int y) {
        auto meterArea = getLocalBounds().removeFromTop(getHeight() - 14).reduced(2);
        float normalized = 1.0f - static_cast<float>(y - meterArea.getY()) / meterArea.getHeight();
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        double db = -60.0 + normalized * 66.0;  // -60 to +6 dB range
        setGainDb(db);
    }

    void timerCallback() override {
        // Mock meter animation - simulate audio activity
        // In real implementation, this would receive actual audio levels
        float targetLevel = static_cast<float>((gainDb_ + 60.0) / 66.0) * 0.8f;
        targetLevel += (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.1f;
        meterLevel_ = meterLevel_ * 0.9f + targetLevel * 0.1f;
        meterLevel_ = juce::jlimit(0.0f, 1.0f, meterLevel_);
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainMeterComponent)
};

//==============================================================================
// SquareButtonLookAndFeel - Square corners for device slot buttons
//==============================================================================
class SquareButtonLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& bgColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();

        auto baseColour = bgColour;
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.1f);

        g.setColour(baseColour);
        g.fillRect(bounds);

        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRect(bounds, 1.0f);
    }
};

//==============================================================================
// DeviceSlotComponent - Interactive device display
//==============================================================================
class TrackChainContent::DeviceSlotComponent : public juce::Component {
  public:
    static constexpr int GAIN_SLIDER_WIDTH = 28;
    static constexpr int MODULATOR_PANEL_WIDTH = 60;
    static constexpr int PARAM_PANEL_WIDTH = 80;

    static SquareButtonLookAndFeel& getSquareButtonLookAndFeel() {
        static SquareButtonLookAndFeel laf;
        return laf;
    }

    DeviceSlotComponent(TrackChainContent& owner, magda::TrackId trackId,
                        const magda::DeviceInfo& device)
        : owner_(owner),
          trackId_(trackId),
          device_(device),
          gainSliderVisible_(device.gainPanelOpen),
          modPanelVisible_(device.modPanelOpen),
          paramPanelVisible_(device.paramPanelOpen),
          collapsed_(!device.expanded) {
        // Bypass button
        bypassButton_.setButtonText("B");
        bypassButton_.setColour(juce::TextButton::buttonColourId,
                                DarkTheme::getColour(DarkTheme::SURFACE));
        bypassButton_.setColour(juce::TextButton::buttonOnColourId,
                                DarkTheme::getColour(DarkTheme::STATUS_WARNING));
        bypassButton_.setColour(juce::TextButton::textColourOffId,
                                DarkTheme::getSecondaryTextColour());
        bypassButton_.setColour(juce::TextButton::textColourOnId,
                                DarkTheme::getColour(DarkTheme::BACKGROUND));
        bypassButton_.setClickingTogglesState(true);
        bypassButton_.setToggleState(device_.bypassed, juce::dontSendNotification);
        bypassButton_.onClick = [this]() {
            magda::TrackManager::getInstance().setDeviceBypassed(trackId_, device_.id,
                                                                 bypassButton_.getToggleState());
        };
        addAndMakeVisible(bypassButton_);

        // Modulator toggle button
        modToggleButton_.setButtonText("M");
        modToggleButton_.setColour(juce::TextButton::buttonColourId,
                                   DarkTheme::getColour(DarkTheme::SURFACE));
        modToggleButton_.setColour(juce::TextButton::buttonOnColourId,
                                   DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
        modToggleButton_.setColour(juce::TextButton::textColourOffId,
                                   DarkTheme::getSecondaryTextColour());
        modToggleButton_.setColour(juce::TextButton::textColourOnId,
                                   DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        modToggleButton_.setClickingTogglesState(true);
        modToggleButton_.setToggleState(modPanelVisible_, juce::dontSendNotification);
        modToggleButton_.onClick = [this]() {
            modPanelVisible_ = modToggleButton_.getToggleState();
            // Save state to TrackManager
            if (auto* dev = magda::TrackManager::getInstance().getDevice(trackId_, device_.id)) {
                dev->modPanelOpen = modPanelVisible_;
            }
            resized();
            repaint();
            // Notify parent to re-layout all slots
            owner_.resized();
            owner_.repaint();
        };
        addAndMakeVisible(modToggleButton_);

        // Gain toggle button
        gainToggleButton_.setButtonText("G");
        gainToggleButton_.setColour(juce::TextButton::buttonColourId,
                                    DarkTheme::getColour(DarkTheme::SURFACE));
        gainToggleButton_.setColour(juce::TextButton::buttonOnColourId,
                                    DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        gainToggleButton_.setColour(juce::TextButton::textColourOffId,
                                    DarkTheme::getSecondaryTextColour());
        gainToggleButton_.setColour(juce::TextButton::textColourOnId,
                                    DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        gainToggleButton_.setClickingTogglesState(true);
        gainToggleButton_.setToggleState(gainSliderVisible_, juce::dontSendNotification);
        gainToggleButton_.onClick = [this]() {
            gainSliderVisible_ = gainToggleButton_.getToggleState();
            gainMeter_.setVisible(gainSliderVisible_);
            // Save state to TrackManager
            if (auto* dev = magda::TrackManager::getInstance().getDevice(trackId_, device_.id)) {
                dev->gainPanelOpen = gainSliderVisible_;
            }
            resized();
            repaint();
            // Notify parent to re-layout all slots
            owner_.resized();
            owner_.repaint();
        };
        addAndMakeVisible(gainToggleButton_);

        // Gain meter with text slider - restore dB value from device
        gainMeter_.setGainDb(device_.gainDb, juce::dontSendNotification);
        gainMeter_.setVisible(gainSliderVisible_);
        gainMeter_.onGainChanged = [this](double db) {
            // Save gain dB value to TrackManager
            if (auto* dev = magda::TrackManager::getInstance().getDevice(trackId_, device_.id)) {
                dev->gainDb = static_cast<float>(db);
            }
        };
        if (gainSliderVisible_) {
            addAndMakeVisible(gainMeter_);
        } else {
            addChildComponent(gainMeter_);
        }

        // Parameter toggle button
        paramToggleButton_.setButtonText("P");
        paramToggleButton_.setColour(juce::TextButton::buttonColourId,
                                     DarkTheme::getColour(DarkTheme::SURFACE));
        paramToggleButton_.setColour(juce::TextButton::buttonOnColourId,
                                     DarkTheme::getColour(DarkTheme::ACCENT_PURPLE));
        paramToggleButton_.setColour(juce::TextButton::textColourOffId,
                                     DarkTheme::getSecondaryTextColour());
        paramToggleButton_.setColour(juce::TextButton::textColourOnId,
                                     DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        paramToggleButton_.setClickingTogglesState(true);
        paramToggleButton_.setToggleState(paramPanelVisible_, juce::dontSendNotification);
        paramToggleButton_.onClick = [this]() {
            paramPanelVisible_ = paramToggleButton_.getToggleState();
            // Save state to TrackManager
            if (auto* dev = magda::TrackManager::getInstance().getDevice(trackId_, device_.id)) {
                dev->paramPanelOpen = paramPanelVisible_;
            }
            resized();
            repaint();
            // Notify parent to re-layout all slots
            owner_.resized();
            owner_.repaint();
        };
        addAndMakeVisible(paramToggleButton_);

        // Mock parameter knobs (will be replaced with real params later)
        for (int i = 0; i < 4; ++i) {
            auto knob = std::make_unique<juce::Slider>();
            knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob->setRange(0.0, 1.0, 0.01);
            knob->setValue(0.5);
            knob->setColour(juce::Slider::rotarySliderFillColourId,
                            DarkTheme::getColour(DarkTheme::ACCENT_PURPLE));
            knob->setColour(juce::Slider::rotarySliderOutlineColourId,
                            DarkTheme::getColour(DarkTheme::SURFACE));
            addChildComponent(*knob);
            paramKnobs_.push_back(std::move(knob));
        }

        // Modulator slot buttons (mock - 3 slots)
        for (int i = 0; i < 3; ++i) {
            auto& btn = modSlotButtons_[i];
            btn = std::make_unique<juce::TextButton>("+");
            btn->setColour(juce::TextButton::buttonColourId,
                           DarkTheme::getColour(DarkTheme::SURFACE));
            btn->setColour(juce::TextButton::textColourOffId, DarkTheme::getSecondaryTextColour());
            btn->onClick = [this, i]() {
                // Show modulator type menu
                juce::PopupMenu menu;
                menu.addItem(1, "LFO");
                menu.addItem(2, "Bezier LFO");
                menu.addItem(3, "ADSR");
                menu.addItem(4, "Envelope Follower");
                menu.showMenuAsync(juce::PopupMenu::Options(), [this, i](int result) {
                    if (result > 0) {
                        juce::StringArray types = {"", "LFO", "BEZ", "ADSR", "ENV"};
                        modSlotButtons_[i]->setButtonText(types[result]);
                        DBG("Added modulator type " << result << " to slot " << i);
                    }
                });
            };
            addChildComponent(*btn);
        }

        // UI button (opens plugin editor window)
        uiButton_.setButtonText("U");
        uiButton_.setColour(juce::TextButton::buttonColourId,
                            DarkTheme::getColour(DarkTheme::SURFACE));
        uiButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getSecondaryTextColour());
        uiButton_.onClick = [this]() {
            // TODO: Open plugin UI window
            DBG("Open plugin UI for: " << device_.name);
        };
        addAndMakeVisible(uiButton_);

        // Delete button
        deleteButton_.setButtonText(juce::String::fromUTF8("✕"));
        deleteButton_.setColour(juce::TextButton::buttonColourId,
                                DarkTheme::getColour(DarkTheme::SURFACE));
        deleteButton_.setColour(juce::TextButton::textColourOffId,
                                DarkTheme::getSecondaryTextColour());
        deleteButton_.onClick = [this]() {
            magda::TrackManager::getInstance().removeDeviceFromTrack(trackId_, device_.id);
        };
        addAndMakeVisible(deleteButton_);

        // Apply square button look and feel to all buttons
        bypassButton_.setLookAndFeel(&getSquareButtonLookAndFeel());
        modToggleButton_.setLookAndFeel(&getSquareButtonLookAndFeel());
        paramToggleButton_.setLookAndFeel(&getSquareButtonLookAndFeel());
        gainToggleButton_.setLookAndFeel(&getSquareButtonLookAndFeel());
        uiButton_.setLookAndFeel(&getSquareButtonLookAndFeel());
        deleteButton_.setLookAndFeel(&getSquareButtonLookAndFeel());
    }

    ~DeviceSlotComponent() override {
        // Clear LookAndFeel references
        bypassButton_.setLookAndFeel(nullptr);
        modToggleButton_.setLookAndFeel(nullptr);
        paramToggleButton_.setLookAndFeel(nullptr);
        gainToggleButton_.setLookAndFeel(nullptr);
        uiButton_.setLookAndFeel(nullptr);
        deleteButton_.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();

        // Mod panel on left (visible even when collapsed)
        if (modPanelVisible_) {
            auto modArea = bounds.removeFromLeft(MODULATOR_PANEL_WIDTH);
            g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
            g.fillRoundedRectangle(modArea.toFloat(), 4.0f);
            g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
            g.drawRoundedRectangle(modArea.toFloat(), 4.0f, 1.0f);

            // Draw "Mod" label at top
            g.setColour(DarkTheme::getSecondaryTextColour());
            g.setFont(FontManager::getInstance().getUIFont(8.0f));
            g.drawText("Mod", modArea.removeFromTop(14), juce::Justification::centred);
        }

        // Param panel (between mod and main, visible even when collapsed)
        if (paramPanelVisible_) {
            auto paramArea = bounds.removeFromLeft(PARAM_PANEL_WIDTH);
            g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
            g.fillRoundedRectangle(paramArea.toFloat(), 4.0f);
            g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
            g.drawRoundedRectangle(paramArea.toFloat(), 4.0f, 1.0f);

            // Draw "Params" label at top
            g.setColour(DarkTheme::getSecondaryTextColour());
            g.setFont(FontManager::getInstance().getUIFont(8.0f));
            g.drawText("Params", paramArea.removeFromTop(14), juce::Justification::centred);
        }

        // Gain panel on right (visible even when collapsed)
        if (gainSliderVisible_) {
            bounds.removeFromRight(GAIN_SLIDER_WIDTH);
        }

        // Background for main area
        auto bgColour = device_.bypassed ? DarkTheme::getColour(DarkTheme::SURFACE).withAlpha(0.5f)
                                         : DarkTheme::getColour(DarkTheme::SURFACE);
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Border
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

        if (!collapsed_) {
            // Device name
            auto textBounds = bounds.reduced(6);
            textBounds.removeFromTop(20);     // Skip header row
            textBounds.removeFromBottom(20);  // Skip footer row

            auto textColour = device_.bypassed ? DarkTheme::getSecondaryTextColour().withAlpha(0.5f)
                                               : DarkTheme::getTextColour();
            g.setColour(textColour);
            g.setFont(FontManager::getInstance().getUIFontBold(11.0f));
            g.drawText(device_.name, textBounds, juce::Justification::centred);

            // Manufacturer + format
            auto mfrBounds = textBounds;
            mfrBounds.removeFromTop(16);
            g.setColour(DarkTheme::getSecondaryTextColour());
            g.setFont(FontManager::getInstance().getUIFont(9.0f));
            g.drawText(device_.manufacturer + " - " + device_.getFormatString(), mfrBounds,
                       juce::Justification::centred);
        }
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        collapsed_ = !collapsed_;
        // Save state to TrackManager
        if (auto* dev = magda::TrackManager::getInstance().getDevice(trackId_, device_.id)) {
            dev->expanded = !collapsed_;
        }
        resized();
        repaint();
        // Notify parent to re-layout all slots
        owner_.resized();
        owner_.repaint();
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(4);

        // Modulator panel on the left if visible (always, even when collapsed)
        if (modPanelVisible_) {
            auto modArea = bounds.removeFromLeft(MODULATOR_PANEL_WIDTH - 4);
            modArea.removeFromTop(14);  // Skip "Mod" label
            modArea = modArea.reduced(2);

            int slotHeight = (modArea.getHeight() - 4) / 3;
            for (int i = 0; i < 3; ++i) {
                modSlotButtons_[i]->setBounds(modArea.removeFromTop(slotHeight).reduced(0, 1));
                modSlotButtons_[i]->setVisible(true);
            }
        } else {
            for (auto& btn : modSlotButtons_) {
                btn->setVisible(false);
            }
        }

        // Parameter panel (always, even when collapsed)
        if (paramPanelVisible_) {
            auto paramArea = bounds.removeFromLeft(PARAM_PANEL_WIDTH - 4);
            paramArea.removeFromTop(14);  // Skip "Params" label
            paramArea = paramArea.reduced(2);

            // Layout knobs in a 2x2 grid
            int knobSize = (paramArea.getWidth() - 2) / 2;
            int row = 0, col = 0;
            for (auto& knob : paramKnobs_) {
                int x = paramArea.getX() + col * (knobSize + 2);
                int y = paramArea.getY() + row * (knobSize + 2);
                knob->setBounds(x, y, knobSize, knobSize);
                knob->setVisible(true);
                col++;
                if (col >= 2) {
                    col = 0;
                    row++;
                }
            }
        } else {
            for (auto& knob : paramKnobs_) {
                knob->setVisible(false);
            }
        }

        // Gain meter on the right if visible (always, even when collapsed)
        if (gainSliderVisible_) {
            auto meterArea = bounds.removeFromRight(GAIN_SLIDER_WIDTH - 4);
            gainMeter_.setBounds(meterArea.reduced(2, 2));
            gainMeter_.setVisible(true);
        } else {
            gainMeter_.setVisible(false);
        }

        // Layout buttons for main plugin area
        if (collapsed_) {
            // Collapsed mode: vertical column of buttons
            // Top group: ON, U, X (device controls)
            // Bottom group: M, P, G (panel toggles)
            int buttonSize = 16;
            int spacing = 2;
            int x = bounds.getX() + (bounds.getWidth() - buttonSize) / 2;  // Center horizontally
            int y = bounds.getY();

            // Device controls at top
            bypassButton_.setBounds(x, y, buttonSize, buttonSize);
            y += buttonSize + spacing;
            uiButton_.setBounds(x, y, buttonSize, buttonSize);
            y += buttonSize + spacing;
            deleteButton_.setBounds(x, y, buttonSize, buttonSize);
            y += buttonSize + spacing + 4;  // Extra gap between groups

            // Panel toggles at bottom
            modToggleButton_.setBounds(x, y, buttonSize, buttonSize);
            y += buttonSize + spacing;
            paramToggleButton_.setBounds(x, y, buttonSize, buttonSize);
            y += buttonSize + spacing;
            gainToggleButton_.setBounds(x, y, buttonSize, buttonSize);
        } else {
            // Expanded mode: header and footer layout
            int btnSize = 16;
            int btnSpacing = 2;
            int inset = 6;  // Inset from edges

            // Header: [ON] [U] ... [X]
            auto headerRow = bounds.removeFromTop(18);
            headerRow.removeFromLeft(inset);
            headerRow.removeFromRight(inset);
            bypassButton_.setBounds(headerRow.removeFromLeft(btnSize));
            headerRow.removeFromLeft(btnSpacing);
            uiButton_.setBounds(headerRow.removeFromLeft(btnSize));
            deleteButton_.setBounds(headerRow.removeFromRight(btnSize));

            // Footer: [M] [P] ... [G]
            auto footerRow = bounds.removeFromBottom(18);
            footerRow.removeFromLeft(inset);
            footerRow.removeFromRight(inset);
            modToggleButton_.setBounds(footerRow.removeFromLeft(btnSize));
            footerRow.removeFromLeft(btnSpacing);
            paramToggleButton_.setBounds(footerRow.removeFromLeft(btnSize));
            gainToggleButton_.setBounds(footerRow.removeFromRight(btnSize));
        }
    }

    bool isGainSliderVisible() const {
        return gainSliderVisible_;
    }
    bool isModPanelVisible() const {
        return modPanelVisible_;
    }

    int getExpandedWidth() const {
        int width = collapsed_ ? 36 : 130;  // Collapsed = vertical buttons only, expanded = full
        if (modPanelVisible_)
            width += MODULATOR_PANEL_WIDTH;
        if (paramPanelVisible_)
            width += PARAM_PANEL_WIDTH;
        if (gainSliderVisible_)
            width += GAIN_SLIDER_WIDTH;
        return width;
    }

    bool isCollapsed() const {
        return collapsed_;
    }

  private:
    TrackChainContent& owner_;
    magda::TrackId trackId_;
    magda::DeviceInfo device_;
    juce::TextButton bypassButton_;
    juce::TextButton modToggleButton_;
    juce::TextButton paramToggleButton_;
    juce::TextButton gainToggleButton_;
    juce::TextButton uiButton_;
    juce::TextButton deleteButton_;
    GainMeterComponent gainMeter_;
    std::unique_ptr<juce::TextButton> modSlotButtons_[3];
    std::vector<std::unique_ptr<juce::Slider>> paramKnobs_;
    bool gainSliderVisible_ = false;
    bool modPanelVisible_ = false;
    bool paramPanelVisible_ = false;
    bool collapsed_ = false;
};

// dB conversion helpers
namespace {
constexpr float MIN_DB = -60.0f;
constexpr float MAX_DB = 6.0f;
constexpr float UNITY_DB = 0.0f;

float gainToDb(float gain) {
    if (gain <= 0.0f)
        return MIN_DB;
    return 20.0f * std::log10(gain);
}

float dbToGain(float db) {
    if (db <= MIN_DB)
        return 0.0f;
    return std::pow(10.0f, db / 20.0f);
}

float dbToFaderPos(float db) {
    if (db <= MIN_DB)
        return 0.0f;
    if (db >= MAX_DB)
        return 1.0f;

    if (db < UNITY_DB) {
        return 0.75f * (db - MIN_DB) / (UNITY_DB - MIN_DB);
    } else {
        return 0.75f + 0.25f * (db - UNITY_DB) / (MAX_DB - UNITY_DB);
    }
}

float faderPosToDb(float pos) {
    if (pos <= 0.0f)
        return MIN_DB;
    if (pos >= 1.0f)
        return MAX_DB;

    if (pos < 0.75f) {
        return MIN_DB + (pos / 0.75f) * (UNITY_DB - MIN_DB);
    } else {
        return UNITY_DB + ((pos - 0.75f) / 0.25f) * (MAX_DB - UNITY_DB);
    }
}
}  // namespace

TrackChainContent::TrackChainContent() {
    setName("Track Chain");

    // No selection label
    noSelectionLabel_.setText("Select a track to view its signal chain",
                              juce::dontSendNotification);
    noSelectionLabel_.setFont(FontManager::getInstance().getUIFont(12.0f));
    noSelectionLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    noSelectionLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(noSelectionLabel_);

    // Track name at right strip
    trackNameLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    trackNameLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    trackNameLabel_.setJustificationType(juce::Justification::centredLeft);
    addChildComponent(trackNameLabel_);

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
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            magda::TrackManager::getInstance().setTrackMuted(selectedTrackId_,
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
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            magda::TrackManager::getInstance().setTrackSoloed(selectedTrackId_,
                                                              soloButton_.getToggleState());
        }
    };
    addChildComponent(soloButton_);

    // Gain slider - using dB scale with unity at 0.75 position
    gainSlider_.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainSlider_.setRange(0.0, 1.0, 0.001);
    gainSlider_.setValue(0.75);  // Unity gain (0 dB)
    gainSlider_.setSliderSnapsToMousePosition(false);
    gainSlider_.setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    gainSlider_.setColour(juce::Slider::backgroundColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    gainSlider_.setColour(juce::Slider::thumbColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    gainSlider_.setLookAndFeel(&mixerLookAndFeel_);
    gainSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            float faderPos = static_cast<float>(gainSlider_.getValue());
            float db = faderPosToDb(faderPos);
            float gain = dbToGain(db);
            magda::TrackManager::getInstance().setTrackVolume(selectedTrackId_, gain);
            // Update gain label
            juce::String dbText;
            if (db <= MIN_DB) {
                dbText = "-inf";
            } else {
                dbText = juce::String(db, 1) + " dB";
            }
            gainValueLabel_.setText(dbText, juce::dontSendNotification);
        }
    };
    addChildComponent(gainSlider_);

    // Gain value label
    gainValueLabel_.setText("0.0 dB", juce::dontSendNotification);
    gainValueLabel_.setJustificationType(juce::Justification::centred);
    gainValueLabel_.setColour(juce::Label::textColourId,
                              DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
    gainValueLabel_.setFont(FontManager::getInstance().getUIFont(9.0f));
    addChildComponent(gainValueLabel_);

    // Pan slider (rotary knob)
    panSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider_.setRange(-1.0, 1.0, 0.01);
    panSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                         DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    panSlider_.setColour(juce::Slider::rotarySliderOutlineColourId,
                         DarkTheme::getColour(DarkTheme::SURFACE));
    panSlider_.setLookAndFeel(&mixerLookAndFeel_);
    panSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            magda::TrackManager::getInstance().setTrackPan(
                selectedTrackId_, static_cast<float>(panSlider_.getValue()));
            // Update pan label
            float pan = static_cast<float>(panSlider_.getValue());
            juce::String panText;
            if (std::abs(pan) < 0.01f) {
                panText = "C";
            } else if (pan < 0) {
                panText = juce::String(static_cast<int>(std::abs(pan) * 100)) + "L";
            } else {
                panText = juce::String(static_cast<int>(pan * 100)) + "R";
            }
            panValueLabel_.setText(panText, juce::dontSendNotification);
        }
    };
    addChildComponent(panSlider_);

    // Pan value label
    panValueLabel_.setText("C", juce::dontSendNotification);
    panValueLabel_.setJustificationType(juce::Justification::centred);
    panValueLabel_.setColour(juce::Label::textColourId,
                             DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
    panValueLabel_.setFont(FontManager::getInstance().getUIFont(10.0f));
    addChildComponent(panValueLabel_);

    // Add device button
    addDeviceButton_.setButtonText("+");
    addDeviceButton_.setColour(juce::TextButton::buttonColourId,
                               DarkTheme::getColour(DarkTheme::SURFACE));
    addDeviceButton_.setColour(juce::TextButton::textColourOffId,
                               DarkTheme::getSecondaryTextColour());
    addDeviceButton_.onClick = [this]() {
        // Would open plugin browser or show plugin selector
        DBG("Add device clicked - would show plugin selector");
    };
    addChildComponent(addDeviceButton_);

    // Register as listener
    magda::TrackManager::getInstance().addListener(this);

    // Check if there's already a selected track
    selectedTrackId_ = magda::TrackManager::getInstance().getSelectedTrack();
    updateFromSelectedTrack();
}

TrackChainContent::~TrackChainContent() {
    magda::TrackManager::getInstance().removeListener(this);
    // Clear look and feel before destruction
    gainSlider_.setLookAndFeel(nullptr);
    panSlider_.setLookAndFeel(nullptr);
}

void TrackChainContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());

    if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
        // Draw the chain area background
        auto bounds = getLocalBounds();
        auto stripWidth = 100;
        auto chainArea = bounds.withTrimmedRight(stripWidth);

        // Draw arrows between device slots
        auto slotArea = chainArea.reduced(8);
        int slotSpacing = 8;
        int arrowWidth = 20;

        int x = slotArea.getX();
        for (size_t i = 0; i < deviceSlots_.size(); ++i) {
            int slotWidth = deviceSlots_[i]->getExpandedWidth();
            x += slotWidth;  // After device slot

            // Draw arrow after each device (except the last one before add button)
            auto arrowArea =
                juce::Rectangle<int>(x, slotArea.getY(), arrowWidth, slotArea.getHeight());
            g.setColour(DarkTheme::getSecondaryTextColour());

            int arrowY = arrowArea.getCentreY();
            int arrowX = arrowArea.getCentreX();
            g.drawLine(static_cast<float>(arrowX - 6), static_cast<float>(arrowY),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);
            // Arrow head
            g.drawLine(static_cast<float>(arrowX + 2), static_cast<float>(arrowY - 4),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);
            g.drawLine(static_cast<float>(arrowX + 2), static_cast<float>(arrowY + 4),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);

            x += arrowWidth + slotSpacing;
        }

        // Draw separator line before track strip
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawLine(static_cast<float>(chainArea.getRight()), 0.0f,
                   static_cast<float>(chainArea.getRight()), static_cast<float>(getHeight()), 1.0f);
    }
}

void TrackChainContent::resized() {
    auto bounds = getLocalBounds();
    const auto& metrics = magda::MixerMetrics::getInstance();

    if (selectedTrackId_ == magda::INVALID_TRACK_ID) {
        noSelectionLabel_.setBounds(bounds);
        addDeviceButton_.setVisible(false);
    } else {
        // Track info strip at right border
        auto stripWidth = 100;
        auto strip = bounds.removeFromRight(stripWidth).reduced(4);

        // Chain area (left of strip)
        auto chainArea = bounds.reduced(8);

        // Layout device slots horizontally
        int slotHeight = chainArea.getHeight();
        int arrowWidth = 20;
        int slotSpacing = 8;

        int x = chainArea.getX();
        for (auto& slot : deviceSlots_) {
            int slotWidth = slot->getExpandedWidth();
            slot->setBounds(x, chainArea.getY(), slotWidth, slotHeight);
            x += slotWidth + arrowWidth + slotSpacing;
        }

        // Add device button after all slots
        addDeviceButton_.setBounds(x, chainArea.getY(), 40, slotHeight);
        addDeviceButton_.setVisible(true);

        // Track name at top of strip
        trackNameLabel_.setBounds(strip.removeFromTop(20));
        strip.removeFromTop(4);

        // Pan knob
        auto panArea = strip.removeFromTop(metrics.knobSize);
        panSlider_.setBounds(panArea.withSizeKeepingCentre(metrics.knobSize, metrics.knobSize));

        // Pan value label
        panValueLabel_.setBounds(strip.removeFromTop(14));
        strip.removeFromTop(4);

        // M/S buttons
        auto buttonRow = strip.removeFromTop(24);
        muteButton_.setBounds(buttonRow.removeFromLeft(36));
        buttonRow.removeFromLeft(4);
        soloButton_.setBounds(buttonRow.removeFromLeft(36));
        strip.removeFromTop(4);

        // Gain value label
        gainValueLabel_.setBounds(strip.removeFromTop(12));

        // Gain slider (vertical) - takes remaining space
        gainSlider_.setBounds(strip);
    }
}

void TrackChainContent::onActivated() {
    selectedTrackId_ = magda::TrackManager::getInstance().getSelectedTrack();
    updateFromSelectedTrack();
}

void TrackChainContent::onDeactivated() {
    // Nothing to do
}

void TrackChainContent::tracksChanged() {
    if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
        const auto* track = magda::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (!track) {
            selectedTrackId_ = magda::INVALID_TRACK_ID;
            updateFromSelectedTrack();
        }
    }
}

void TrackChainContent::trackPropertyChanged(int trackId) {
    if (static_cast<magda::TrackId>(trackId) == selectedTrackId_) {
        updateFromSelectedTrack();
    }
}

void TrackChainContent::trackSelectionChanged(magda::TrackId trackId) {
    selectedTrackId_ = trackId;
    updateFromSelectedTrack();
}

void TrackChainContent::trackDevicesChanged(magda::TrackId trackId) {
    if (trackId == selectedTrackId_) {
        rebuildDeviceSlots();
    }
}

void TrackChainContent::updateFromSelectedTrack() {
    if (selectedTrackId_ == magda::INVALID_TRACK_ID) {
        showTrackStrip(false);
        noSelectionLabel_.setVisible(true);
        deviceSlots_.clear();
    } else {
        const auto* track = magda::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (track) {
            trackNameLabel_.setText(track->name, juce::dontSendNotification);
            muteButton_.setToggleState(track->muted, juce::dontSendNotification);
            soloButton_.setToggleState(track->soloed, juce::dontSendNotification);

            // Convert linear gain to fader position
            float db = gainToDb(track->volume);
            float faderPos = dbToFaderPos(db);
            gainSlider_.setValue(faderPos, juce::dontSendNotification);

            // Update gain label
            juce::String dbText;
            if (db <= MIN_DB) {
                dbText = "-inf";
            } else {
                dbText = juce::String(db, 1) + " dB";
            }
            gainValueLabel_.setText(dbText, juce::dontSendNotification);

            panSlider_.setValue(track->pan, juce::dontSendNotification);

            // Update pan label
            float pan = track->pan;
            juce::String panText;
            if (std::abs(pan) < 0.01f) {
                panText = "C";
            } else if (pan < 0) {
                panText = juce::String(static_cast<int>(std::abs(pan) * 100)) + "L";
            } else {
                panText = juce::String(static_cast<int>(pan * 100)) + "R";
            }
            panValueLabel_.setText(panText, juce::dontSendNotification);

            showTrackStrip(true);
            noSelectionLabel_.setVisible(false);
            rebuildDeviceSlots();
        } else {
            showTrackStrip(false);
            noSelectionLabel_.setVisible(true);
            deviceSlots_.clear();
        }
    }

    resized();
    repaint();
}

void TrackChainContent::showTrackStrip(bool show) {
    trackNameLabel_.setVisible(show);
    muteButton_.setVisible(show);
    soloButton_.setVisible(show);
    gainSlider_.setVisible(show);
    gainValueLabel_.setVisible(show);
    panSlider_.setVisible(show);
    panValueLabel_.setVisible(show);
}

void TrackChainContent::rebuildDeviceSlots() {
    // Remove existing slots
    deviceSlots_.clear();

    if (selectedTrackId_ == magda::INVALID_TRACK_ID) {
        return;
    }

    const auto* devices = magda::TrackManager::getInstance().getDevices(selectedTrackId_);
    if (!devices) {
        return;
    }

    // Create a slot component for each device
    for (const auto& device : *devices) {
        auto slot = std::make_unique<DeviceSlotComponent>(*this, selectedTrackId_, device);
        addAndMakeVisible(*slot);
        deviceSlots_.push_back(std::move(slot));
    }

    resized();
    repaint();
}

}  // namespace magda::daw::ui
