#include "LFOCurveEditor.hpp"

#include <algorithm>
#include <cmath>

namespace magda {

LFOCurveEditor::LFOCurveEditor() {
    setName("LFOCurveEditor");

    // Initialize with default sine-like curve (just two points for now)
    // First point at 0.0 phase, 0.5 value (center)
    CurvePoint p1;
    p1.id = nextPointId_++;
    p1.x = 0.0;
    p1.y = 0.5;
    p1.curveType = CurveType::Linear;
    points_.push_back(p1);

    // Second point at 0.5 phase, 1.0 value (peak)
    CurvePoint p2;
    p2.id = nextPointId_++;
    p2.x = 0.5;
    p2.y = 1.0;
    p2.curveType = CurveType::Linear;
    points_.push_back(p2);

    // Third point at 1.0 phase, 0.5 value (back to center)
    CurvePoint p3;
    p3.id = nextPointId_++;
    p3.x = 1.0;
    p3.y = 0.5;
    p3.curveType = CurveType::Linear;
    points_.push_back(p3);

    rebuildPointComponents();
}

LFOCurveEditor::~LFOCurveEditor() = default;

void LFOCurveEditor::setModInfo(ModInfo* mod) {
    modInfo_ = mod;
    // TODO: Load custom waveform points from ModInfo if stored
    repaint();
}

double LFOCurveEditor::getPixelsPerX() const {
    // X is phase 0-1, so pixels per X = width
    return getWidth() > 0 ? static_cast<double>(getWidth()) : 100.0;
}

double LFOCurveEditor::pixelToX(int px) const {
    int width = getWidth();
    if (width <= 0)
        return 0.0;
    return static_cast<double>(px) / width;
}

int LFOCurveEditor::xToPixel(double x) const {
    return static_cast<int>(x * getWidth());
}

const std::vector<CurvePoint>& LFOCurveEditor::getPoints() const {
    return points_;
}

void LFOCurveEditor::onPointAdded(double x, double y, CurveType curveType) {
    // Clamp x to 0-1 range
    x = juce::jlimit(0.0, 1.0, x);
    y = juce::jlimit(0.0, 1.0, y);

    CurvePoint newPoint;
    newPoint.id = nextPointId_++;
    newPoint.x = x;
    newPoint.y = y;
    newPoint.curveType = curveType;

    // Insert in sorted order by x
    auto insertPos =
        std::lower_bound(points_.begin(), points_.end(), newPoint,
                         [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
    points_.insert(insertPos, newPoint);

    rebuildPointComponents();
    notifyWaveformChanged();
}

void LFOCurveEditor::onPointMoved(uint32_t pointId, double newX, double newY) {
    // Clamp values
    newX = juce::jlimit(0.0, 1.0, newX);
    newY = juce::jlimit(0.0, 1.0, newY);

    for (auto& point : points_) {
        if (point.id == pointId) {
            point.x = newX;
            point.y = newY;
            break;
        }
    }

    // Re-sort points by x position
    std::sort(points_.begin(), points_.end(),
              [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });

    rebuildPointComponents();
    notifyWaveformChanged();
}

void LFOCurveEditor::onPointDeleted(uint32_t pointId) {
    // Don't delete if only 2 points remain
    if (points_.size() <= 2)
        return;

    points_.erase(std::remove_if(points_.begin(), points_.end(),
                                 [pointId](const CurvePoint& p) { return p.id == pointId; }),
                  points_.end());

    if (selectedPointId_ == pointId) {
        selectedPointId_ = INVALID_CURVE_POINT_ID;
    }

    rebuildPointComponents();
    notifyWaveformChanged();
}

void LFOCurveEditor::onPointSelected(uint32_t pointId) {
    selectedPointId_ = pointId;

    // Update selection state on point components
    for (auto& pc : pointComponents_) {
        pc->setSelected(pc->getPointId() == pointId);
    }

    repaint();
}

void LFOCurveEditor::onTensionChanged(uint32_t pointId, double tension) {
    for (auto& point : points_) {
        if (point.id == pointId) {
            point.tension = tension;
            break;
        }
    }

    repaint();
    notifyWaveformChanged();
}

void LFOCurveEditor::onHandlesChanged(uint32_t pointId, const CurveHandleData& inHandle,
                                      const CurveHandleData& outHandle) {
    for (auto& point : points_) {
        if (point.id == pointId) {
            point.inHandle = inHandle;
            point.outHandle = outHandle;
            break;
        }
    }

    repaint();
    notifyWaveformChanged();
}

void LFOCurveEditor::paintGrid(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Horizontal center line (0.5 value)
    g.setColour(juce::Colour(0x20FFFFFF));
    int centerY = bounds.getHeight() / 2;
    g.drawHorizontalLine(centerY, 0.0f, static_cast<float>(bounds.getWidth()));

    // Quarter lines (0.25, 0.75 value)
    g.setColour(juce::Colour(0x10FFFFFF));
    g.drawHorizontalLine(bounds.getHeight() / 4, 0.0f, static_cast<float>(bounds.getWidth()));
    g.drawHorizontalLine(bounds.getHeight() * 3 / 4, 0.0f, static_cast<float>(bounds.getWidth()));

    // Vertical quarter lines (phase 0.25, 0.5, 0.75)
    g.setColour(juce::Colour(0x10FFFFFF));
    for (int i = 1; i < 4; ++i) {
        int x = bounds.getWidth() * i / 4;
        g.drawVerticalLine(x, 0.0f, static_cast<float>(bounds.getHeight()));
    }

    // Phase 0.5 line (center) slightly brighter
    g.setColour(juce::Colour(0x20FFFFFF));
    g.drawVerticalLine(bounds.getWidth() / 2, 0.0f, static_cast<float>(bounds.getHeight()));
}

void LFOCurveEditor::notifyWaveformChanged() {
    if (onWaveformChanged) {
        onWaveformChanged();
    }

    // TODO: Store waveform data in ModInfo for persistence
}

}  // namespace magda
