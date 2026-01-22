#include "MacroKnobComponent.hpp"

#include "ui/themes/DarkTheme.hpp"
#include "ui/themes/FontManager.hpp"

namespace magda::daw::ui {

MacroKnobComponent::MacroKnobComponent(int macroIndex) : macroIndex_(macroIndex) {
    // Initialize macro with default values
    currentMacro_ = magda::MacroInfo(macroIndex);

    // Name label - editable on double-click
    nameLabel_.setText(currentMacro_.name, juce::dontSendNotification);
    nameLabel_.setFont(FontManager::getInstance().getUIFont(8.0f));
    nameLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    nameLabel_.setJustificationType(juce::Justification::centred);
    nameLabel_.setEditable(false, true, false);  // Single-click doesn't edit, double-click does
    nameLabel_.onTextChange = [this]() { onNameLabelEdited(); };
    // Pass single clicks through to parent for selection (double-click still edits)
    nameLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(nameLabel_);

    // Value slider
    valueSlider_.setRange(0.0, 1.0, 0.01);
    valueSlider_.setValue(currentMacro_.value, juce::dontSendNotification);
    valueSlider_.setFont(FontManager::getInstance().getUIFont(9.0f));
    valueSlider_.onValueChanged = [this](double value) {
        currentMacro_.value = static_cast<float>(value);
        if (onValueChanged) {
            onValueChanged(currentMacro_.value);
        }
    };
    addAndMakeVisible(valueSlider_);
}

void MacroKnobComponent::setMacroInfo(const magda::MacroInfo& macro) {
    currentMacro_ = macro;
    nameLabel_.setText(macro.name, juce::dontSendNotification);
    valueSlider_.setValue(macro.value, juce::dontSendNotification);
    repaint();  // Update link indicator
}

void MacroKnobComponent::setAvailableTargets(
    const std::vector<std::pair<magda::DeviceId, juce::String>>& devices) {
    availableTargets_ = devices;
}

void MacroKnobComponent::setSelected(bool selected) {
    if (selected_ != selected) {
        selected_ = selected;
        repaint();
    }
}

void MacroKnobComponent::paint(juce::Graphics& g) {
    // Background - highlight when selected (purple for macros)
    if (selected_) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_PURPLE).withAlpha(0.3f));
    } else {
        g.setColour(DarkTheme::getColour(DarkTheme::SURFACE).brighter(0.04f));
    }
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);

    // Border - purple when selected
    if (selected_) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_PURPLE));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 3.0f, 2.0f);
    } else {
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 3.0f, 1.0f);
    }

    // Link indicator at bottom
    auto linkArea = getLocalBounds().removeFromBottom(LINK_INDICATOR_HEIGHT);
    paintLinkIndicator(g, linkArea);
}

void MacroKnobComponent::resized() {
    auto bounds = getLocalBounds().reduced(1);

    // Name label at top
    nameLabel_.setBounds(bounds.removeFromTop(NAME_LABEL_HEIGHT));

    // Value slider
    bounds.removeFromTop(1);
    valueSlider_.setBounds(bounds.removeFromTop(VALUE_SLIDER_HEIGHT));

    // Link indicator area (painted, not a component)
    // bounds.removeFromTop(LINK_INDICATOR_HEIGHT) is handled in paint
}

void MacroKnobComponent::mouseDown(const juce::MouseEvent& e) {
    if (!e.mods.isPopupMenu()) {
        // Track drag start position
        dragStartPos_ = e.getPosition();
        isDragging_ = false;
    }
}

void MacroKnobComponent::mouseDrag(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu())
        return;

    // Check if we've moved enough to start a drag
    if (!isDragging_) {
        auto distance = e.getPosition().getDistanceFrom(dragStartPos_);
        if (distance > DRAG_THRESHOLD) {
            isDragging_ = true;

            // Find a DragAndDropContainer ancestor
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
                // Create drag description: "macro_drag:trackId:topLevelDeviceId:macroIndex"
                juce::String desc = DRAG_PREFIX;
                desc += juce::String(parentPath_.trackId) + ":";
                desc += juce::String(parentPath_.topLevelDeviceId) + ":";
                desc += juce::String(macroIndex_);

                // Create a snapshot of this component for drag image
                auto snapshot = createComponentSnapshot(getLocalBounds());

                container->startDragging(desc, this, juce::ScaledImage(snapshot), true);
            }
        }
    }
}

