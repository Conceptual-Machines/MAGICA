#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magica {

class TrackComponent : public juce::Component {
public:
    TrackComponent();
    ~TrackComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Track properties
    void setTrackName(const juce::String& name);
    juce::String getTrackName() const;
    void setSelected(bool selected);
    bool isSelected() const;

    // Track controls
    void setMuted(bool muted);
    void setSolo(bool solo);
    void setVolume(float volume);
    void setPan(float pan);

    bool isMuted() const { return muted; }
    bool isSolo() const { return solo; }
    float getVolume() const { return volume; }
    float getPan() const { return pan; }

private:
    juce::String trackName = "Track";
    bool selected = false;
    bool muted = false;
    bool solo = false;
    float volume = 1.0f;
    float pan = 0.0f;

    // Layout constants
    static constexpr int CONTROL_AREA_WIDTH = 200;
    static constexpr int BUTTON_SIZE = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComponent)
};

} // namespace magica 