#pragma once

#include <juce_core/juce_core.h>

namespace magda {

using DeviceId = int;
constexpr DeviceId INVALID_DEVICE_ID = -1;

/**
 * @brief Plugin format enumeration
 */
enum class PluginFormat { VST3, AU, VST, Internal };

/**
 * @brief Device/plugin information stored on a track
 */
struct DeviceInfo {
    DeviceId id = INVALID_DEVICE_ID;
    juce::String name;          // Display name (e.g., "Pro-Q 3")
    juce::String pluginId;      // Unique plugin identifier for loading
    juce::String manufacturer;  // Plugin vendor
    PluginFormat format = PluginFormat::VST3;

    bool bypassed = false;  // Device bypass state
    bool expanded = true;   // UI expanded state

    // Gain stage (for the hidden gain stage feature)
    int gainParameterIndex = -1;  // -1 means no gain stage configured
    float gainValue = 1.0f;       // Current gain value (linear)

    // For future use: parameter state, modulation targets, etc.

    juce::String getFormatString() const {
        switch (format) {
            case PluginFormat::VST3:
                return "VST3";
            case PluginFormat::AU:
                return "AU";
            case PluginFormat::VST:
                return "VST";
            case PluginFormat::Internal:
                return "Internal";
            default:
                return "Unknown";
        }
    }
};

}  // namespace magda
