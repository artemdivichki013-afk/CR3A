#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace cr3a
{
namespace pid
{
    // global
    inline constexpr const char* inGain   = "inGain";
    inline constexpr const char* outGain  = "outGain";
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* bypass   = "bypass";
    inline constexpr const char* limiter  = "limiter";
    inline constexpr const char* mono     = "mono";
    // formant
    inline constexpr const char* formantOn = "formantOn";
    inline constexpr const char* formant   = "formant";
    // doubler
    inline constexpr const char* dblOn     = "dblOn";
    inline constexpr const char* dblType   = "dblType";
    inline constexpr const char* dblMix    = "dblMix";
    inline constexpr const char* dblWidth  = "dblWidth";
    inline constexpr const char* dblPan    = "dblPan";
    inline constexpr const char* dblDetune = "dblDetune";
    inline constexpr const char* dblTime   = "dblTime";
    // eq
    inline constexpr const char* eqOn      = "eqOn";
    inline constexpr const char* eqLowEnd  = "eqLowEnd";
    inline constexpr const char* eqLow     = "eqLow";
    inline constexpr const char* eqMid     = "eqMid";
    inline constexpr const char* eqMidHigh = "eqMidHigh";
    inline constexpr const char* eqMidEnd  = "eqMidEnd";
    inline constexpr const char* eqAir     = "eqAir";
    inline constexpr const char* eqHarsh   = "eqHarsh";
    // comp
    inline constexpr const char* compOn    = "compOn";
    inline constexpr const char* compAmt   = "compAmt";
    inline constexpr const char* compGain  = "compGain";
    // saturation
    inline constexpr const char* satOn     = "satOn";
    inline constexpr const char* sat       = "sat";
    inline constexpr const char* dist      = "dist";
    inline constexpr const char* distTone  = "distTone";
    // ott
    inline constexpr const char* ottOn     = "ottOn";
    inline constexpr const char* hyper     = "hyper";
    inline constexpr const char* ott       = "ott";
    // delay
    inline constexpr const char* dlyOn     = "dlyOn";
    inline constexpr const char* dlyMix    = "dlyMix";
    inline constexpr const char* dlyTime   = "dlyTime";
    inline constexpr const char* dlyDotted = "dlyDotted";
    inline constexpr const char* dlyFb     = "dlyFb";
    inline constexpr const char* dlyCut    = "dlyCut";
    inline constexpr const char* dlyMode   = "dlyMode";
    // reverb
    inline constexpr const char* revOn     = "revOn";
    inline constexpr const char* revMix    = "revMix";
    inline constexpr const char* revDecay  = "revDecay";
    inline constexpr const char* revPre    = "revPre";
    inline constexpr const char* revTone   = "revTone";
    // width
    inline constexpr const char* widthOn   = "widthOn";
    inline constexpr const char* width     = "width";
}

inline const juce::StringArray& delayTimeNames()  { static const juce::StringArray s { "1/32", "1/16", "1/8", "1/4", "1/2", "1/1" }; return s; }
inline const juce::StringArray& delayModeNames()  { static const juce::StringArray s { "Stereo", "Ping-Pong", "Tape" }; return s; }
inline const juce::StringArray& doublerTypeNames(){ static const juce::StringArray s { "Dual", "Quad", "Ping-Pong" }; return s; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using Attr = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto fmtDb  = [] (float v, int) { return String (v, 1) + " dB"; };
    auto fmtPct = [] (float v, int) { return String (roundToInt (v)) + " %"; };
    auto fmtSt  = [] (float v, int) { return String (v, 1) + " st"; };
    auto fmtMs  = [] (float v, int) { return String (roundToInt (v)) + " ms"; };
    auto fmtCt  = [] (float v, int) { return String (roundToInt (v)) + " ct"; };

    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, std::function<String (float, int)> fmt, float step = 0.01f)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                        NormalisableRange<float> (lo, hi, step), def, Attr().withStringFromValueFunction (fmt)));
    };
    auto B = [&] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto C = [&] (const char* id, const char* name, const StringArray& items, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };

    // global
    F (pid::inGain,  "Input",  -24.0f, 24.0f, 0.0f,  fmtDb, 0.1f);
    F (pid::outGain, "Output", -24.0f, 12.0f, 0.0f,  fmtDb, 0.1f);
    F (pid::mix,     "Mix",      0.0f, 100.0f, 100.0f, fmtPct, 1.0f);
    B (pid::bypass,  "Bypass", false);
    B (pid::limiter, "Limiter", true);
    B (pid::mono,    "Mono Check", false);

    // formant
    B (pid::formantOn, "Formant On", true);
    F (pid::formant,   "Formant", -12.0f, 12.0f, 0.0f, fmtSt, 0.1f);

    // doubler
    B (pid::dblOn, "Doubler On", true);
    C (pid::dblType, "Doubler Type", doublerTypeNames(), 0);
    F (pid::dblMix,    "Doubler Mix",    0.0f, 100.0f, 0.0f,  fmtPct, 1.0f);
    F (pid::dblWidth,  "Doubler Width",  0.0f, 100.0f, 70.0f, fmtPct, 1.0f);
    F (pid::dblPan,    "Doubler Pan Auto", 0.0f, 100.0f, 0.0f, fmtPct, 1.0f);
    F (pid::dblDetune, "Doubler Detune", 0.0f, 40.0f, 12.0f, fmtCt, 1.0f);
    F (pid::dblTime,   "Doubler Time",   8.0f, 60.0f, 24.0f, fmtMs, 1.0f);

    // eq
    B (pid::eqOn, "EQ On", true);
    F (pid::eqLowEnd,  "EQ Low End",  -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);
    F (pid::eqLow,     "EQ Low",      -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);
    F (pid::eqMid,     "EQ Mid",      -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);
    F (pid::eqMidHigh, "EQ Mid High", -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);
    F (pid::eqMidEnd,  "EQ Mid End",  -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);
    F (pid::eqAir,     "EQ Air",      -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);
    F (pid::eqHarsh,   "EQ Harsh",      0.0f, 100.0f, 0.0f, fmtPct, 1.0f);

    // comp
    B (pid::compOn, "Comp On", true);
    F (pid::compAmt,  "Comp",       0.0f, 100.0f, 0.0f, fmtPct, 1.0f);
    F (pid::compGain, "Comp Gain", -12.0f, 12.0f, 0.0f, fmtDb, 0.1f);

    // saturation
    B (pid::satOn, "Saturation On", true);
    F (pid::sat,      "Saturation", 0.0f, 100.0f, 0.0f,  fmtPct, 1.0f);
    F (pid::dist,     "Distortion", 0.0f, 100.0f, 0.0f,  fmtPct, 1.0f);
    F (pid::distTone, "Dist Tone",  0.0f, 100.0f, 55.0f, fmtPct, 1.0f);

    // ott
    B (pid::ottOn, "OTT On", true);
    F (pid::hyper, "Hyper", 0.0f, 100.0f, 0.0f, fmtPct, 1.0f);
    F (pid::ott,   "OTT",   0.0f, 100.0f, 0.0f, fmtPct, 1.0f);

    // delay
    B (pid::dlyOn, "Delay On", true);
    F (pid::dlyMix, "Delay Mix", 0.0f, 100.0f, 0.0f, fmtPct, 1.0f);
    C (pid::dlyTime, "Delay Time", delayTimeNames(), 2);
    B (pid::dlyDotted, "Delay Dotted", false);
    F (pid::dlyFb,  "Delay Feedback", 0.0f, 95.0f, 35.0f, fmtPct, 1.0f);
    F (pid::dlyCut, "Delay Cutoff",   0.0f, 100.0f, 70.0f, fmtPct, 1.0f);
    C (pid::dlyMode, "Delay Mode", delayModeNames(), 0);

    // reverb
    B (pid::revOn, "Reverb On", true);
    F (pid::revMix,   "Reverb Mix",   0.0f, 100.0f, 0.0f,  fmtPct, 1.0f);
    F (pid::revDecay, "Reverb Decay", 0.0f, 100.0f, 40.0f, fmtPct, 1.0f);
    F (pid::revPre,   "Reverb Pre-Delay", 0.0f, 200.0f, 20.0f, fmtMs, 1.0f);
    F (pid::revTone,  "Reverb Tone",  0.0f, 100.0f, 60.0f, fmtPct, 1.0f);

    // width
    B (pid::widthOn, "Width On", true);
    F (pid::width, "Width", 0.0f, 200.0f, 100.0f, fmtPct, 1.0f);

    return { p.begin(), p.end() };
}
} // namespace cr3a
