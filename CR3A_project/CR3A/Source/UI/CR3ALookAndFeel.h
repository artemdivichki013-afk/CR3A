#pragma once
#include "Theme.h"

namespace cr3a
{
class CR3ALookAndFeel : public juce::LookAndFeel_V4
{
public:
    CR3ALookAndFeel();

    void setTheme (int index);
    int  getThemeIndex() const noexcept { return themeIdx; }
    const Theme& t() const noexcept { return themes()[(size_t) themeIdx]; }
    juce::Colour col (juce::uint32 v) const { return juce::Colour (v); }

    // fonts
    juce::Font font (float height, bool bold = false) const;
    juce::Font getLabelFont (juce::Label&) override                  { return font (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return font (juce::jmin (13.0f, (float) h * 0.55f), true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override            { return font (13.0f, true); }
    juce::Font getPopupMenuFont() override                           { return font (13.0f); }

    // ---- custom painting used by the components
    void paintBackground (juce::Graphics&, int w, int h);
    void paintPanel (juce::Graphics&, juce::Rectangle<float> r, const juce::String& title, bool on, bool hasPower);
    void paintDisplayBezel (juce::Graphics&, juce::Rectangle<float> r);
    void paintKnob (juce::Graphics&, juce::Rectangle<float> area, float pos, bool bipolar, int steps, bool hover, bool drag);
    void paintSwitch (juce::Graphics&, juce::Rectangle<float> r, bool on, bool hover, const juce::String& label);
    void paintPower (juce::Graphics&, juce::Rectangle<float> r, bool on, bool hover);
    void paintLogo (juce::Graphics&, juce::Rectangle<float> r, juce::Colour c);
    void paintMeterBar (juce::Graphics&, juce::Rectangle<float> r, float v01, bool reductionDown);

    /** area inside a panel below the header */
    static constexpr float headerH = 30.0f;

    // ---- overrides
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

private:
    void applyColours();
    void rimGloss (juce::Graphics&, juce::Rectangle<float> r, float radius, float alpha);
    void bevel (juce::Graphics&, juce::Rectangle<float> r, bool raised);

    int themeIdx = 0;
    juce::Image logoMask;
    juce::StringArray availableFonts;
};
} // namespace cr3a
