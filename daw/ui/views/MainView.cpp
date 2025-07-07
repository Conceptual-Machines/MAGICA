#include "MainView.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

MainView::MainView() {
    // Create viewport for scrolling
    viewport = std::make_unique<juce::Viewport>();
    // Note: Viewport doesn't have a background color ID, so we'll handle this in paint instead
    addAndMakeVisible(*viewport);
    
    // Create content component
    content = std::make_unique<ViewportContent>(*this);
    viewport->setViewedComponent(content.get(), false);
    
    // Create track area
    trackArea = std::make_unique<TrackArea>();
    content->addAndMakeVisible(*trackArea);
    
    // Add some initial tracks for demonstration
    addTrack();
    addTrack();
    addTrack();
    
    updateContentSize();
}

MainView::~MainView() = default;

void MainView::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::BACKGROUND));
}

void MainView::resized() {
    viewport->setBounds(getLocalBounds());
    updateContentSize();
}

void MainView::setHorizontalZoom(double zoomFactor) {
    horizontalZoom = juce::jmax(0.1, zoomFactor);
    updateContentSize();
}

void MainView::setVerticalZoom(double zoomFactor) {
    verticalZoom = juce::jmax(0.5, juce::jmin(3.0, zoomFactor));
    updateContentSize();
}

void MainView::scrollToPosition(double timePosition) {
    auto pixelPosition = static_cast<int>(timePosition * horizontalZoom);
    viewport->setViewPosition(pixelPosition, viewport->getViewPositionY());
}

void MainView::scrollToTrack(int trackIndex) {
    auto trackHeight = static_cast<int>(DEFAULT_TRACK_HEIGHT * verticalZoom);
    auto yPosition = TIMELINE_HEIGHT + trackIndex * trackHeight;
    viewport->setViewPosition(viewport->getViewPositionX(), yPosition);
}

void MainView::addTrack() {
    trackArea->addTrack();
    updateContentSize();
}

void MainView::removeTrack(int trackIndex) {
    trackArea->removeTrack(trackIndex);
    updateContentSize();
}

void MainView::selectTrack(int trackIndex) {
    trackArea->selectTrack(trackIndex);
}

void MainView::setTimelineLength(double lengthInSeconds) {
    timelineLength = lengthInSeconds;
    updateContentSize();
}

void MainView::setPlayheadPosition(double position) {
    playheadPosition = position;
    content->repaint();
}

void MainView::updateContentSize() {
    auto contentWidth = static_cast<int>(timelineLength * horizontalZoom);
    auto trackHeight = static_cast<int>(DEFAULT_TRACK_HEIGHT * verticalZoom);
    auto contentHeight = TIMELINE_HEIGHT + trackArea->getNumTracks() * trackHeight;
    
    content->setSize(juce::jmax(contentWidth, viewport->getWidth()),
                     juce::jmax(contentHeight, viewport->getHeight()));
    
    // Update track area size
    trackArea->setBounds(0, TIMELINE_HEIGHT, content->getWidth(), contentHeight - TIMELINE_HEIGHT);
}

void MainView::updateViewportPosition() {
    // Called when viewport position changes
    repaint();
}

// ViewportContent implementation
MainView::ViewportContent::ViewportContent(MainView& owner) : owner(owner) {
}

MainView::ViewportContent::~ViewportContent() = default;

