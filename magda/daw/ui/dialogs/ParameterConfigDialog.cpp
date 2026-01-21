#include "ParameterConfigDialog.hpp"

#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magda::daw::ui {

//==============================================================================
// ToggleCell - Checkbox cell for visible/use as gain columns
//==============================================================================
class ParameterConfigDialog::ToggleCell : public juce::Component {
  public:
    ToggleCell(ParameterConfigDialog& owner, int row, int column)
        : owner_(owner), row_(row), column_(column) {
        toggle_.setColour(juce::ToggleButton::tickColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        toggle_.setColour(juce::ToggleButton::tickDisabledColourId,
                          DarkTheme::getColour(DarkTheme::TEXT_DIM));
        toggle_.onClick = [this]() {
            if (row_ < static_cast<int>(owner_.parameters_.size())) {
                if (column_ == ColumnIds::Visible) {
                    owner_.parameters_[row_].isVisible = toggle_.getToggleState();
                } else if (column_ == ColumnIds::UseAsGain) {
                    // Uncheck all others first (only one gain stage)
                    for (auto& p : owner_.parameters_) {
                        p.useAsGain = false;
                    }
                    owner_.parameters_[row_].useAsGain = toggle_.getToggleState();
                    // Force refresh all cells to update enabled states
                    owner_.table_.updateContent();
                }
            }
        };
        addAndMakeVisible(toggle_);
    }

    void update(int row, int column) {
        row_ = row;
        column_ = column;
        if (row_ < static_cast<int>(owner_.parameters_.size())) {
            const auto& param = owner_.parameters_[row_];
            if (column_ == ColumnIds::Visible) {
                toggle_.setToggleState(param.isVisible, juce::dontSendNotification);
                toggle_.setEnabled(true);
                toggle_.setVisible(true);
            } else if (column_ == ColumnIds::UseAsGain) {
                toggle_.setToggleState(param.useAsGain, juce::dontSendNotification);
                // Check if another parameter is already selected as gain
                bool anotherIsGain = false;
                for (const auto& p : owner_.parameters_) {
                    if (p.useAsGain && &p != &param) {
                        anotherIsGain = true;
                        break;
                    }
                }
                // Show only if: canBeGain AND (this is the selected one OR none is selected)
                bool canSelect = param.canBeGain && (!anotherIsGain || param.useAsGain);
                toggle_.setVisible(canSelect);
                toggle_.setEnabled(canSelect);
            }
        }
    }

    void resized() override {
        toggle_.setBounds(getLocalBounds().reduced(4));
    }

  private:
    ParameterConfigDialog& owner_;
    int row_;
    int column_;
    juce::ToggleButton toggle_;
};

//==============================================================================
// ComboCell - Dropdown cell for unit selection
//==============================================================================
class ParameterConfigDialog::ComboCell : public juce::Component {
  public:
    ComboCell(ParameterConfigDialog& owner, int row) : owner_(owner), row_(row) {
        combo_.addItem("%", 1);
        combo_.addItem("Hz", 2);
        combo_.addItem("dB", 3);
        combo_.addItem("ms", 4);
        combo_.addItem("semitones", 5);
        combo_.addItem("custom", 6);

        combo_.setColour(juce::ComboBox::backgroundColourId,
                         DarkTheme::getColour(DarkTheme::SURFACE));
        combo_.setColour(juce::ComboBox::textColourId, DarkTheme::getTextColour());
        combo_.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

        combo_.onChange = [this]() {
            if (row_ < static_cast<int>(owner_.parameters_.size())) {
                owner_.parameters_[row_].unit = combo_.getText();
            }
        };
        addAndMakeVisible(combo_);
    }

    void update(int row) {
        row_ = row;
        if (row_ < static_cast<int>(owner_.parameters_.size())) {
            const auto& param = owner_.parameters_[row_];
            // Find matching item
            for (int i = 0; i < combo_.getNumItems(); ++i) {
                if (combo_.getItemText(i) == param.unit) {
                    combo_.setSelectedItemIndex(i, juce::dontSendNotification);
                    return;
                }
            }
            combo_.setSelectedId(1, juce::dontSendNotification);  // Default to %
        }
    }

    void resized() override {
        combo_.setBounds(getLocalBounds().reduced(2));
    }

