#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace cr3a;

// ============================================================================
static juce::File settingsFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("CR3A").getChildFile ("settings.txt");
}

CR3AProcessor::CR3AProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CR3A_STATE", createParameterLayout())
{
    pBypass = rawParam (pid::bypass);       pLimiter = rawParam (pid::limiter);   pMono = rawParam (pid::mono);
    pFormantOn = rawParam (pid::formantOn); pDblOn = rawParam (pid::dblOn);       pDblType = rawParam (pid::dblType);
    pEqOn = rawParam (pid::eqOn);           pCompOn = rawParam (pid::compOn);     pSatOn = rawParam (pid::satOn);
    pOttOn = rawParam (pid::ottOn);         pDlyOn = rawParam (pid::dlyOn);       pDlyTime = rawParam (pid::dlyTime);
    pDlyDotted = rawParam (pid::dlyDotted); pDlyMode = rawParam (pid::dlyMode);   pRevOn = rawParam (pid::revOn);
    pWidthOn = rawParam (pid::widthOn);

    // remember the last used theme for new instances
    auto f = settingsFile();
    if (f.existsAsFile())
        themeIndex = juce::jlimit (0, 99, f.loadFileAsString().trim().getIntValue());
}

bool CR3AProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CR3AProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    currentSampleRate = sampleRate;

    formant.prepare (sr);
    eq.prepare (sr);
    comp.prepare (sr);
    saturator.prepare (sr, kChunk);
    ott.prepare (sr);
    doubler.prepare (sr);
    widthFx.prepare (sr);
    delayFx.prepare (sr);
    reverbFx.prepare (sr);
    limiterFx.prepare (sr);

    latency = formant.getLatency() + saturator.getLatency() + limiterFx.getLatency();
    for (auto& d : dryLine) d.prepare (latency + 8);
    setLatencySamples (latency);

    sInGain.init (rawParam (pid::inGain), sr);       sOutGain.init (rawParam (pid::outGain), sr);
    sMix.init (rawParam (pid::mix), sr);             sFormant.init (rawParam (pid::formant), sr, 0.05);
    sDblMix.init (rawParam (pid::dblMix), sr);       sDblWidth.init (rawParam (pid::dblWidth), sr);
    sDblPan.init (rawParam (pid::dblPan), sr);       sDblDetune.init (rawParam (pid::dblDetune), sr, 0.08);
    sDblTime.init (rawParam (pid::dblTime), sr, 0.08);
    sEqLowEnd.init (rawParam (pid::eqLowEnd), sr);   sEqLow.init (rawParam (pid::eqLow), sr);
    sEqMid.init (rawParam (pid::eqMid), sr);         sEqMidHigh.init (rawParam (pid::eqMidHigh), sr);
    sEqMidEnd.init (rawParam (pid::eqMidEnd), sr);   sEqAir.init (rawParam (pid::eqAir), sr);
    sEqHarsh.init (rawParam (pid::eqHarsh), sr);
    sCompAmt.init (rawParam (pid::compAmt), sr);     sCompGain.init (rawParam (pid::compGain), sr);
    sSat.init (rawParam (pid::sat), sr);             sDist.init (rawParam (pid::dist), sr);
    sDistTone.init (rawParam (pid::distTone), sr);   sHyper.init (rawParam (pid::hyper), sr);
    sOtt.init (rawParam (pid::ott), sr);
    sDlyMix.init (rawParam (pid::dlyMix), sr);       sDlyFb.init (rawParam (pid::dlyFb), sr);
    sDlyCut.init (rawParam (pid::dlyCut), sr);
    sRevMix.init (rawParam (pid::revMix), sr);       sRevDecay.init (rawParam (pid::revDecay), sr, 0.08);
    sRevPre.init (rawParam (pid::revPre), sr, 0.08); sRevTone.init (rawParam (pid::revTone), sr);
    sWidth.init (rawParam (pid::width), sr);

    prevIn = juce::Decibels::decibelsToGain (sInGain.raw->load());
    prevOut = juce::Decibels::decibelsToGain (sOutGain.raw->load());
    prevWet = (pBypass->load() > 0.5f) ? 0.0f : sMix.raw->load() * 0.01f;
    prevMono = pMono->load() > 0.5f ? 1.0f : 0.0f;
    prevDly = prevRev = 0.0f;
}

static inline void rampGain (float* l, float* r, int n, float g0, float g1) noexcept
{
    if (g0 == g1)
    {
        for (int i = 0; i < n; ++i) { l[i] *= g1; r[i] *= g1; }
        return;
    }
    const float step = (g1 - g0) / (float) n;
    float g = g0;
    for (int i = 0; i < n; ++i) { g += step; l[i] *= g; r[i] *= g; }
}

void CR3AProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, numSamples);

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0) bpm = *b;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    for (int pos = 0; pos < numSamples; pos += kChunk)
        processChunk (L + pos, R + pos, std::min (kChunk, numSamples - pos), bpm);
}