void MacroKnobComponent::mouseUp(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) {
        // Right-click shows link menu
        showLinkMenu();
    } else if (!isDragging_) {
        // Left-click (no drag) - select this macro
        if (onClicked) {
            onClicked();
        }
    }
    isDragging_ = false;
}

void MacroKnobComponent::paintLinkIndicator(juce::Graphics& g, juce::Rectangle<int> area) {
    // Link indicator dot
    int dotSize = 4;
    auto dotBounds = area.withSizeKeepingCentre(dotSize, dotSize);

    if (currentMacro_.isLinked()) {
        // Purple filled dot when linked
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_PURPLE));
        g.fillEllipse(dotBounds.toFloat());
    } else {
        // Gray outline dot when not linked
        g.setColour(DarkTheme::getSecondaryTextColour().withAlpha(0.5f));
        g.drawEllipse(dotBounds.toFloat(), 1.0f);
    }
}

void MacroKnobComponent::showLinkMenu() {
    juce::PopupMenu menu;

    // Mock param names (same as DeviceSlotComponent)
    static const char* mockParamNames[16] = {
        "Cutoff",   "Resonance", "Drive",    "Mix",   "Attack", "Decay", "Sustain", "Release",
        "LFO Rate", "LFO Depth", "Feedback", "Width", "Low",    "Mid",   "High",    "Output"};

    menu.addSectionHeader("Link to Parameter...");
    menu.addSeparator();

    // Add submenu for each available device
    int itemId = 1;
    for (const auto& [deviceId, deviceName] : availableTargets_) {
        juce::PopupMenu deviceMenu;

        // Use proper param names
        for (int paramIdx = 0; paramIdx < 16; ++paramIdx) {
            juce::String paramName = mockParamNames[paramIdx];

            // Check if this is the currently linked target
            bool isCurrentTarget = currentMacro_.target.deviceId == deviceId &&
                                   currentMacro_.target.paramIndex == paramIdx;

            deviceMenu.addItem(itemId, paramName, true, isCurrentTarget);
            itemId++;
        }

        menu.addSubMenu(deviceName, deviceMenu);
    }

    menu.addSeparator();

    // Clear link option
    int clearLinkId = 10000;
    menu.addItem(clearLinkId, "Clear Link", currentMacro_.isLinked());

    // Show menu and handle selection
    auto safeThis = juce::Component::SafePointer<MacroKnobComponent>(this);
    auto targets = availableTargets_;  // Capture by value for async safety

    menu.showMenuAsync(juce::PopupMenu::Options(), [safeThis, targets, clearLinkId](int result) {
        if (safeThis == nullptr || result == 0) {
            return;
        }

        if (result == clearLinkId) {
            // Clear the link
            safeThis->currentMacro_.target = magda::MacroTarget{};
            safeThis->repaint();
            if (safeThis->onTargetChanged) {
                safeThis->onTargetChanged(safeThis->currentMacro_.target);
            }
            return;
        }

        // Calculate which device and param was selected
        int itemId = 1;
        for (const auto& [deviceId, deviceName] : targets) {
            for (int paramIdx = 0; paramIdx < 16; ++paramIdx) {
                if (itemId == result) {
                    // Set the new target
                    safeThis->currentMacro_.target.deviceId = deviceId;
                    safeThis->currentMacro_.target.paramIndex = paramIdx;
                    safeThis->repaint();
                    if (safeThis->onTargetChanged) {
                        safeThis->onTargetChanged(safeThis->currentMacro_.target);
                    }
                    return;
                }
                itemId++;
            }
        }
    });
}

void MacroKnobComponent::onNameLabelEdited() {
    auto newName = nameLabel_.getText().trim();
    if (newName.isEmpty()) {
        // Reset to default name if empty
        newName = "Macro " + juce::String(macroIndex_ + 1);
        nameLabel_.setText(newName, juce::dontSendNotification);
    }

    if (newName != currentMacro_.name) {
        currentMacro_.name = newName;
        if (onNameChanged) {
            onNameChanged(newName);
        }
    }
}

}  // namespace magda::daw::ui
