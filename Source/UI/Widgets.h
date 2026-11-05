#pragma once
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "CR3ALookAndFeel.h"
#include "../DSP/Utils.h"

namespace cr3a
{
inline CR3ALookAndFeel& lfOf (juce::Component& c)
{
    return static_cast<CR3ALookAndFeel&> (c.getLookAndFeel());
}

// ============================================================================
//  A rotary knob with caption below and value text above.
// ============================================================================
class KnobCell : public juce::Component
{
public:
    using Readout = std::function<void (const juce::String& name, const juce::String& value)>;

    KnobCell (juce::AudioProcessorValueTreeState& apvts, const char* paramId, const juce::String& captionText,
              bool bipolar = false, float scale = 1.0f)
        : caption (captionText), knobScale (scale)
    {
        param = apvts.getParameter (paramId);
        stepped = dynamic_cast<juce::AudioParameterChoice*> (param) != nullptr;

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        slider.getProperties().set ("bipolar", bipolar);
        if (stepped)
            slider.getProperties().set ("steps", dynamic_cast<juce::AudioParameterChoice*> (param)->choices.size());
        addAndMakeVisible (slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.textFromValueFunction = [this] (double v) { return valueText ((float) v); };
        slider.addMouseListener (this, false);
        slider.onValueChange = [this]
        {
            repaint();
            if (hovering && onReadout) onReadout (param->getName (32), valueText ((float) slider.getValue()));
        };
    }

    Readout onReadout;

    juce::String valueText (float realValue) const
    {
        return param->getText (param->convertTo0to1 (realValue), 32);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop (14);
        b.removeFromBottom (18);
        const int d = (int) ((float) juce::jmin (b.getWidth(), b.getHeight()) * knobScale);
        slider.setBounds (b.withSizeKeepingCentre (d, d));
    }

    void paint (juce::Graphics& g) override
    {
        auto& lf = lfOf (*this);
        const auto& th = lf.t();
        auto b = getLocalBounds();
        auto top = b.removeFromTop (14);
        auto bottom = b.removeFromBottom (18);

        const bool showValue = stepped || hovering || slider.isMouseButtonDown();
        if (showValue)
        {
            g.setFont (lf.font (11.5f, true));
            g.setColour (lf.col (th.accent2 == th.accent ? th.text : (th.look == Look::Graphite ? th.text : th.accent)));
            g.drawText (valueText ((float) slider.getValue()), top, juce::Justification::centred);
        }
        g.setFont (lf.font (10.5f, true).withExtraKerningFactor (0.08f));
        g.setColour (lf.col (th.textDim).withAlpha (0.4f + 0.6f * (hovering ? 1.0f : 0.9f)));
        if (hovering) g.setColour (lf.col (th.text));
        g.drawText (caption.toUpperCase(), bottom, juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent&) override { hovering = true;  repaint(); if (onReadout) onReadout (param->getName (32), valueText ((float) slider.getValue())); }
    void mouseExit  (const juce::MouseEvent&) override { hovering = false; repaint(); if (onReadout) onReadout ({}, {}); }

    juce::Slider slider;

private:
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String caption;
    float knobScale = 1.0f;
    bool stepped = false, hovering = false;
};

// ============================================================================
//  Toggle button wrappers
// ============================================================================
class SwitchCell : public juce::Component
{
public:
    SwitchCell (juce::AudioProcessorValueTreeState& apvts, const char* paramId, const juce::String& text, bool power = false)
    {
        button.setButtonText (text);
        button.getProperties().set ("kind", power ? "power" : "switch");
        addAndMakeVisible (button);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramId, button);
    }
    void resized() override { button.setBounds (getLocalBounds()); }
    juce::ToggleButton button;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// ============================================================================
//  Panel for one effect module with optional power button in the header.
// ============================================================================
class ModulePanel : public juce::Component
{
public:
    ModulePanel (juce::AudioProcessorValueTreeState& apvts, const juce::String& titleText, const char* powerId = nullptr)
        : title (titleText)
    {
        if (powerId != nullptr)
        {
            power = std::make_unique<SwitchCell> (apvts, powerId, "", true);
            addAndMakeVisible (*power);
            auto* prm = apvts.getParameter (powerId);
            on = prm->getValue() > 0.5f;
            stateAttach = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v)
            {
                on = v > 0.5f;
                updateDim();
                repaint();
            });
            stateAttach->sendInitialUpdate();
        }
    }

    void paint (juce::Graphics& g) override
    {
        lfOf (*this).paintPanel (g, getLocalBounds().toFloat().reduced (1.0f), title, on, power != nullptr);
    }

