#include "MainView.hpp"
#include "../themes/DarkTheme.hpp"

namespace magica {

MainView::MainView() {
    // Create arrangement timeline viewport
    arrangementViewport = std::make_unique<juce::Viewport>();
    arrangementTimeline = std::make_unique<ArrangementTimelineComponent>();
    arrangementViewport->setViewedComponent(arrangementTimeline.get(), false);
    arrangementViewport->setScrollBarsShown(false, false);
    addAndMakeVisible(*arrangementViewport);
    
    // Set up arrangement timeline callbacks
    arrangementTimeline->onPlayheadPositionChanged = [this](double position) {
        setPlayheadPosition(position);
    };
    
    // Create timeline viewport
    timelineViewport = std::make_unique<juce::Viewport>();
    timeline = std::make_unique<TimelineComponent>();
    timelineViewport->setViewedComponent(timeline.get(), false);
    timelineViewport->setScrollBarsShown(false, false);
    addAndMakeVisible(*timelineViewport);
    
    // Set up timeline callbacks
    timeline->onPlayheadPositionChanged = [this](double position) {
        setPlayheadPosition(position);
    };
    
    // Create track viewport
    trackViewport = std::make_unique<juce::Viewport>();
    trackContent = std::make_unique<TrackViewportContent>(*this);
    trackViewport->setViewedComponent(trackContent.get(), false);
    trackViewport->setScrollBarsShown(true, true);
    addAndMakeVisible(*trackViewport);
    
    // Create track area
    trackArea = std::make_unique<TrackArea>();
    trackContent->addAndMakeVisible(*trackArea);
    
    // Create playhead component (always on top)
    playheadComponent = std::make_unique<PlayheadComponent>(*this);
    addAndMakeVisible(*playheadComponent);
    playheadComponent->toFront(false);
    
    // Set up scroll synchronization
    trackViewport->getHorizontalScrollBar().addListener(this);
    
    // Add some initial tracks for testing
    addTrack();
    addTrack();
    addTrack();
    
    // Set initial timeline length and zoom
    setTimelineLength(120.0);
}

MainView::~MainView() = default;

void MainView::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::BACKGROUND));
}

void MainView::resized() {
    auto bounds = getLocalBounds();
    
    // Arrangement timeline viewport at the top - offset by track header width
    auto arrangementArea = bounds.removeFromTop(ARRANGEMENT_HEIGHT);
    arrangementArea.removeFromLeft(TrackComponent::TRACK_HEADER_WIDTH); // Align with track lanes
    arrangementViewport->setBounds(arrangementArea);
    
    // Timeline viewport below arrangement - offset by track header width
    auto timelineArea = bounds.removeFromTop(TIMELINE_HEIGHT);
    timelineArea.removeFromLeft(TrackComponent::TRACK_HEADER_WIDTH); // Align with track lanes
    timelineViewport->setBounds(timelineArea);
    
    // Track viewport gets the remaining space
    trackViewport->setBounds(bounds);
    
    // Playhead component covers the entire MainView area
    playheadComponent->setBounds(getLocalBounds());
    
    // Set initial horizontal zoom to show about 60 seconds in the viewport width
    if (horizontalZoom == 1.0) { // Only set initial zoom once
        auto viewportWidth = timelineViewport->getWidth();
        if (viewportWidth > 0) {
            horizontalZoom = viewportWidth / 60.0; // Show 60 seconds initially
        }
    }
    
    updateContentSizes();
}

void MainView::setHorizontalZoom(double zoomFactor) {
    horizontalZoom = juce::jmax(0.1, zoomFactor);
    
    // Update zoom on timeline components
    timeline->setZoom(horizontalZoom);
    arrangementTimeline->setZoom(horizontalZoom);
    
    // Update zoom on all track components for grid synchronization
    for (size_t i = 0; i < trackArea->getNumTracks(); ++i) {
        if (auto* track = trackArea->getTrack(i)) {
            track->setZoom(horizontalZoom);
        }
    }
    
    updateContentSizes();
    repaint(); // Repaint for unified playhead
}

