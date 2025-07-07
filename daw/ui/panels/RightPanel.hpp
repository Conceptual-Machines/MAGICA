#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magica {

class RightPanel : public juce::Component {
public:
    RightPanel();
    ~RightPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightPanel)
};

} // namespace magica 