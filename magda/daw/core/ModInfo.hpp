#pragma once

#include <juce_core/juce_core.h>

#include <vector>

#include "DeviceInfo.hpp"

namespace magda {

using ModId = int;
constexpr ModId INVALID_MOD_ID = -1;
constexpr int MODS_PER_PAGE = 8;
constexpr int DEFAULT_MOD_PAGES = 2;
constexpr int NUM_MODS = MODS_PER_PAGE * DEFAULT_MOD_PAGES;

/**
 * @brief Type of modulator
 */
enum class ModType { LFO, Envelope, Random, Follower };

/**
 * @brief Target for a mod link (which device parameter it modulates)
 */
struct ModTarget {
    DeviceId deviceId = INVALID_DEVICE_ID;
    int paramIndex = -1;  // Which parameter on the device

    bool isValid() const {
        return deviceId != INVALID_DEVICE_ID && paramIndex >= 0;
    }

    bool operator==(const ModTarget& other) const {
        return deviceId == other.deviceId && paramIndex == other.paramIndex;
    }

    bool operator!=(const ModTarget& other) const {
        return !(*this == other);
    }
};

/**
 * @brief A modulator that can be linked to device parameters
 *
 * Mods provide dynamic modulation of parameters.
 * Each rack and chain has 16 mods by default.
 */
struct ModInfo {
    ModId id = INVALID_MOD_ID;
    juce::String name;  // e.g., "LFO 1" or user-defined
    ModType type = ModType::LFO;
    float amount = 0.5f;  // 0.0 to 1.0, modulation depth
    float rate = 1.0f;    // Rate/speed of modulation
    ModTarget target;     // Optional linked parameter

    // Default constructor
    ModInfo() = default;

    // Constructor with index (for initialization)
    explicit ModInfo(int index)
        : id(index), name(getDefaultName(index, ModType::LFO)), type(ModType::LFO) {}

    bool isLinked() const {
        return target.isValid();
    }

    static juce::String getDefaultName(int index, ModType t) {
        juce::String prefix;
        switch (t) {
            case ModType::LFO:
                prefix = "LFO ";
                break;
            case ModType::Envelope:
                prefix = "Env ";
                break;
            case ModType::Random:
                prefix = "Rnd ";
                break;
            case ModType::Follower:
                prefix = "Fol ";
                break;
        }
        return prefix + juce::String(index + 1);
    }
};

/**
 * @brief Vector of mods (used by RackInfo and ChainInfo)
 */
using ModArray = std::vector<ModInfo>;

/**
 * @brief Initialize a ModArray with default values
 */
inline ModArray createDefaultMods(int numMods = NUM_MODS) {
    ModArray mods;
    mods.reserve(numMods);
    for (int i = 0; i < numMods; ++i) {
        mods.push_back(ModInfo(i));
    }
    return mods;
}

/**
 * @brief Add a page of mods (8 mods) to an existing array
 */
inline void addModPage(ModArray& mods) {
    int startIndex = static_cast<int>(mods.size());
    for (int i = 0; i < MODS_PER_PAGE; ++i) {
        mods.push_back(ModInfo(startIndex + i));
    }
}

/**
 * @brief Remove a page of mods (8 mods) from an existing array
 * @return true if page was removed, false if at minimum size
 */
inline bool removeModPage(ModArray& mods, int minMods = NUM_MODS) {
    if (static_cast<int>(mods.size()) <= minMods) {
        return false;  // At minimum size
    }

    int toRemove = juce::jmin(MODS_PER_PAGE, static_cast<int>(mods.size()) - minMods);
    for (int i = 0; i < toRemove; ++i) {
        mods.pop_back();
    }
    return true;
}

}  // namespace magda