void MainView::setVerticalZoom(double zoomFactor) {
    verticalZoom = juce::jmax(0.5, juce::jmin(3.0, zoomFactor));
    updateContentSizes();
}

void MainView::scrollToPosition(double timePosition) {
    auto pixelPosition = static_cast<int>(timePosition * horizontalZoom);
    arrangementViewport->setViewPosition(pixelPosition, 0);
    timelineViewport->setViewPosition(pixelPosition, 0);
    trackViewport->setViewPosition(pixelPosition, trackViewport->getViewPositionY());
}

void MainView::scrollToTrack(int trackIndex) {
    auto trackHeight = static_cast<int>(DEFAULT_TRACK_HEIGHT * verticalZoom);
    auto yPosition = trackIndex * trackHeight;
    trackViewport->setViewPosition(trackViewport->getViewPositionX(), yPosition);
}

void MainView::addTrack() {
    trackArea->addTrack();
    updateContentSizes();
}

void MainView::removeTrack(int trackIndex) {
    trackArea->removeTrack(trackIndex);
    updateContentSizes();
}

void MainView::selectTrack(int trackIndex) {
    trackArea->selectTrack(trackIndex);
}

void MainView::setTimelineLength(double lengthInSeconds) {
    timelineLength = lengthInSeconds;
    timeline->setTimelineLength(lengthInSeconds);
    arrangementTimeline->setTimelineLength(lengthInSeconds);
    updateContentSizes();
}

void MainView::setPlayheadPosition(double position) {
    playheadPosition = position;
    // Update the dedicated playhead component
    playheadComponent->setPlayheadPosition(position);
    playheadComponent->repaint();
}

void MainView::updateContentSizes() {
    auto contentWidth = static_cast<int>(timelineLength * horizontalZoom);
    auto trackContentHeight = trackArea->getHeight(); // Use actual track area height
    
    // Debug output
    juce::Logger::writeToLog("updateContentSizes: contentWidth=" + juce::String(contentWidth) + 
                            ", trackContentHeight=" + juce::String(trackContentHeight) +
                            ", numTracks=" + juce::String(trackArea->getNumTracks()));
    
    // Update arrangement timeline size
    arrangementTimeline->setSize(juce::jmax(contentWidth, arrangementViewport->getWidth()), ARRANGEMENT_HEIGHT);
    
    // Update timeline size
    timeline->setSize(juce::jmax(contentWidth, timelineViewport->getWidth()), TIMELINE_HEIGHT);
    
    // Update track content size
    trackContent->setSize(juce::jmax(contentWidth, trackViewport->getWidth()),
                         juce::jmax(trackContentHeight, trackViewport->getHeight()));
    
    // Update track area size
    trackArea->setBounds(0, 0, trackContent->getWidth(), trackContentHeight);
    
    // Repaint playhead after content size changes
    playheadComponent->repaint();
    
    // Debug output for final sizes
    juce::Logger::writeToLog("Final sizes - trackContent: " + juce::String(trackContent->getWidth()) + "x" + juce::String(trackContent->getHeight()) +
                            ", trackArea: " + trackArea->getBounds().toString());
}

void MainView::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) {
    // Sync timeline and arrangement viewports when track viewport scrolls horizontally
    if (scrollBarThatHasMoved == &trackViewport->getHorizontalScrollBar()) {
        arrangementViewport->setViewPosition(static_cast<int>(newRangeStart), 0);
        timelineViewport->setViewPosition(static_cast<int>(newRangeStart), 0);
    }
}

// TrackViewportContent implementation
MainView::TrackViewportContent::TrackViewportContent(MainView& owner) : owner(owner) {
}

MainView::TrackViewportContent::~TrackViewportContent() = default;

