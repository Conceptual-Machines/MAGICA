#include "ArrangementTimelineComponent.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

ArrangementTimelineComponent::ArrangementTimelineComponent() {
    setSize(800, 30);
    
    // Add some default sections for demonstration
    addSection("Intro", 0.0, 8.0, juce::Colours::green);
    addSection("Verse 1", 8.0, 24.0, juce::Colours::blue);
    addSection("Chorus", 24.0, 40.0, juce::Colours::orange);
    addSection("Verse 2", 40.0, 56.0, juce::Colours::blue);
    addSection("Chorus", 56.0, 72.0, juce::Colours::orange);
    addSection("Bridge", 72.0, 88.0, juce::Colours::purple);
    addSection("Outro", 88.0, 120.0, juce::Colours::red);
}

void ArrangementTimelineComponent::paint(juce::Graphics& g) {
    // Fill background
    g.fillAll(DarkTheme::getColour(DarkTheme::BACKGROUND));
    
    // Draw border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(getLocalBounds(), 1);
    
    // Draw sections
    for (size_t i = 0; i < sections.size(); ++i) {
        drawSection(g, *sections[i], static_cast<int>(i) == selectedSectionIndex);
    }
    
    // Draw playhead
    g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    int playheadX = timeToPixel(playheadPosition);
    if (playheadX >= 0 && playheadX < getWidth()) {
        // Draw shadow for better visibility
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(playheadX + 1, 0, playheadX + 1, getHeight(), 5.0f);
        // Draw main playhead line
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(playheadX, 0, playheadX, getHeight(), 4.0f);
    }
}

void ArrangementTimelineComponent::resized() {
    // Zoom is now controlled by parent component for proper synchronization
    // No automatic zoom calculation here
}

void ArrangementTimelineComponent::mouseDown(const juce::MouseEvent& event) {
    int clickedSection = findSectionAtPosition(event.x, event.y);
    
    if (clickedSection >= 0) {
        selectedSectionIndex = clickedSection;
        
        // Check if clicking on section edge for resizing
        bool isStartEdge;
        if (isOnSectionEdge(event.x, clickedSection, isStartEdge)) {
            isDraggingEdge = true;
            isDraggingStart = isStartEdge;
        } else {
            isDraggingSection = true;
        }
    } else {
        selectedSectionIndex = -1;
        
        // Set playhead position
        double clickTime = pixelToTime(event.x);
        setPlayheadPosition(clickTime);
        if (onPlayheadPositionChanged) {
            onPlayheadPositionChanged(playheadPosition);
        }
    }
    
    repaint();
}

void ArrangementTimelineComponent::mouseDrag(const juce::MouseEvent& event) {
    if (selectedSectionIndex >= 0 && selectedSectionIndex < static_cast<int>(sections.size())) {
        auto& section = *sections[selectedSectionIndex];
        double dragTime = pixelToTime(event.x);
        
        if (isDraggingEdge) {
            // Resize section
            if (isDraggingStart) {
                section.startTime = juce::jmax(0.0, juce::jmin(dragTime, section.endTime - 1.0));
            } else {
                section.endTime = juce::jmax(section.startTime + 1.0, juce::jmin(dragTime, timelineLength));
            }
        } else if (isDraggingSection) {
            // Move entire section
            double sectionLength = section.endTime - section.startTime;
            double newStartTime = juce::jmax(0.0, juce::jmin(dragTime, timelineLength - sectionLength));
            section.startTime = newStartTime;
            section.endTime = newStartTime + sectionLength;
        }
        
        if (onSectionChanged) {
            onSectionChanged(selectedSectionIndex, section);
        }
        
        repaint();
    }
}

void ArrangementTimelineComponent::mouseDoubleClick(const juce::MouseEvent& event) {
    int clickedSection = findSectionAtPosition(event.x, event.y);
    
    if (clickedSection >= 0) {
        // For now, just cycle through some default names on double-click
        auto& section = *sections[clickedSection];
        static const juce::StringArray names = { "Intro", "Verse", "Chorus", "Bridge", "Outro", "Solo", "Break" };
        
        // Find current name index and move to next
        int currentIndex = names.indexOf(section.name);
        if (currentIndex >= 0) {
            section.name = names[(currentIndex + 1) % names.size()];
        } else {
            section.name = names[0]; // Default to first name if not found
        }
        repaint();
    } else {
        // Create new section at click position
        double clickTime = pixelToTime(event.x);
        double sectionLength = 16.0; // Default 16 seconds
        double startTime = juce::jmax(0.0, clickTime - sectionLength / 2.0);
        double endTime = juce::jmin(timelineLength, startTime + sectionLength);
        
        juce::String newName = getDefaultSectionName();
        addSection(newName, startTime, endTime);
        
        if (onSectionAdded) {
            onSectionAdded(newName, startTime, endTime);
        }
    }
}

