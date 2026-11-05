#pragma once
#include "Parameters.h"

namespace cr3a
{
struct PresetValue { const char* id; float value; };

struct Preset
{
    const char* name;
    std::vector<PresetValue> values;    // real (denormalised) values, everything else = default
};

inline const std::vector<Preset>& factoryPresets()
{
    using namespace pid;
    static const std::vector<Preset> list =
    {
        { "Init", {} },

        { "Clean Vox", {
            { eqLowEnd, -2.0f }, { eqMidEnd, 1.5f }, { eqAir, 2.5f }, { eqHarsh, 35.0f },
            { compAmt, 32.0f }, { compGain, 0.0f }, { sat, 12.0f },
            { revMix, 12.0f }, { revDecay, 35.0f }, { revPre, 25.0f } } },

        { "Dark Vox", {
            { formant, -3.5f }, { eqLowEnd, 1.5f }, { eqMidHigh, -1.5f }, { eqAir, 3.5f }, { eqHarsh, 45.0f },
            { compAmt, 58.0f }, { compGain, 1.0f }, { sat, 35.0f }, { dist, 28.0f }, { distTone, 48.0f },
            { dblMix, 30.0f }, { dblWidth, 80.0f }, { dblPan, 25.0f },
            { dlyMix, 16.0f }, { dlyTime, 2.0f }, { dlyDotted, 1.0f }, { dlyFb, 38.0f }, { dlyCut, 55.0f },
            { revMix, 16.0f }, { revDecay, 46.0f }, { revTone, 45.0f }, { width, 125.0f } } },

        { "Deep Voice", {
            { formant, -6.0f }, { eqLowEnd, 3.0f }, { eqLow, 1.5f }, { eqMidEnd, -1.0f }, { eqAir, 1.5f },
            { compAmt, 45.0f }, { sat, 25.0f }, { revMix, 8.0f }, { revDecay, 30.0f } } },

        { "Hyper Pop", {
            { formant, 3.0f }, { eqLowEnd, -3.0f }, { eqMidEnd, 2.0f }, { eqAir, 6.0f }, { eqHarsh, 55.0f },
            { compAmt, 62.0f }, { compGain, 1.0f }, { sat, 30.0f }, { dist, 20.0f }, { distTone, 75.0f },
            { hyper, 70.0f }, { ott, 42.0f }, { dblMix, 42.0f }, { dblType, 1.0f }, { dblWidth, 100.0f }, { dblPan, 45.0f },
            { dlyMix, 14.0f }, { dlyTime, 1.0f }, { dlyFb, 30.0f }, { dlyMode, 1.0f },
            { revMix, 18.0f }, { revDecay, 52.0f }, { width, 150.0f } } },

        { "Crunch Lead", {
            { formant, -2.0f }, { eqLow, -1.5f }, { eqMidHigh, 2.0f }, { eqAir, 2.0f }, { eqHarsh, 50.0f },
            { compAmt, 66.0f }, { compGain, 0.0f }, { sat, 45.0f }, { dist, 62.0f }, { distTone, 50.0f },
            { ott, 25.0f }, { dblMix, 25.0f }, { revMix, 10.0f }, { revDecay, 30.0f } } },

        { "Wide Doubles", {
            { eqAir, 2.0f }, { compAmt, 35.0f }, { dblMix, 55.0f }, { dblType, 1.0f }, { dblWidth, 100.0f },
            { dblDetune, 18.0f }, { dblPan, 30.0f }, { width, 140.0f }, { revMix, 14.0f } } },

        { "Space Echo", {
            { eqLowEnd, -2.0f }, { eqAir, 2.0f }, { compAmt, 30.0f },
            { dlyMix, 34.0f }, { dlyTime, 3.0f }, { dlyFb, 52.0f }, { dlyCut, 46.0f }, { dlyMode, 2.0f },
            { revMix, 30.0f }, { revDecay, 72.0f }, { revPre, 45.0f }, { revTone, 42.0f } } },

        { "Lo-Fi Radio", {
            { eqLowEnd, -12.0f }, { eqLow, -5.0f }, { eqMid, 4.0f }, { eqMidHigh, 5.0f }, { eqAir, -8.0f },
            { compAmt, 70.0f }, { sat, 60.0f }, { dist, 45.0f }, { distTone, 22.0f }, { width, 20.0f } } },

        { "Smooth Pop", {
            { eqLowEnd, -1.5f }, { eqMid, -1.0f }, { eqMidEnd, 1.5f }, { eqAir, 3.0f }, { eqHarsh, 55.0f },
            { compAmt, 48.0f }, { compGain, 0.5f }, { sat, 18.0f }, { ott, 12.0f },
            { dblMix, 16.0f }, { dblWidth, 60.0f }, { revMix, 15.0f }, { revDecay, 42.0f }, { width, 115.0f } } },
    };
    return list;
}

/** Sets every parameter to its default and then applies the preset values. */
inline void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset)
{
    for (auto* param : apvts.processor.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const juce::String id = r->paramID;
            if (id == pid::bypass || id == pid::inGain || id == pid::outGain || id == pid::mix
                || id == pid::limiter || id == pid::mono)
                continue;                                    // global stage is not part of presets
            r->beginChangeGesture();
            r->setValueNotifyingHost (r->getDefaultValue());
            r->endChangeGesture();
        }

    for (auto& v : preset.values)
        if (auto* r = apvts.getParameter (v.id))
        {
            r->beginChangeGesture();
            r->setValueNotifyingHost (r->convertTo0to1 (v.value));
            r->endChangeGesture();
        }
}
} // namespace cr3a
