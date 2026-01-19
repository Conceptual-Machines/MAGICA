#pragma once

#include <vector>

#include "ClipTypes.hpp"
#include "TrackTypes.hpp"

namespace magica {

/**
 * @brief Selection types in the DAW
 */
enum class SelectionType {
    None,      // Nothing selected
    Track,     // Track selected (for mixer/inspector)
    Clip,      // Clip selected (for inspector/editing)
    TimeRange  // Time range selected (for operations)
};

/**
 * @brief Time range selection data
 */
struct TimeRangeSelection {
    double startTime = 0.0;
    double endTime = 0.0;
    std::vector<TrackId> trackIds;  // Which tracks are included

    bool isValid() const {
        return endTime > startTime && !trackIds.empty();
    }

    double getLength() const {
        return endTime - startTime;
    }
};

/**
 * @brief Listener interface for selection changes
 */
class SelectionManagerListener {
  public:
    virtual ~SelectionManagerListener() = default;

    virtual void selectionTypeChanged(SelectionType newType) = 0;
    virtual void trackSelectionChanged([[maybe_unused]] TrackId trackId) {}
    virtual void clipSelectionChanged([[maybe_unused]] ClipId clipId) {}
    virtual void timeRangeSelectionChanged([[maybe_unused]] const TimeRangeSelection& selection) {}
};

/**
 * @brief Singleton manager that coordinates selection state across the DAW
 *
 * Ensures only one type of selection is active at a time (track OR clip OR range)
 * and notifies listeners of changes.
 */
class SelectionManager {
  public:
    static SelectionManager& getInstance();

    // Prevent copying
    SelectionManager(const SelectionManager&) = delete;
    SelectionManager& operator=(const SelectionManager&) = delete;

    // ========================================================================
    // Selection State
    // ========================================================================

    SelectionType getSelectionType() const {
        return selectionType_;
    }

    // ========================================================================
    // Track Selection
    // ========================================================================

    /**
     * @brief Select a track (clears clip and range selection)
     */
    void selectTrack(TrackId trackId);

    /**
     * @brief Get the currently selected track
     * @return INVALID_TRACK_ID if no track selected
     */
    TrackId getSelectedTrack() const {
        return selectedTrackId_;
    }

    // ========================================================================
    // Clip Selection
    // ========================================================================

    /**
     * @brief Select a clip (clears track and range selection)
     */
    void selectClip(ClipId clipId);

    /**
     * @brief Get the currently selected clip
     * @return INVALID_CLIP_ID if no clip selected
     */
    ClipId getSelectedClip() const {
        return selectedClipId_;
    }

    // ========================================================================
    // Time Range Selection
    // ========================================================================

    /**
     * @brief Set a time range selection (clears track and clip selection)
     */
    void selectTimeRange(double startTime, double endTime, const std::vector<TrackId>& trackIds);

    /**
     * @brief Get the current time range selection
     */
    const TimeRangeSelection& getTimeRangeSelection() const {
        return timeRangeSelection_;
    }

    /**
     * @brief Check if there's a valid time range selection
     */
    bool hasTimeRangeSelection() const {
        return selectionType_ == SelectionType::TimeRange && timeRangeSelection_.isValid();
    }

    // ========================================================================
    // Clear
    // ========================================================================

    /**
     * @brief Clear all selections
     */
    void clearSelection();

    // ========================================================================
    // Listeners
    // ========================================================================

    void addListener(SelectionManagerListener* listener);
    void removeListener(SelectionManagerListener* listener);

  private:
    SelectionManager();
    ~SelectionManager() = default;

    SelectionType selectionType_ = SelectionType::None;
    TrackId selectedTrackId_ = INVALID_TRACK_ID;
    ClipId selectedClipId_ = INVALID_CLIP_ID;
    TimeRangeSelection timeRangeSelection_;

    std::vector<SelectionManagerListener*> listeners_;

    void notifySelectionTypeChanged(SelectionType type);
    void notifyTrackSelectionChanged(TrackId trackId);
    void notifyClipSelectionChanged(ClipId clipId);
    void notifyTimeRangeSelectionChanged(const TimeRangeSelection& selection);
};

}  // namespace magica