void CR3AProcessor::processChunk (float* L, float* R, int n, double bpm)
{
    // ---------------------------------------------------------------- meters in
    float inPk = 0.0f;
    for (int i = 0; i < n; ++i) inPk = std::max (inPk, std::max (std::abs (L[i]), std::abs (R[i])));
    if (inPk > inLevel.load()) inLevel.store (inPk);            // the GUI resets it when it has read it

    // ---------------------------------------------------------------- dry path (latency compensated)
    for (int i = 0; i < n; ++i)
    {
        dryL[(size_t) i] = dryLine[0].readInt (latency);
        dryR[(size_t) i] = dryLine[1].readInt (latency);
        dryLine[0].push (L[i]);
        dryLine[1].push (R[i]);
    }

    // ---------------------------------------------------------------- input gain
    const float inG = dbToLin (sInGain.next (n));
    rampGain (L, R, n, prevIn, inG);
    prevIn = inG;

    // ---------------------------------------------------------------- formant
    const bool fOn = pFormantOn->load() > 0.5f;
    formant.setShiftSemitones (sFormant.next (n, 0.0f, fOn));
    formant.process (0, L, n);
    formant.process (1, R, n);

    // ---------------------------------------------------------------- EQ
    {
        const bool on = pEqOn->load() > 0.5f;
        VocalEQ::Params p;
        p.lowEnd  = sEqLowEnd.next (n, 0.0f, on);
        p.low     = sEqLow.next (n, 0.0f, on);
        p.mid     = sEqMid.next (n, 0.0f, on);
        p.midHigh = sEqMidHigh.next (n, 0.0f, on);
        p.midEnd  = sEqMidEnd.next (n, 0.0f, on);
        p.air     = sEqAir.next (n, 0.0f, on);
        p.harsh   = sEqHarsh.next (n, 0.0f, on) * 0.01f;
        eq.process (L, R, n, p);
    }

    // ---------------------------------------------------------------- compressor
    {
        const bool on = pCompOn->load() > 0.5f;
        comp.process (L, R, n, sCompAmt.next (n, 0.0f, on) * 0.01f, sCompGain.next (n, 0.0f, on));
        if (comp.getGainReduction() > compGR.load()) compGR.store (comp.getGainReduction());
    }

    // ---------------------------------------------------------------- saturation / distortion / hyper
    {
        const bool sOn = pSatOn->load() > 0.5f, oOn = pOttOn->load() > 0.5f;
        Saturator::Params p;
        p.sat   = sSat.next (n, 0.0f, sOn) * 0.01f;
        p.dist  = sDist.next (n, 0.0f, sOn) * 0.01f;
        p.tone  = sDistTone.next (n) * 0.01f;
        p.hyper = sHyper.next (n, 0.0f, oOn) * 0.01f;
        saturator.process (L, R, n, p);
    }

    // ---------------------------------------------------------------- OTT
    ott.process (L, R, n, sOtt.next (n, 0.0f, pOttOn->load() > 0.5f) * 0.01f);

    // ---------------------------------------------------------------- doubler
    {
        const bool on = pDblOn->load() > 0.5f;
        Doubler::Params p;
        p.mix = sDblMix.next (n, 0.0f, on) * 0.01f;
        p.width = sDblWidth.next (n) * 0.01f;
        p.panAuto = sDblPan.next (n) * 0.01f;
        p.detuneCents = sDblDetune.next (n);
        p.timeMs = sDblTime.next (n);
        p.type = (int) (pDblType->load() + 0.5f);
        doubler.process (L, R, n, p);
    }

    // ---------------------------------------------------------------- width
    widthFx.process (L, R, n, sWidth.next (n, 100.0f, pWidthOn->load() > 0.5f) * 0.01f);

    // ---------------------------------------------------------------- sends: delay + reverb
    {
        static const float beats[6] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        const int ti = juce::jlimit (0, 5, (int) (pDlyTime->load() + 0.5f));
        float b = beats[ti] * (pDlyDotted->load() > 0.5f ? 1.5f : 1.0f);
        const float secs = juce::jlimit (0.005f, 9.5f, b * 60.0f / (float) bpm);

        StereoDelay::Params dp;
        dp.timeSamples = secs * (float) sr;
        dp.feedback = sDlyFb.next (n) * 0.01f;
        dp.cutoff = sDlyCut.next (n) * 0.01f;
        dp.mode = juce::jlimit (0, 2, (int) (pDlyMode->load() + 0.5f));
        delayFx.process (L, R, wdL.data(), wdR.data(), n, dp);

        Reverb::Params rp;
        rp.decay = sRevDecay.next (n) * 0.01f;
        rp.tone = sRevTone.next (n) * 0.01f;
        rp.preMs = sRevPre.next (n);
        reverbFx.process (L, R, wrL.data(), wrR.data(), n, rp);

        const float dG = sDlyMix.next (n, 0.0f, pDlyOn->load() > 0.5f) * 0.01f;
        const float rG = sRevMix.next (n, 0.0f, pRevOn->load() > 0.5f) * 0.01f * 1.5f;
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) (i + 1) / (float) n;
            const float gd = prevDly + (dG - prevDly) * t;
            const float gr = prevRev + (rG - prevRev) * t;
            L[i] += wdL[(size_t) i] * gd + wrL[(size_t) i] * gr;
            R[i] += wdR[(size_t) i] * gd + wrR[(size_t) i] * gr;
        }
        prevDly = dG; prevRev = rG;
    }

    // ---------------------------------------------------------------- dry / wet + bypass
    {
        const bool byp = pBypass->load() > 0.5f;
        const float wet = sMix.next (n, 0.0f, ! byp) * 0.01f;
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) (i + 1) / (float) n;
            const float g = prevWet + (wet - prevWet) * t;
            L[i] = dryL[(size_t) i] + (L[i] - dryL[(size_t) i]) * g;
            R[i] = dryR[(size_t) i] + (R[i] - dryR[(size_t) i]) * g;
        }
        prevWet = wet;
    }

    // ---------------------------------------------------------------- limiter + output gain + mono
    limiterFx.process (L, R, n, pLimiter->load() > 0.5f);
    if (limiterFx.getGainReduction() > limGR.load()) limGR.store (limiterFx.getGainReduction());

    const float outG = dbToLin (sOutGain.next (n));
    rampGain (L, R, n, prevOut, outG);
    prevOut = outG;

    const float monoTarget = pMono->load() > 0.5f ? 1.0f : 0.0f;
    if (monoTarget > 0.0f || prevMono > 0.0f)
    {
        const float newMono = prevMono + (monoTarget - prevMono) * std::min (1.0f, (float) n / (0.01f * (float) sr));
        for (int i = 0; i < n; ++i)
        {
            const float a = prevMono + (newMono - prevMono) * ((float) (i + 1) / (float) n);
            const float m = 0.5f * (L[i] + R[i]);
            L[i] += (m - L[i]) * a;
            R[i] += (m - R[i]) * a;
        }
        prevMono = newMono;
    }

    // ---------------------------------------------------------------- meters out / scope
    float outPk = 0.0f;
    int sp = scopePos.load();
    for (int i = 0; i < n; ++i)
    {
        outPk = std::max (outPk, std::max (std::abs (L[i]), std::abs (R[i])));
        scope[(size_t) sp] = 0.5f * (L[i] + R[i]);
        sp = (sp + 1) % scopeSize;
    }
    scopePos.store (sp);
    if (outPk > outLevel.load()) outLevel.store (outPk);
}

