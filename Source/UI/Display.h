#pragma once
#include "../PluginProcessor.h"
#include "Widgets.h"

namespace cr3a
{
/**
    Central display: live spectrum of the output, value read-out for the
    control under the mouse (otherwise the preset name) and IN / OUT / GR meters.
*/
class DisplayComponent : public juce::Component, private juce::Timer
{
public:
    explicit DisplayComponent (CR3AProcessor& p) : proc (p), fft (11)
    {
        window.resize (kN);
        for (int i = 0; i < kN; ++i)
            window[(size_t) i] = (float) (0.5 - 0.5 * std::cos (kTwoPi * i / kN));
        spec.assign ((size_t) (2 * kN), 0.0f);
        bars.assign ((size_t) kBars, -100.0f);
        startTimerHz (30);
    }

    void setReadout (const juce::String& name, const juce::String& value)
    {
        hoverName = name; hoverValue = value;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto& lf = lfOf (*this);
        const auto& th = lf.t();
        auto r = getLocalBounds().toFloat();
        lf.paintDisplayBezel (g, r);

        auto screen = r.reduced (th.look == Look::Win98 ? 4.0f : 2.0f);
        auto meters = screen.removeFromRight (88.0f);
        screen.removeFromRight (6.0f);

        // spectrum -----------------------------------------------------------
        g.saveState();
        g.reduceClipRegion (screen.toNearestInt());
        const auto accent = lf.col (th.look == Look::Win98 ? 0xff22ff44 : (th.look == Look::Paper || th.look == Look::Graphite ? th.displayText : th.accent));

        // grid: 100, 1k, 10k
        g.setColour (lf.col (th.displayText).withAlpha (0.08f));
        for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        {
            const float x = screen.getX() + screen.getWidth() * freqToX (f);
            g.fillRect (x, screen.getY(), 1.0f, screen.getHeight());
        }
        g.setFont (lf.font (9.5f, true));
        g.setColour (lf.col (th.displayText).withAlpha (0.35f));
        for (auto pr : { std::pair<float, const char*> { 100.0f, "100" }, { 1000.0f, "1k" }, { 10000.0f, "10k" } })
            g.drawText (pr.second, juce::Rectangle<float> (screen.getX() + screen.getWidth() * freqToX (pr.first) + 3.0f, screen.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::centredLeft);

        juce::Path line;
        for (int i = 0; i < kBars; ++i)
        {
            const float v = juce::jlimit (0.0f, 1.0f, (bars[(size_t) i] + 96.0f) / 80.0f);
            const float x = screen.getX() + screen.getWidth() * (float) i / (float) (kBars - 1);
            const float y = screen.getBottom() - 4.0f - v * (screen.getHeight() - 10.0f);
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }
        juce::Path area (line);
        area.lineTo (screen.getRight(), screen.getBottom()); area.lineTo (screen.getX(), screen.getBottom()); area.closeSubPath();
        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.50f), 0, screen.getY(), accent.withAlpha (0.03f), 0, screen.getBottom(), false));
        g.fillPath (area);
        g.setColour (accent.withAlpha (0.95f));
        g.strokePath (line, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));

        // read-out -------------------------------------------------------------
        const bool idle = hoverName.isEmpty();
        const juce::String t1 = idle ? juce::String ("PRESET") : hoverName.toUpperCase();
        const juce::String t2 = idle ? proc.getPresetName() : hoverValue;
        auto textArea = screen.reduced (14.0f, 10.0f).removeFromLeft (screen.getWidth() * 0.6f);
        g.setFont (lf.font (11.0f, true).withExtraKerningFactor (0.14f));
        g.setColour (lf.col (th.displayText).withAlpha (0.55f));
        g.drawText (t1, textArea.removeFromTop (16.0f), juce::Justification::centredLeft);
        g.setFont (lf.font (26.0f, true));
        g.setColour (lf.col (th.displayText));
        g.drawText (t2, textArea.removeFromTop (34.0f), juce::Justification::centredLeft, true);

        // meters -----------------------------------------------------------------
        g.restoreState();
        auto m = meters.reduced (4.0f, 8.0f);
        const float colW = (m.getWidth() - 12.0f) / 3.0f;
        auto labelArea = m.removeFromBottom (14.0f);
        const char* names[3] = { "IN", "OUT", "GR" };
        const float vals[3] = { dbToMeter (inDb), dbToMeter (outDb), juce::jlimit (0.0f, 1.0f, grDb / 18.0f) };
        for (int i = 0; i < 3; ++i)
        {
            auto bar = juce::Rectangle<float> (m.getX() + (float) i * (colW + 6.0f), m.getY(), colW, m.getHeight());
            lf.paintMeterBar (g, bar, vals[i], i == 2);
            g.setFont (lf.font (9.5f, true));
            g.setColour (lf.col (th.displayText).withAlpha (0.6f));
            g.drawText (names[i], juce::Rectangle<float> (bar.getX() - 4, labelArea.getY(), bar.getWidth() + 8, 14.0f), juce::Justification::centred);
        }
    }