    void resized() override
    {
        if (power) power->setBounds (10, 7, 16, 16);
        updateDim();
    }

    void childrenAdded() { updateDim(); }

    void updateDim()
    {
        for (auto* c : getChildren())
            if (c != power.get())
                c->setAlpha (on ? 1.0f : 0.42f);
    }

    /** places a cell on a grid inside the panel body */
    void place (juce::Component& c, int col, int row, int cellW, int cellH, int padX = 12, int padY = 4)
    {
        addAndMakeVisible (c);
        c.setBounds (padX + col * cellW, (int) CR3ALookAndFeel::headerH + padY + row * cellH, cellW, cellH);
    }

    juce::String title;
    std::unique_ptr<SwitchCell> power;
    bool on = true;

private:
    std::unique_ptr<juce::ParameterAttachment> stateAttach;
};

// ============================================================================
//  Small live EQ response curve
// ============================================================================
class EqCurve : public juce::Component, private juce::Timer
{
public:
    explicit EqCurve (juce::AudioProcessorValueTreeState& a) : apvts (a) { startTimerHz (20); }

    void paint (juce::Graphics& g) override
    {
        auto& lf = lfOf (*this);
        const auto& th = lf.t();
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (lf.col (th.displayBg).withAlpha (th.look == Look::Glass ? 0.8f : 1.0f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (lf.col (th.displayGrid));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        auto in = r.reduced (4.0f, 5.0f);

        // grid
        g.setColour (lf.col (th.displayText).withAlpha (0.08f));
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (in.getY() + in.getHeight() * (float) i / 4.0f), in.getX(), in.getRight());
        g.setColour (lf.col (th.displayText).withAlpha (0.16f));
        g.drawHorizontalLine ((int) in.getCentreY(), in.getX(), in.getRight());

        const double fs = 48000.0;
        const float gains[6] = { get ("eqLowEnd"), get ("eqLow"), get ("eqMid"), get ("eqMidHigh"), get ("eqMidEnd"), get ("eqAir") };
        const BiquadCoef cs[6] = {
            BiquadCoef::lowShelf (fs, 80.0, 0.7, gains[0]), BiquadCoef::peak (fs, 220.0, 0.85, gains[1]),
            BiquadCoef::peak (fs, 700.0, 0.85, gains[2]),   BiquadCoef::peak (fs, 2200.0, 0.85, gains[3]),
            BiquadCoef::peak (fs, 4000.0, 0.95, gains[4]),  BiquadCoef::highShelf (fs, 12000.0, 0.7, gains[5]) };

        juce::Path p;
        const int pts = 96;
        for (int i = 0; i < pts; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, (double) i / (double) (pts - 1));
            const double w = kTwoPi * f / fs;
            const double c1 = std::cos (w), c2 = std::cos (2 * w);
            double db = 0.0;
            for (auto& c : cs)
            {
                const double num = c.b0 * c.b0 + c.b1 * c.b1 + c.b2 * c.b2 + 2 * (c.b0 * c.b1 + c.b1 * c.b2) * c1 + 2 * c.b0 * c.b2 * c2;
                const double den = 1 + c.a1 * c.a1 + c.a2 * c.a2 + 2 * (c.a1 + c.a1 * c.a2) * c1 + 2 * c.a2 * c2;
                db += 10.0 * std::log10 (std::max (1.0e-12, num / den));
            }
            const float y = in.getCentreY() - (float) (db / 14.0) * in.getHeight() * 0.5f;
            const float x = in.getX() + in.getWidth() * (float) i / (float) (pts - 1);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        juce::Path fill (p);
        fill.lineTo (in.getRight(), in.getCentreY()); fill.lineTo (in.getX(), in.getCentreY()); fill.closeSubPath();
        g.setColour (lf.col (th.accent).withAlpha (0.18f));
        g.fillPath (fill);
        g.setColour (lf.col (th.look == Look::Paper || th.look == Look::Graphite ? th.displayText : th.accent));
        g.strokePath (p, juce::PathStrokeType (1.8f));
    }

private:
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void timerCallback() override
    {
        float sum = 0.0f;
        for (auto id : { "eqLowEnd", "eqLow", "eqMid", "eqMidHigh", "eqMidEnd", "eqAir" }) sum += get (id) * (float) (id[3] + id[4]);
        if (sum != last) { last = sum; repaint(); }
    }
    juce::AudioProcessorValueTreeState& apvts;
    float last = 1.0e9f;
};
} // namespace cr3a
