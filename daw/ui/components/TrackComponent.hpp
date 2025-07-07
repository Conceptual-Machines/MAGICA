#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace magica {

class TrackComponent : public juce::Component {
public:
    static constexpr int TRACK_HEADER_WIDTH = 200;
    static constexpr int MIN_TRACK_HEIGHT = 40;
    static constexpr int DEFAULT_TRACK_HEIGHT = 80;
    static constexpr int MAX_TRACK_HEIGHT = 200;

    TrackComponent();
    ~TrackComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // Track properties
    void setTrackName(const juce::String& name);
    void setSelected(bool shouldBeSelected);
    void setMuted(bool shouldBeMuted);
    void setSolo(bool shouldBeSolo);
    void setVolume(float newVolume);
    void setPan(float newPan);
    
    // Track height management
    void setTrackHeight(int height);
    int getTrackHeight() const { return trackHeight; }
    
    // Zoom management
    void setZoom(double zoom);
    
    // Resize handle
    bool isResizeHandleArea(const juce::Point<int>& point) const;
    
    // Callbacks
    std::function<void(TrackComponent*, int)> onTrackHeightChanged;

    // Getters
    juce::String getTrackName() const { return trackName; }
    bool isSelected() const { return selected; }
    bool isMuted() const { return muted; }
    bool isSolo() const { return solo; }
    float getVolume() const { return volume; }
    float getPan() const { return pan; }

private:
    void setupTrackHeader();
    void paintTrackHeader(juce::Graphics& g, juce::Rectangle<int> area);
    void paintTrackLane(juce::Graphics& g, juce::Rectangle<int> area);
    void paintResizeHandle(juce::Graphics& g);
    
    juce::Rectangle<int> getTrackHeaderArea() const;
    juce::Rectangle<int> getTrackLaneArea() const;
    juce::Rectangle<int> getResizeHandleArea() const;

    // Track properties
    juce::String trackName = "Track";
    bool selected = false;
    bool muted = false;
    bool solo = false;
    float volume = 0.8f;
    float pan = 0.0f;
    int trackHeight = DEFAULT_TRACK_HEIGHT;
    double currentZoom = 1.0;

    // UI components
    std::unique_ptr<juce::Label> nameLabel;
    std::unique_ptr<juce::ToggleButton> muteButton;
    std::unique_ptr<juce::ToggleButton> soloButton;
    std::unique_ptr<juce::Slider> volumeSlider;
    std::unique_ptr<juce::Slider> panSlider;
    
    // Resize functionality
    bool isResizing = false;
    int resizeStartY = 0;
    int resizeStartHeight = 0;
    static constexpr int RESIZE_HANDLE_HEIGHT = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComponent)
};

} // namespace magica 