#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

#include "core/DeviceInfo.hpp"

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

    // Show dialog for a specific plugin (loads real parameters)
    static void showForPlugin(const juce::String& uniqueId, const juce::String& pluginName,
                              juce::Component* parent);

    // Load saved parameter configuration and apply to DeviceInfo
    static bool applyConfigToDevice(const juce::String& uniqueId, magda::DeviceInfo& device);

  private:
    juce::String pluginName_;
    juce::String pluginUniqueId_;  // For saving/loading parameter configuration
    std::vector<MockParameterInfo> parameters_;
    std::vector<int> filteredIndices_;  // Indices of filtered parameters
    juce::String currentSearchText_;

    juce::TableListBox table_;
    juce::TextButton okButton_;
    juce::TextButton cancelButton_;
    juce::TextButton applyButton_;
    juce::TextButton selectAllButton_;
    juce::TextButton deselectAllButton_;
    juce::Label titleLabel_;
    juce::TextEditor searchBox_;
    juce::Label searchLabel_;

    // Column IDs
    enum ColumnIds { ParamName = 1, Visible, Unit, RangeMin, RangeMax, RangeCenter, UseAsGain };

    void buildMockParameters();
    void loadParameters(const juce::String& uniqueId);
    bool isLikelyGainParameter(const juce::String& name);
    void saveParameterConfiguration();
    void loadParameterConfiguration();
    void selectAllParameters();
    void deselectAllParameters();
    void filterParameters(const juce::String& searchText);
    void rebuildFilteredList();
    int getParamIndexForRow(int row) const;

    // Custom cell components
    class ToggleCell;
    class ComboCell;
    class TextCell;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterConfigDialog)
};

}  // namespace magda::daw::ui
