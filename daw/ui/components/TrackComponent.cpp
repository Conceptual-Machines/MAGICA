#include "TrackComponent.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"
#include "../views/MainView.hpp"

namespace magica {

TrackComponent::TrackComponent() {
    setupTrackHeader();
    setSize(800, trackHeight);
}

void TrackComponent::paint(juce::Graphics& g) {
    // Debug output
    juce::Logger::writeToLog("TrackComponent::paint() called - bounds: " + getBounds().toString() + 
                            ", name: " + trackName);
    
    // Paint track header (controls area)
    auto headerArea = getTrackHeaderArea();
    paintTrackHeader(g, headerArea);
    
    // Paint track lane (content area)
    auto laneArea = getTrackLaneArea();
    paintTrackLane(g, laneArea);
    
    // Paint resize handle
    paintResizeHandle(g);
    
    // Draw selection highlight if selected
    if (selected) {
        auto bounds = getLocalBounds();
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE).withAlpha(0.3f));
        g.fillRect(bounds);
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawRect(bounds, 2);
    }
}

void TrackComponent::resized() {
    auto bounds = getLocalBounds();
    auto headerArea = getTrackHeaderArea();
    
    // Layout track header controls
    auto controlArea = headerArea.reduced(8);
    
    // Track name label
    auto nameArea = controlArea.removeFromTop(20);
    if (nameLabel) {
        nameLabel->setBounds(nameArea);
    }
    
    controlArea.removeFromTop(4); // Spacing
    
    // Mute and Solo buttons (side by side)
    auto buttonRow = controlArea.removeFromTop(24);
    if (muteButton) {
        muteButton->setBounds(buttonRow.removeFromLeft(40));
    }
    buttonRow.removeFromLeft(4); // Spacing
    if (soloButton) {
        soloButton->setBounds(buttonRow.removeFromLeft(40));
    }
    
    controlArea.removeFromTop(4); // Spacing
    
    // Volume and Pan sliders
    if (volumeSlider) {
        auto volArea = controlArea.removeFromTop(20);
        volumeSlider->setBounds(volArea);
    }
    
    controlArea.removeFromTop(2); // Spacing
    
    if (panSlider) {
        auto panArea = controlArea.removeFromTop(20);
        panSlider->setBounds(panArea);
    }
}

void TrackComponent::setupTrackHeader() {
    // Track name label
    nameLabel = std::make_unique<juce::Label>();
    nameLabel->setText(trackName, juce::dontSendNotification);
    nameLabel->setFont(FontManager::getInstance().getUIFont(12.0f));
    nameLabel->setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    nameLabel->setEditable(true);
    nameLabel->onTextChange = [this]() {
        trackName = nameLabel->getText();
    };
    addAndMakeVisible(*nameLabel);
    
    // Mute button
    muteButton = std::make_unique<juce::ToggleButton>("M");
    muteButton->setButtonText("M");
    muteButton->setToggleState(muted, juce::dontSendNotification);
    muteButton->onClick = [this]() {
        setMuted(muteButton->getToggleState());
    };
    addAndMakeVisible(*muteButton);
    
    // Solo button
    soloButton = std::make_unique<juce::ToggleButton>("S");
    soloButton->setButtonText("S");
    soloButton->setToggleState(solo, juce::dontSendNotification);
    soloButton->onClick = [this]() {
        setSolo(soloButton->getToggleState());
    };
    addAndMakeVisible(*soloButton);
    
    // Volume slider
    volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.0);
    volumeSlider->setValue(volume);
    volumeSlider->onValueChange = [this]() {
        setVolume(static_cast<float>(volumeSlider->getValue()));
    };
    addAndMakeVisible(*volumeSlider);
    
    // Pan slider
    panSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    panSlider->setRange(-1.0, 1.0);
    panSlider->setValue(pan);
    panSlider->onValueChange = [this]() {
        setPan(static_cast<float>(panSlider->getValue()));
    };
    addAndMakeVisible(*panSlider);
}

void TrackComponent::paintTrackHeader(juce::Graphics& g, juce::Rectangle<int> area) {
    // Fill header background
    g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
    g.fillRect(area);
    
    // Draw header border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(area, 1);
    
    // Draw volume/pan labels (small text)
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(9.0f));
    
    auto labelArea = area.reduced(8);
    labelArea.removeFromTop(48); // Skip name and buttons
    
    if (volumeSlider) {
        auto volLabelArea = labelArea.removeFromTop(20);
        g.drawText("Vol", volLabelArea.removeFromLeft(25), juce::Justification::centredLeft);
    }
    
    if (panSlider) {
        auto panLabelArea = labelArea.removeFromTop(20);
        g.drawText("Pan", panLabelArea.removeFromLeft(25), juce::Justification::centredLeft);
    }
}

