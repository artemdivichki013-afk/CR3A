#pragma once
#include "PluginProcessor.h"
#include "UI/CR3ALookAndFeel.h"

class CR3AEditor : public juce::AudioProcessorEditor
{
public:
    explicit CR3AEditor (CR3AProcessor&);
    ~CR3AEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

    void setTheme (int index);

    static constexpr int baseW = 1200, baseH = 720;

    /** used by the snapshot tool */
    juce::Component& getContent();

private:
    class Content;

    CR3AProcessor& proc;
    cr3a::CR3ALookAndFeel lf;
    std::unique_ptr<Content> content;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CR3AEditor)
};