void ArrangementTimelineComponent::setTimelineLength(double lengthInSeconds) {
    timelineLength = lengthInSeconds;
    repaint();
}

void ArrangementTimelineComponent::setZoom(double pixelsPerSecond) {
    zoom = pixelsPerSecond;
    repaint();
}

void ArrangementTimelineComponent::setPlayheadPosition(double position) {
    playheadPosition = position;
    repaint();
}

void ArrangementTimelineComponent::addSection(const juce::String& name, double startTime, double endTime, juce::Colour colour) {
    sections.push_back(std::make_unique<ArrangementSection>(startTime, endTime, name, colour));
    repaint();
}

void ArrangementTimelineComponent::removeSection(int index) {
    if (index >= 0 && index < static_cast<int>(sections.size())) {
        sections.erase(sections.begin() + index);
        if (selectedSectionIndex == index) {
            selectedSectionIndex = -1;
        } else if (selectedSectionIndex > index) {
            selectedSectionIndex--;
        }
        repaint();
    }
}

void ArrangementTimelineComponent::clearSections() {
    sections.clear();
    selectedSectionIndex = -1;
    repaint();
}

double ArrangementTimelineComponent::pixelToTime(int pixel) const {
    if (zoom > 0) {
        return pixel / zoom;
    }
    return 0.0;
}

int ArrangementTimelineComponent::timeToPixel(double time) const {
    return static_cast<int>(time * zoom);
}

int ArrangementTimelineComponent::findSectionAtPosition(int x, int /* y */) const {
    double clickTime = pixelToTime(x);
    
    for (size_t i = 0; i < sections.size(); ++i) {
        const auto& section = *sections[i];
        if (clickTime >= section.startTime && clickTime <= section.endTime) {
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

bool ArrangementTimelineComponent::isOnSectionEdge(int x, int sectionIndex, bool& isStartEdge) const {
    if (sectionIndex < 0 || sectionIndex >= static_cast<int>(sections.size())) {
        return false;
    }
    
    const auto& section = *sections[sectionIndex];
    int startX = timeToPixel(section.startTime);
    int endX = timeToPixel(section.endTime);
    
    const int edgeThreshold = 5; // pixels
    
    if (abs(x - startX) <= edgeThreshold) {
        isStartEdge = true;
        return true;
    } else if (abs(x - endX) <= edgeThreshold) {
        isStartEdge = false;
        return true;
    }
    
    return false;
}

void ArrangementTimelineComponent::drawSection(juce::Graphics& g, const ArrangementSection& section, bool isSelected) const {
    int startX = timeToPixel(section.startTime);
    int endX = timeToPixel(section.endTime);
    int width = endX - startX;
    
    if (width <= 0 || startX >= getWidth() || endX <= 0) {
        return;
    }
    
    // Clamp to visible area
    startX = juce::jmax(0, startX);
    endX = juce::jmin(getWidth(), endX);
    width = endX - startX;
    
    auto bounds = juce::Rectangle<int>(startX, 2, width, getHeight() - 4);
    
    // Fill section with color
    juce::Colour fillColour = section.colour.withAlpha(isSelected ? 0.8f : 0.6f);
    g.setColour(fillColour);
    g.fillRect(bounds);
    
    // Draw border
    g.setColour(section.colour);
    g.drawRect(bounds, isSelected ? 2 : 1);
    
    // Draw section name
    if (width > 40) { // Only draw text if section is wide enough
        g.setColour(juce::Colours::white);
        g.setFont(FontManager::getInstance().getUIFont(10.0f));
        g.drawText(section.name, bounds.reduced(4, 2), juce::Justification::centredLeft, true);
    }
}

juce::String ArrangementTimelineComponent::getDefaultSectionName() const {
    static const juce::StringArray defaultNames = { "Intro", "Verse", "Chorus", "Bridge", "Outro", "Solo", "Break" };
    
    // Find a name that's not already used
    for (const auto& name : defaultNames) {
        bool nameExists = false;
        for (const auto& section : sections) {
            if (section->name == name) {
                nameExists = true;
                break;
            }
        }
        if (!nameExists) {
            return name;
        }
    }
    
    // If all default names are used, create a numbered section
    return "Section " + juce::String(sections.size() + 1);
}

} // namespace magica 