void MainView::TrackViewportContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    
    // Grid is now drawn by individual TrackComponent instances
    // This ensures proper layering and track-specific rendering
    
    // Note: Playhead is now drawn by MainView for unified positioning
}

void MainView::TrackViewportContent::resized() {
    // Content resizing is handled by MainView
}

void MainView::TrackViewportContent::mouseDown(const juce::MouseEvent& event) {
    // Handle track interaction and playhead positioning
    auto clickTime = event.x / owner.horizontalZoom;
    owner.setPlayheadPosition(clickTime);
}

void MainView::TrackViewportContent::mouseDrag(const juce::MouseEvent& event) {
    // Handle track interaction and playhead positioning
    auto clickTime = event.x / owner.horizontalZoom;
    owner.setPlayheadPosition(clickTime);
}

void MainView::TrackViewportContent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
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
    // Grid overlay is now drawn by individual TrackComponents
    // Just fill any remaining background
    g.fillAll(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
}

void MainView::TrackArea::resized() {
    // Layout tracks vertically using individual track heights
    if (tracks.empty()) return;
    
    int yPosition = 0;
    
    // Debug output
    juce::Logger::writeToLog("TrackArea resized: " + juce::String(getWidth()) + "x" + juce::String(getHeight()) + 
                            ", numTracks: " + juce::String(tracks.size()));
    
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i]) {
            int trackHeight = tracks[i]->getTrackHeight();
            auto trackBounds = juce::Rectangle<int>(0, yPosition, getWidth(), trackHeight);
            tracks[i]->setBounds(trackBounds);
            yPosition += trackHeight;
            
            // Debug output
            juce::Logger::writeToLog("Track " + juce::String(i) + " bounds: " + trackBounds.toString() + 
                                    ", height: " + juce::String(trackHeight));
        }
    }
}

void MainView::TrackArea::addTrack() {
    auto newTrack = std::make_unique<TrackComponent>();
    newTrack->setTrackName("Track " + juce::String(tracks.size() + 1));
    
    // Set initial zoom from parent MainView
    if (auto* parent = getParentMainView()) {
        newTrack->setZoom(parent->getHorizontalZoom());
    }
    
    // Set up track height change callback
    newTrack->onTrackHeightChanged = [this](TrackComponent* track, int newHeight) {
        // Recalculate total height and notify parent
        updateTotalHeight();
        resized();
    };
    
    // Debug output
    juce::Logger::writeToLog("Creating track " + juce::String(tracks.size() + 1));
    
    addAndMakeVisible(*newTrack);
    tracks.push_back(std::move(newTrack));
    updateTotalHeight();
    resized();
    repaint();
}

void MainView::TrackArea::removeTrack(int index) {
    if (index >= 0 && index < static_cast<int>(tracks.size())) {
        tracks.erase(tracks.begin() + index);
        if (selectedTrackIndex == index) {
            selectedTrackIndex = -1;
        } else if (selectedTrackIndex > index) {
            selectedTrackIndex--;
        }
        resized();
        repaint();
    }
}

void MainView::TrackArea::selectTrack(int index) {
    if (index >= 0 && index < static_cast<int>(tracks.size())) {
        // Deselect previous track
        if (selectedTrackIndex >= 0 && selectedTrackIndex < static_cast<int>(tracks.size()) && tracks[selectedTrackIndex]) {
            tracks[selectedTrackIndex]->setSelected(false);
        }
        
        // Select new track
        selectedTrackIndex = index;
        if (tracks[selectedTrackIndex]) {
            tracks[selectedTrackIndex]->setSelected(true);
        }
    }
}

int MainView::TrackArea::getNumTracks() const {
    return static_cast<int>(tracks.size());
}

TrackComponent* MainView::TrackArea::getTrack(int index) const {
    if (index >= 0 && index < static_cast<int>(tracks.size())) {
        return tracks[index].get();
    }
    return nullptr;
}