// ============================================================================
juce::AudioProcessorEditor* CR3AProcessor::createEditor() { return new CR3AEditor (*this); }

void CR3AProcessor::setCurrentProgram (int index)
{
    const auto& list = factoryPresets();
    if (index < 0 || index >= (int) list.size()) return;
    currentProgram = index;
    applyPreset (apvts, list[(size_t) index]);
    setPresetName (list[(size_t) index].name);
}

const juce::String CR3AProcessor::getProgramName (int index)
{
    const auto& list = factoryPresets();
    return (index >= 0 && index < (int) list.size()) ? juce::String (list[(size_t) index].name) : juce::String();
}

void CR3AProcessor::setThemeIndex (int i)
{
    themeIndex = i;
    auto f = settingsFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText (juce::String (i));
}

juce::String CR3AProcessor::getPresetName() const
{
    const juce::ScopedLock sl (presetLock);
    return presetName;
}

void CR3AProcessor::setPresetName (const juce::String& n)
{
    const juce::ScopedLock sl (presetLock);
    presetName = n;
}

juce::File CR3AProcessor::getUserPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("CR3A").getChildFile ("Presets");
}

bool CR3AProcessor::saveUserPreset (const juce::File& file)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        xml->setAttribute ("presetName", file.getFileNameWithoutExtension());
        file.getParentDirectory().createDirectory();
        if (xml->writeTo (file))
        {
            setPresetName (file.getFileNameWithoutExtension());
            return true;
        }
    }
    return false;
}

bool CR3AProcessor::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            // keep the global stage (in/out/mix/bypass) as it is
            for (auto id : { pid::inGain, pid::outGain, pid::mix, pid::bypass, pid::limiter, pid::mono })
            {
                auto cur = apvts.getParameter (id);
                for (auto child : vt)
                    if (child.getProperty ("id").toString() == id)
                        child.setProperty ("value", cur->convertFrom0to1 (cur->getValue()), nullptr);
            }
            apvts.replaceState (vt);
            setPresetName (file.getFileNameWithoutExtension());
            return true;
        }
    return false;
}

void CR3AProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("theme", themeIndex.load(), nullptr);
    state.setProperty ("preset", getPresetName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void CR3AProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            if (vt.hasProperty ("theme"))  themeIndex = (int) vt.getProperty ("theme");
            if (vt.hasProperty ("preset")) setPresetName (vt.getProperty ("preset").toString());
            apvts.replaceState (vt);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CR3AProcessor(); }
