#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PanelContent.hpp"

namespace magda::daw::ui {

/**
 * @brief Mock plugin info for UI mockup
 */
struct MockPluginInfo {
    juce::String name;
    juce::String manufacturer;
    juce::String category;     // Instrument, Effect, etc.
    juce::String format;       // VST3, AU, etc.
    juce::String subcategory;  // EQ, Compressor, Synth, etc.
    bool isFavorite = false;
};

/**
 * @brief Plugin browser panel content
 *
 * Displays a tree view of available plugins organized by category,
 * with search functionality and right-click parameter configuration.
 */
class PluginBrowserContent : public PanelContent, public juce::TreeViewItem {
  public:
    PluginBrowserContent();
    ~PluginBrowserContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::PluginBrowser;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::PluginBrowser, "Plugins", "Browse and insert plugins", "Plugin"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

    // TreeViewItem interface (for root item only)
    bool mightContainSubItems() override {
        return true;
    }

  private:
    // UI Components
    juce::TextEditor searchBox_;
    juce::TreeView pluginTree_;
    juce::ComboBox viewModeSelector_;
    juce::TextButton scanButton_;

    // View modes
    enum class ViewMode {
        ByCategory,      // Instruments, Effects
        ByManufacturer,  // Grouped by vendor
        ByFormat,        // VST3, AU
        Favorites
    };
    ViewMode currentViewMode_ = ViewMode::ByCategory;

    // Mock data
    std::vector<MockPluginInfo> mockPlugins_;

    // Tree building
    void buildMockPluginList();
    void rebuildTree();
    void filterBySearch(const juce::String& searchText);

    // Context menu
    void showPluginContextMenu(const MockPluginInfo& plugin, juce::Point<int> position);
    void showParameterConfigDialog(const MockPluginInfo& plugin);

    class PluginTreeItem;
    class CategoryTreeItem;

    std::unique_ptr<juce::TreeViewItem> rootItem_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserContent)
};

}  // namespace magda::daw::ui