void TrackComponent::paintTrackLane(juce::Graphics& g, juce::Rectangle<int> area) {
    // Fill lane background
    g.setColour(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    g.fillRect(area);
    
    // Draw lane border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(area, 1);
    
    // Draw grid overlay synchronized with timeline
    // Use the stored zoom value (set by parent)
    if (currentZoom <= 0) return;
    
    // Calculate appropriate marker spacing (same logic as TimelineComponent)
    const int minPixelSpacing = 30;
    int markerInterval = 1; // Start with 1 second intervals
    
    // Adjust interval if markers would be too close
    while (static_cast<int>(markerInterval * currentZoom) < minPixelSpacing && markerInterval < 60) {
        markerInterval *= (markerInterval < 10) ? 2 : 5; // 1,2,5,10,20,50...
    }
    
    g.setColour(DarkTheme::getColour(DarkTheme::GRID_LINE));
    
    // Draw vertical grid lines (time intervals) - synchronized with timeline
    for (int i = 0; i <= 300; i += markerInterval) { // 300 seconds max
        int x = static_cast<int>(i * currentZoom);
        if (x >= area.getWidth()) break;
        if (x > 0) {
            g.drawVerticalLine(area.getX() + x, area.getY(), area.getBottom());
        }
    }
    
    // Draw stronger bar lines (every 4 beats at 120 BPM = every 2 seconds)
    g.setColour(DarkTheme::getColour(DarkTheme::BAR_LINE));
    const int barInterval = 8; // 8 seconds per bar (4 beats * 2 seconds per beat)
    for (int i = 0; i <= 300; i += barInterval) {
        int x = static_cast<int>(i * currentZoom);
        if (x >= area.getWidth()) break;
        if (x > 0) {
            g.drawVerticalLine(area.getX() + x, area.getY(), area.getBottom());
        }
    }
    
    // TODO: This is where audio clips/MIDI clips would be drawn
    // For now, just draw a placeholder
    if (!area.isEmpty()) {
        g.setColour(DarkTheme::getColour(DarkTheme::TEXT_SECONDARY).withAlpha(0.1f));
        g.drawText("Track Lane", area, juce::Justification::centred);
    }
}

void TrackComponent::setZoom(double zoom) {
    currentZoom = zoom;
    repaint();
}

juce::Rectangle<int> TrackComponent::getTrackHeaderArea() const {
    return getLocalBounds().removeFromLeft(TRACK_HEADER_WIDTH);
}

juce::Rectangle<int> TrackComponent::getTrackLaneArea() const {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(TRACK_HEADER_WIDTH);
    return bounds;
}

void TrackComponent::setTrackName(const juce::String& name) {
    trackName = name;
    if (nameLabel) {
        nameLabel->setText(name, juce::dontSendNotification);
    }
    repaint();
}

void TrackComponent::setSelected(bool shouldBeSelected) {
    if (selected != shouldBeSelected) {
        selected = shouldBeSelected;
        repaint();
    }
}

void TrackComponent::setMuted(bool shouldBeMuted) {
    if (muted != shouldBeMuted) {
        muted = shouldBeMuted;
        if (muteButton) {
            muteButton->setToggleState(muted, juce::dontSendNotification);
        }
        repaint();
    }
}

void TrackComponent::setSolo(bool shouldBeSolo) {
    if (solo != shouldBeSolo) {
        solo = shouldBeSolo;
        if (soloButton) {
            soloButton->setToggleState(solo, juce::dontSendNotification);
        }
        repaint();
    }
}

void TrackComponent::setVolume(float newVolume) {
    volume = juce::jlimit(0.0f, 1.0f, newVolume);
    if (volumeSlider) {
        volumeSlider->setValue(volume, juce::dontSendNotification);
    }
}

void TrackComponent::setPan(float newPan) {
    pan = juce::jlimit(-1.0f, 1.0f, newPan);
    if (panSlider) {
        panSlider->setValue(pan, juce::dontSendNotification);
    }
}

void TrackComponent::setTrackHeight(int height) {
    trackHeight = juce::jlimit(MIN_TRACK_HEIGHT, MAX_TRACK_HEIGHT, height);
    setSize(getWidth(), trackHeight);
    resized();
}

bool TrackComponent::isResizeHandleArea(const juce::Point<int>& point) const {
    return getResizeHandleArea().contains(point);
}

void TrackComponent::mouseDown(const juce::MouseEvent& event) {
    if (isResizeHandleArea(event.getPosition())) {
        isResizing = true;
        resizeStartY = event.y;
        resizeStartHeight = trackHeight;
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
}

void TrackComponent::mouseDrag(const juce::MouseEvent& event) {
    if (isResizing) {
        int deltaY = event.y - resizeStartY;
        int newHeight = resizeStartHeight + deltaY;
        newHeight = juce::jlimit(MIN_TRACK_HEIGHT, MAX_TRACK_HEIGHT, newHeight);
        
        if (newHeight != trackHeight) {
            setTrackHeight(newHeight);
            if (onTrackHeightChanged) {
                onTrackHeightChanged(this, newHeight);
            }
        }
    }
}

void TrackComponent::mouseMove(const juce::MouseEvent& event) {
    if (isResizeHandleArea(event.getPosition())) {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    } else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void TrackComponent::paintResizeHandle(juce::Graphics& g) {
    auto handleArea = getResizeHandleArea();
    
    // Draw resize handle background
    g.setColour(DarkTheme::getColour(DarkTheme::SURFACE).brighter(0.1f));
    g.fillRect(handleArea);
    
    // Draw resize handle grip lines
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER).brighter(0.2f));
    auto center = handleArea.getCentre();
    int lineWidth = handleArea.getWidth() / 3;
    
    for (int i = -1; i <= 1; ++i) {
        int y = center.y + i * 2;
        g.drawHorizontalLine(y, center.x - lineWidth / 2, center.x + lineWidth / 2);
    }
}

juce::Rectangle<int> TrackComponent::getResizeHandleArea() const {
    auto bounds = getLocalBounds();
    return bounds.removeFromBottom(RESIZE_HANDLE_HEIGHT);
}

} // namespace magica 