  private:
    ParameterConfigDialog& owner_;
    int row_;
    juce::ComboBox combo_;
};

//==============================================================================
// TextCell - Editable text cell for range values
//==============================================================================
class ParameterConfigDialog::TextCell : public juce::Component {
  public:
    TextCell(ParameterConfigDialog& owner, int row, int column)
        : owner_(owner), row_(row), column_(column) {
        editor_.setColour(juce::TextEditor::backgroundColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
        editor_.setColour(juce::TextEditor::textColourId, DarkTheme::getTextColour());
        editor_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        editor_.setJustification(juce::Justification::centred);
        editor_.setFont(FontManager::getInstance().getUIFont(11.0f));

        editor_.onFocusLost = [this]() { commitValue(); };
        editor_.onReturnKey = [this]() { commitValue(); };

        addAndMakeVisible(editor_);
    }

    void update(int row, int column) {
        row_ = row;
        column_ = column;
        if (row_ < static_cast<int>(owner_.parameters_.size())) {
            const auto& param = owner_.parameters_[row_];
            float value = 0.0f;
            switch (column_) {
                case ColumnIds::RangeMin:
                    value = param.rangeMin;
                    break;
                case ColumnIds::RangeMax:
                    value = param.rangeMax;
                    break;
                case ColumnIds::RangeCenter:
                    value = param.rangeCenter;
                    break;
            }
            editor_.setText(juce::String(value, 2), juce::dontSendNotification);
        }
    }

    void resized() override {
        editor_.setBounds(getLocalBounds().reduced(2));
    }

  private:
    void commitValue() {
        if (row_ < static_cast<int>(owner_.parameters_.size())) {
            float value = editor_.getText().getFloatValue();
            switch (column_) {
                case ColumnIds::RangeMin:
                    owner_.parameters_[row_].rangeMin = value;
                    break;
                case ColumnIds::RangeMax:
                    owner_.parameters_[row_].rangeMax = value;
                    break;
                case ColumnIds::RangeCenter:
                    owner_.parameters_[row_].rangeCenter = value;
                    break;
            }
        }
    }

    ParameterConfigDialog& owner_;
    int row_;
    int column_;
    juce::TextEditor editor_;
};

//==============================================================================
// ParameterConfigDialog
//==============================================================================
ParameterConfigDialog::ParameterConfigDialog(const juce::String& pluginName)
    : pluginName_(pluginName) {
    // Title
    titleLabel_.setText("Configure Parameters - " + pluginName_, juce::dontSendNotification);
    titleLabel_.setFont(FontManager::getInstance().getUIFontBold(14.0f));
    titleLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addAndMakeVisible(titleLabel_);

    // Setup table
    table_.setModel(this);
    table_.setColour(juce::ListBox::backgroundColourId,
                     DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
    table_.setColour(juce::ListBox::outlineColourId, DarkTheme::getBorderColour());
    table_.setOutlineThickness(1);
    table_.setRowHeight(28);

    auto& header = table_.getHeader();
    header.addColumn("Parameter", ParamName, 150, 100, 300);
    header.addColumn("Visible", Visible, 60, 60, 60);
    header.addColumn("Unit", Unit, 80, 60, 100);
    header.addColumn("Min", RangeMin, 60, 50, 80);
    header.addColumn("Max", RangeMax, 60, 50, 80);
    header.addColumn("Center", RangeCenter, 60, 50, 80);
    header.addColumn("Gain", UseAsGain, 50, 50, 50);

    header.setColour(juce::TableHeaderComponent::backgroundColourId,
                     DarkTheme::getColour(DarkTheme::SURFACE));
    header.setColour(juce::TableHeaderComponent::textColourId, DarkTheme::getTextColour());

    addAndMakeVisible(table_);

    // Buttons
    okButton_.setButtonText("OK");
    okButton_.setColour(juce::TextButton::buttonColourId,
                        DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    okButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    okButton_.onClick = [this]() {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>()) {
            dw->exitModalState(1);
        }
    };
    addAndMakeVisible(okButton_);

    cancelButton_.setButtonText("Cancel");
    cancelButton_.setColour(juce::TextButton::buttonColourId,
                            DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    cancelButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getTextColour());
    cancelButton_.onClick = [this]() {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>()) {
            dw->exitModalState(0);
        }
    };
    addAndMakeVisible(cancelButton_);

    applyButton_.setButtonText("Apply");
    applyButton_.setColour(juce::TextButton::buttonColourId,
                           DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    applyButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getTextColour());
    applyButton_.onClick = [this]() {
        // Would save settings here
        DBG("Apply parameter config");
    };
    addAndMakeVisible(applyButton_);

    // Build mock data
    buildMockParameters();

    setSize(620, 500);
}

void ParameterConfigDialog::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
}

void ParameterConfigDialog::resized() {
    auto bounds = getLocalBounds().reduced(16);

    // Title at top
    titleLabel_.setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(12);

    // Buttons at bottom
    auto buttonRow = bounds.removeFromBottom(32);
    bounds.removeFromBottom(12);

    const int buttonWidth = 80;
    const int buttonSpacing = 8;

    okButton_.setBounds(buttonRow.removeFromRight(buttonWidth));
    buttonRow.removeFromRight(buttonSpacing);
    applyButton_.setBounds(buttonRow.removeFromRight(buttonWidth));
    buttonRow.removeFromRight(buttonSpacing);
    cancelButton_.setBounds(buttonRow.removeFromRight(buttonWidth));

    // Table takes remaining space
    table_.setBounds(bounds);
}

int ParameterConfigDialog::getNumRows() {
    return static_cast<int>(parameters_.size());
}

void ParameterConfigDialog::paintRowBackground(juce::Graphics& g, int rowNumber, int width,
                                               int height, bool rowIsSelected) {
    if (rowIsSelected) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE).withAlpha(0.2f));
    } else if (rowNumber % 2 == 0) {
        g.setColour(DarkTheme::getColour(DarkTheme::SURFACE).withAlpha(0.3f));
    } else {
        g.setColour(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
    }
    g.fillRect(0, 0, width, height);
}

