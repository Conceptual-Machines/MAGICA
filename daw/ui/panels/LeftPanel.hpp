#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magica {

class LeftPanel : public juce::Component {
public:
    LeftPanel();
    ~LeftPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftPanel)
};

} // namespace magica 