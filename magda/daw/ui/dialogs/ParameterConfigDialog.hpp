#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace magda::daw::ui {

/**
 * @brief Mock parameter info for UI mockup
 */
struct MockParameterInfo {
    juce::String name;
    float defaultValue = 0.5f;
    bool isVisible = true;
    juce::String unit;  // Hz, dB, ms, %, semitones, custom
    float rangeMin = 0.0f;
    float rangeMax = 1.0f;
    float rangeCenter = 0.5f;
    bool useAsGain = false;
    bool canBeGain = false;  // Sanity check result
};

/**
 * @brief Dialog for configuring plugin parameters
 *
 * Shows a table with columns:
 * - Parameter name
 * - Visible toggle
 * - Custom unit
 * - Custom range (min/max/center)
 * - Use as gain stage
 */
class ParameterConfigDialog : public juce::Component, public juce::TableListBoxModel {
  public:
    ParameterConfigDialog(const juce::String& pluginName);
    ~ParameterConfigDialog() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // TableListBoxModel interface
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height,
                            bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height,
                   bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected,
                                       Component* existingComponentToUpdate) override;

    // Show dialog modally
    static void show(const juce::String& pluginName, juce::Component* parent);

  private:
    juce::String pluginName_;
    std::vector<MockParameterInfo> parameters_;

    juce::TableListBox table_;
    juce::TextButton okButton_;
    juce::TextButton cancelButton_;
    juce::TextButton applyButton_;
    juce::Label titleLabel_;

    // Column IDs
    enum ColumnIds { ParamName = 1, Visible, Unit, RangeMin, RangeMax, RangeCenter, UseAsGain };

    void buildMockParameters();
    bool isLikelyGainParameter(const juce::String& name);

    // Custom cell components
    class ToggleCell;
    class ComboCell;
    class TextCell;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterConfigDialog)
};

}  // namespace magda::daw::ui