void MainView::TrackArea::updateTotalHeight() {
    int totalHeight = 0;
    for (const auto& track : tracks) {
        if (track) {
            totalHeight += track->getTrackHeight();
        }
    }
    
    // Update our own height to match total track heights
    setSize(getWidth(), totalHeight);
    
    // Notify parent MainView to update content sizes
    if (auto* parent = dynamic_cast<MainView*>(getParentComponent()->getParentComponent())) {
        parent->updateContentSizes();
    }
}

MainView* MainView::TrackArea::getParentMainView() const {
    // Navigate up the component hierarchy to find MainView
    if (auto* trackContent = getParentComponent()) {
        if (auto* trackViewport = trackContent->getParentComponent()) {
            if (auto* mainView = dynamic_cast<MainView*>(trackViewport->getParentComponent())) {
                return mainView;
            }
        }
    }
    return nullptr;
}

// PlayheadComponent implementation
MainView::PlayheadComponent::PlayheadComponent(MainView& owner) : owner(owner) {
    setInterceptsMouseClicks(false, false); // Allow mouse events to pass through
}

MainView::PlayheadComponent::~PlayheadComponent() = default;

void MainView::PlayheadComponent::paint(juce::Graphics& g) {
    // Calculate playhead position in pixels
    auto playheadX = static_cast<int>(playheadPosition * owner.horizontalZoom);
    
    // Only draw if playhead is within visible area
    if (playheadX < 0 || playheadX >= static_cast<int>(owner.timelineLength * owner.horizontalZoom)) {
        return;
    }
    
    // Get viewport positions to account for scrolling
    int arrangementScrollX = owner.arrangementViewport->getViewPositionX();
    int timelineScrollX = owner.timelineViewport->getViewPositionX();
    int trackScrollX = owner.trackViewport->getViewPositionX();
    
    // Draw playhead in arrangement timeline area
    auto arrangementBounds = owner.arrangementViewport->getBounds();
    int arrangementPlayheadX = arrangementBounds.getX() + (playheadX - arrangementScrollX);
    if (arrangementPlayheadX >= arrangementBounds.getX() && arrangementPlayheadX < arrangementBounds.getRight()) {
        // Draw shadow for better visibility
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(arrangementPlayheadX + 1, arrangementBounds.getY(), 
                   arrangementPlayheadX + 1, arrangementBounds.getBottom(), 5.0f);
        // Draw main playhead line
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(arrangementPlayheadX, arrangementBounds.getY(), 
                   arrangementPlayheadX, arrangementBounds.getBottom(), 4.0f);
    }
    
    // Draw playhead in main timeline area
    auto timelineBounds = owner.timelineViewport->getBounds();
    int timelinePlayheadX = timelineBounds.getX() + (playheadX - timelineScrollX);
    if (timelinePlayheadX >= timelineBounds.getX() && timelinePlayheadX < timelineBounds.getRight()) {
        // Draw shadow for better visibility
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(timelinePlayheadX + 1, timelineBounds.getY(), 
                   timelinePlayheadX + 1, timelineBounds.getBottom(), 5.0f);
        // Draw main playhead line
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(timelinePlayheadX, timelineBounds.getY(), 
                   timelinePlayheadX, timelineBounds.getBottom(), 4.0f);
    }
    
    // Draw playhead in track area
    auto trackBounds = owner.trackViewport->getBounds();
    int trackPlayheadX = trackBounds.getX() + (playheadX - trackScrollX);
    if (trackPlayheadX >= trackBounds.getX() && trackPlayheadX < trackBounds.getRight()) {
        // Draw shadow for better visibility
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(trackPlayheadX + 1, trackBounds.getY(), 
                   trackPlayheadX + 1, trackBounds.getBottom(), 5.0f);
        // Draw main playhead line
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(trackPlayheadX, trackBounds.getY(), 
                   trackPlayheadX, trackBounds.getBottom(), 4.0f);
    }
}

void MainView::PlayheadComponent::setPlayheadPosition(double position) {
    playheadPosition = position;
}

} // namespace magica 