void ParameterConfigDialog::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width,
                                      int height, bool /*rowIsSelected*/) {
    if (rowNumber >= static_cast<int>(parameters_.size()))
        return;

    const auto& param = parameters_[rowNumber];

    g.setColour(DarkTheme::getTextColour());
    g.setFont(FontManager::getInstance().getUIFont(11.0f));

    if (columnId == ParamName) {
        // Draw parameter name with gain indicator if applicable
        juce::String text = param.name;
        if (param.canBeGain) {
            g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
            if (param.useAsGain) {
                text = juce::String::fromUTF8("◉ ") + text;
            }
        }
        g.drawText(text, 8, 0, width - 16, height, juce::Justification::centredLeft);
    }
    // Other columns use custom components
}

juce::Component* ParameterConfigDialog::refreshComponentForCell(int rowNumber, int columnId,
                                                                bool /*isRowSelected*/,
                                                                Component* existingComponent) {
    if (rowNumber >= static_cast<int>(parameters_.size()))
        return nullptr;

    // ParamName column is painted, not a component
    if (columnId == ParamName)
        return nullptr;

    if (columnId == Visible || columnId == UseAsGain) {
        auto* toggle = dynamic_cast<ToggleCell*>(existingComponent);
        if (toggle == nullptr) {
            toggle = new ToggleCell(*this, rowNumber, columnId);
        }
        toggle->update(rowNumber, columnId);
        return toggle;
    }

    if (columnId == Unit) {
        auto* combo = dynamic_cast<ComboCell*>(existingComponent);
        if (combo == nullptr) {
            combo = new ComboCell(*this, rowNumber);
        }
        combo->update(rowNumber);
        return combo;
    }

    if (columnId == RangeMin || columnId == RangeMax || columnId == RangeCenter) {
        auto* text = dynamic_cast<TextCell*>(existingComponent);
        if (text == nullptr) {
            text = new TextCell(*this, rowNumber, columnId);
        }
        text->update(rowNumber, columnId);
        return text;
    }

    return nullptr;
}

void ParameterConfigDialog::buildMockParameters() {
    // Mock parameters that might be in a typical plugin like FabFilter Pro-Q 3
    parameters_ = {
        {"Output Gain", 0.5f, true, "dB", -30.0f, 30.0f, 0.0f, false, true},
        {"Mix", 1.0f, true, "%", 0.0f, 100.0f, 50.0f, false, true},
        {"Band 1 Frequency", 0.3f, true, "Hz", 20.0f, 20000.0f, 1000.0f, false, false},
        {"Band 1 Gain", 0.5f, true, "dB", -30.0f, 30.0f, 0.0f, false, true},
        {"Band 1 Q", 0.5f, true, "%", 0.1f, 10.0f, 1.0f, false, false},
        {"Band 1 Type", 0.0f, true, "%", 0.0f, 1.0f, 0.5f, false, false},
        {"Band 2 Frequency", 0.5f, true, "Hz", 20.0f, 20000.0f, 1000.0f, false, false},
        {"Band 2 Gain", 0.5f, true, "dB", -30.0f, 30.0f, 0.0f, false, true},
        {"Band 2 Q", 0.5f, true, "%", 0.1f, 10.0f, 1.0f, false, false},
        {"Band 3 Frequency", 0.7f, true, "Hz", 20.0f, 20000.0f, 1000.0f, false, false},
        {"Band 3 Gain", 0.5f, true, "dB", -30.0f, 30.0f, 0.0f, false, true},
        {"Band 3 Q", 0.5f, true, "%", 0.1f, 10.0f, 1.0f, false, false},
        {"Analyzer Mode", 0.0f, false, "%", 0.0f, 1.0f, 0.5f, false, false},
        {"Auto Gain", 0.0f, true, "%", 0.0f, 1.0f, 0.5f, false, false},
        {"Master Level", 0.8f, true, "dB", -60.0f, 12.0f, 0.0f, false, true},
    };

    // Run sanity check
    for (auto& param : parameters_) {
        param.canBeGain = isLikelyGainParameter(param.name);
    }
}

bool ParameterConfigDialog::isLikelyGainParameter(const juce::String& name) {
    auto lower = name.toLowerCase();
    return lower.contains("gain") || lower.contains("volume") || lower.contains("output") ||
           lower.contains("level") || lower.contains("master") || lower.contains("mix");
}

void ParameterConfigDialog::show(const juce::String& pluginName, juce::Component* /*parent*/) {
    auto* dialog = new ParameterConfigDialog(pluginName);

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Parameter Configuration";
    options.dialogBackgroundColour = DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND);
    options.content.setOwned(dialog);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;

    options.launchAsync();
}

}  // namespace magda::daw::ui
