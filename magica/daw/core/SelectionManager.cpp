#include "SelectionManager.hpp"

#include <algorithm>

#include "ClipManager.hpp"
#include "TrackManager.hpp"

namespace magica {

SelectionManager& SelectionManager::getInstance() {
    static SelectionManager instance;
    return instance;
}

SelectionManager::SelectionManager() {
    // Start with no selection
}

// ============================================================================
// Track Selection
// ============================================================================

void SelectionManager::selectTrack(TrackId trackId) {
    bool typeChanged = selectionType_ != SelectionType::Track;
    bool trackChanged = selectedTrackId_ != trackId;

    // Clear other selection types
    selectedClipId_ = INVALID_CLIP_ID;
    timeRangeSelection_ = TimeRangeSelection{};

    selectionType_ = SelectionType::Track;
    selectedTrackId_ = trackId;

    // Sync with TrackManager
    TrackManager::getInstance().setSelectedTrack(trackId);

    // Sync with ClipManager (clear clip selection)
    ClipManager::getInstance().clearClipSelection();

    if (typeChanged) {
        notifySelectionTypeChanged(SelectionType::Track);
    }
    if (trackChanged) {
        notifyTrackSelectionChanged(trackId);
    }
}

// ============================================================================
// Clip Selection
// ============================================================================

void SelectionManager::selectClip(ClipId clipId) {
    bool typeChanged = selectionType_ != SelectionType::Clip;
    bool clipChanged = selectedClipId_ != clipId;

    // Clear other selection types
    selectedTrackId_ = INVALID_TRACK_ID;
    timeRangeSelection_ = TimeRangeSelection{};

    selectionType_ = SelectionType::Clip;
    selectedClipId_ = clipId;

    // Sync with ClipManager
    ClipManager::getInstance().setSelectedClip(clipId);

    // Sync with TrackManager (clear track selection)
    TrackManager::getInstance().setSelectedTrack(INVALID_TRACK_ID);

    if (typeChanged) {
        notifySelectionTypeChanged(SelectionType::Clip);
    }
    if (clipChanged) {
        notifyClipSelectionChanged(clipId);
    }
}

// ============================================================================
// Time Range Selection
// ============================================================================

void SelectionManager::selectTimeRange(double startTime, double endTime,
                                       const std::vector<TrackId>& trackIds) {
    bool typeChanged = selectionType_ != SelectionType::TimeRange;

    // Clear other selection types
    selectedTrackId_ = INVALID_TRACK_ID;
    selectedClipId_ = INVALID_CLIP_ID;

    selectionType_ = SelectionType::TimeRange;
    timeRangeSelection_.startTime = startTime;
    timeRangeSelection_.endTime = endTime;
    timeRangeSelection_.trackIds = trackIds;

    // Sync with managers (clear their selections)
    TrackManager::getInstance().setSelectedTrack(INVALID_TRACK_ID);
    ClipManager::getInstance().clearClipSelection();

    if (typeChanged) {
        notifySelectionTypeChanged(SelectionType::TimeRange);
    }
    notifyTimeRangeSelectionChanged(timeRangeSelection_);
}

// ============================================================================
// Clear
// ============================================================================

void SelectionManager::clearSelection() {
    if (selectionType_ == SelectionType::None) {
        return;
    }

    selectionType_ = SelectionType::None;
    selectedTrackId_ = INVALID_TRACK_ID;
    selectedClipId_ = INVALID_CLIP_ID;
    timeRangeSelection_ = TimeRangeSelection{};

    // Sync with managers
    TrackManager::getInstance().setSelectedTrack(INVALID_TRACK_ID);
    ClipManager::getInstance().clearClipSelection();

    notifySelectionTypeChanged(SelectionType::None);
}

// ============================================================================
// Listeners
// ============================================================================

void SelectionManager::addListener(SelectionManagerListener* listener) {
    if (listener && std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end()) {
        listeners_.push_back(listener);
    }
}

void SelectionManager::removeListener(SelectionManagerListener* listener) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

// ============================================================================
// Private Notification Helpers
// ============================================================================

void SelectionManager::notifySelectionTypeChanged(SelectionType type) {
    for (auto* listener : listeners_) {
        listener->selectionTypeChanged(type);
    }
}

void SelectionManager::notifyTrackSelectionChanged(TrackId trackId) {
    for (auto* listener : listeners_) {
        listener->trackSelectionChanged(trackId);
    }
}

void SelectionManager::notifyClipSelectionChanged(ClipId clipId) {
    for (auto* listener : listeners_) {
        listener->clipSelectionChanged(clipId);
    }
}

void SelectionManager::notifyTimeRangeSelectionChanged(const TimeRangeSelection& selection) {
    for (auto* listener : listeners_) {
        listener->timeRangeSelectionChanged(selection);
    }
}

}  // namespace magica
