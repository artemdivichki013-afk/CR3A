#pragma once
#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "Presets.h"
#include "DSP/Compressor.h"
#include "DSP/Doubler.h"
#include "DSP/Formant.h"
#include "DSP/Limiter.h"
#include "DSP/OTT.h"
#include "DSP/Reverb.h"
#include "DSP/Saturator.h"
#include "DSP/StereoDelay.h"
#include "DSP/VocalEQ.h"
#include "DSP/Width.h"

class CR3AProcessor : public juce::AudioProcessor
{
public:
    CR3AProcessor();
    ~CR3AProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CR3A"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return (int) cr3a::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ------------------------------------------------------------------ UI related state
    int  getThemeIndex() const noexcept { return themeIndex.load(); }
    void setThemeIndex (int i);
    juce::String getPresetName() const;
    void setPresetName (const juce::String& n);
    static juce::File getUserPresetDirectory();
    bool saveUserPreset (const juce::File& file);
    bool loadUserPreset (const juce::File& file);

    juce::AudioProcessorValueTreeState apvts;

    // meters (written on the audio thread, read by the GUI)
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f }, compGR { 0.0f }, limGR { 0.0f };
    static constexpr int scopeSize = 4096;
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopePos { 0 };
    std::atomic<double> currentSampleRate { 48000.0 };

private:
    static constexpr int kChunk = 32;

    struct Sm
    {
        std::atomic<float>* raw = nullptr;
        juce::SmoothedValue<float> sv;
        void init (std::atomic<float>* r, double sr, double seconds = 0.03)
        {
            raw = r;
            sv.reset (sr, seconds);
            sv.setCurrentAndTargetValue (raw->load());
        }
        float next (int n, float neutral = 0.0f, bool on = true)
        {
            sv.setTargetValue (on ? raw->load() : neutral);
            return sv.skip (n);
        }
    };

    void processChunk (float* L, float* R, int n, double bpm);
    std::atomic<float>* rawParam (const char* id) { return apvts.getRawParameterValue (id); }

    // smoothed continuous parameters
    Sm sInGain, sOutGain, sMix, sFormant, sDblMix, sDblWidth, sDblPan, sDblDetune, sDblTime,
       sEqLowEnd, sEqLow, sEqMid, sEqMidHigh, sEqMidEnd, sEqAir, sEqHarsh, sCompAmt, sCompGain,
       sSat, sDist, sDistTone, sHyper, sOtt, sDlyMix, sDlyFb, sDlyCut, sRevMix, sRevDecay,
       sRevPre, sRevTone, sWidth;

    // discrete parameters
    std::atomic<float> *pBypass, *pLimiter, *pMono, *pFormantOn, *pDblOn, *pDblType, *pEqOn, *pCompOn,
                       *pSatOn, *pOttOn, *pDlyOn, *pDlyTime, *pDlyDotted, *pDlyMode, *pRevOn, *pWidthOn;

    // DSP
    cr3a::FormantShifter formant;
    cr3a::VocalEQ eq;
    cr3a::Compressor comp;
    cr3a::Saturator saturator;
    cr3a::OTT ott;
    cr3a::Doubler doubler;
    cr3a::Width widthFx;
    cr3a::StereoDelay delayFx;
    cr3a::Reverb reverbFx;
    cr3a::Limiter limiterFx;
    cr3a::DelayLine dryLine[2];

    double sr = 48000.0;
    int latency = 0;
    float prevIn = 1.0f, prevOut = 1.0f, prevWet = 1.0f, prevMono = 0.0f, prevDly = 0.0f, prevRev = 0.0f;

    std::array<float, kChunk> dryL {}, dryR {}, wdL {}, wdR {}, wrL {}, wrR {};

    std::atomic<int> themeIndex { 0 };
    int currentProgram = 0;
    juce::String presetName { "Init" };
    mutable juce::CriticalSection presetLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CR3AProcessor)
};