void MainView::ViewportContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::TIMELINE_BACKGROUND));
    
    // Draw timeline background
    auto timelineArea = juce::Rectangle<int>(0, 0, getWidth(), owner.TIMELINE_HEIGHT);
    g.setColour(DarkTheme::getColour(DarkTheme::TRANSPORT_BACKGROUND));
    g.fillRect(timelineArea);
    
    // Draw timeline grid
    g.setColour(DarkTheme::getColour(DarkTheme::GRID_LINE));
    
    // Draw vertical grid lines (seconds)
    for (int i = 0; i < owner.timelineLength; ++i) {
        auto x = static_cast<int>(i * owner.horizontalZoom);
        g.drawVerticalLine(x, 0, getHeight());
    }
    
    // Draw beat lines (assuming 4/4 time at 120 BPM)
    g.setColour(DarkTheme::getColour(DarkTheme::BEAT_LINE));
    auto beatsPerSecond = 120.0 / 60.0; // Default tempo
    for (double beat = 0; beat < owner.timelineLength * beatsPerSecond; beat += 1.0) {
        auto x = static_cast<int>((beat / beatsPerSecond) * owner.horizontalZoom);
        g.drawVerticalLine(x, 0, getHeight());
    }
    
    // Draw bar lines (4 beats per bar)
    g.setColour(DarkTheme::getColour(DarkTheme::BAR_LINE));
    for (double bar = 0; bar < owner.timelineLength * beatsPerSecond / 4.0; bar += 1.0) {
        auto x = static_cast<int>((bar * 4.0 / beatsPerSecond) * owner.horizontalZoom);
        g.drawVerticalLine(x, 0, getHeight());
    }
    
    // Draw timeline markers
    g.setColour(DarkTheme::getTextColour());
    g.setFont(FontManager::getInstance().getUIFont(12.0f));
    
    for (int i = 0; i < owner.timelineLength; i += 5) {
        auto x = static_cast<int>(i * owner.horizontalZoom);
        auto timeText = juce::String(i) + "s";
        g.drawText(timeText, x + 2, 2, 50, 20, juce::Justification::left);
    }
    
    // Draw playhead
    g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    auto playheadX = static_cast<int>(owner.playheadPosition * owner.horizontalZoom);
    g.drawVerticalLine(playheadX, 0, getHeight());
}

void MainView::ViewportContent::resized() {
    // Content resizing is handled by MainView
}

void MainView::ViewportContent::mouseDown(const juce::MouseEvent& event) {
    // Handle timeline clicking
    if (event.y < owner.TIMELINE_HEIGHT) {
        auto clickTime = event.x / owner.horizontalZoom;
        owner.setPlayheadPosition(clickTime);
    }
}

void MainView::ViewportContent::mouseDrag(const juce::MouseEvent& event) {
    // Handle timeline dragging
    if (event.y < owner.TIMELINE_HEIGHT) {
        auto clickTime = event.x / owner.horizontalZoom;
        owner.setPlayheadPosition(clickTime);
    }
}

void MainView::ViewportContent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    // Handle zoom with mouse wheel
    if (event.mods.isCommandDown()) {
        // Horizontal zoom
        auto zoomFactor = wheel.deltaY > 0 ? 1.1 : 0.9;
        owner.setHorizontalZoom(owner.horizontalZoom * zoomFactor);
    } else if (event.mods.isShiftDown()) {
        // Vertical zoom
        auto zoomFactor = wheel.deltaY > 0 ? 1.1 : 0.9;
        owner.setVerticalZoom(owner.verticalZoom * zoomFactor);
    }
}

// TrackArea implementation
MainView::TrackArea::TrackArea() {
}

MainView::TrackArea::~TrackArea() = default;

void MainView::TrackArea::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    
    // Draw track separators
    g.setColour(DarkTheme::getColour(DarkTheme::TRACK_SEPARATOR));
    
    auto trackHeight = getBounds().getHeight() / juce::jmax(1, static_cast<int>(tracks.size()));
    for (int i = 1; i < tracks.size(); ++i) {
        auto y = i * trackHeight;
        g.drawHorizontalLine(y, 0, getWidth());
    }
}

void MainView::TrackArea::resized() {
    // Layout tracks
    if (tracks.empty()) return;
    
    auto trackHeight = getBounds().getHeight() / static_cast<int>(tracks.size());
    
    for (int i = 0; i < tracks.size(); ++i) {
        auto trackBounds = juce::Rectangle<int>(0, i * trackHeight, getWidth(), trackHeight);
        // tracks[i]->setBounds(trackBounds); // Will implement TrackComponent later
    }
}

void MainView::TrackArea::addTrack() {
    // For now, just track the count - we'll implement TrackComponent later
    tracks.push_back(nullptr);
    repaint();
}

void MainView::TrackArea::removeTrack(int index) {
    if (index >= 0 && index < tracks.size()) {
        tracks.erase(tracks.begin() + index);
        repaint();
    }
}

void MainView::TrackArea::selectTrack(int index) {
    if (index >= 0 && index < tracks.size()) {
        selectedTrackIndex = index;
        repaint();
    }
}

int MainView::TrackArea::getNumTracks() const {
    return static_cast<int>(tracks.size());
}

} // namespace magica 