private:
    static constexpr int kN = 2048, kBars = 110;

    static float freqToX (float f) { return std::log (f / 25.0f) / std::log (18000.0f / 25.0f); }
    static float dbToMeter (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); }

    void timerCallback() override
    {
        // meters
        auto fall = [] (float& state, float newDb, float dt)
        {
            if (newDb > state) state = newDb; else state = std::max (newDb, state - 28.0f * dt);
        };
        const float dt = 1.0f / 30.0f;
        fall (inDb, linToDb (proc.inLevel.exchange (0.0f)), dt);
        fall (outDb, linToDb (proc.outLevel.exchange (0.0f)), dt);
        const float gr = std::max (proc.compGR.exchange (0.0f), proc.limGR.exchange (0.0f));
        if (gr > grDb) grDb = gr; else grDb = std::max (gr, grDb - 36.0f * dt);

        // spectrum
        const int wp = proc.scopePos.load();
        const int size = CR3AProcessor::scopeSize;
        for (int i = 0; i < kN; ++i)
        {
            const int idx = (wp - kN + i + size * 2) % size;
            spec[(size_t) (2 * i)] = proc.scope[(size_t) idx] * window[(size_t) i];
            spec[(size_t) (2 * i + 1)] = 0.0f;
        }
        fft.transform (spec.data(), false);

        const double sr = proc.currentSampleRate.load();
        const float binHz = (float) (sr / kN);
        for (int i = 0; i < kBars; ++i)
        {
            const float f0 = 25.0f * std::pow (18000.0f / 25.0f, (float) i / (float) kBars);
            const float f1 = 25.0f * std::pow (18000.0f / 25.0f, (float) (i + 1) / (float) kBars);
            const int b0 = std::max (1, (int) std::floor (f0 / binHz)), b1 = std::min (kN / 2 - 1, std::max (b0, (int) std::ceil (f1 / binHz)));
            float m = 0.0f;
            for (int b = b0; b <= b1; ++b)
                m = std::max (m, std::sqrt (spec[(size_t) (2 * b)] * spec[(size_t) (2 * b)] + spec[(size_t) (2 * b + 1)] * spec[(size_t) (2 * b + 1)]));
            float db = linToDb (m / (kN * 0.25f) + 1.0e-9f);
            const float fc = 0.5f * (f0 + f1);
            db += 4.5f * std::log2 (fc / 1000.0f);                    // tilt for a flatter-looking picture
            float& s = bars[(size_t) i];
            s = db > s ? s + (db - s) * 0.6f : s + (db - s) * 0.12f;
        }
        // light smoothing along the frequency axis
        for (int i = 1; i < kBars - 1; ++i) bars[(size_t) i] = 0.25f * bars[(size_t) (i - 1)] + 0.5f * bars[(size_t) i] + 0.25f * bars[(size_t) (i + 1)];
        repaint();
    }

    CR3AProcessor& proc;
    FFT fft;
    std::vector<float> window, spec, bars;
    juce::String hoverName, hoverValue;
    float inDb = -100.0f, outDb = -100.0f, grDb = 0.0f;
};
} // namespace cr3a
