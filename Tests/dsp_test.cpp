// Dev tool: offline checks of the DSP chain through the real processor.
#include <juce_audio_processors/juce_audio_processors.h>
#include <chrono>
#include <cstdio>
#include "PluginProcessor.h"

using namespace cr3a;
static const double SR = 48000.0;

static void setP (CR3AProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static void resetAll (CR3AProcessor& p)
{
    for (auto* prm : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            r->setValueNotifyingHost (r->getDefaultValue());
}

struct Stereo { std::vector<float> l, r; };

static Stereo render (CR3AProcessor& p, const Stereo& in, int block = 512)
{
    Stereo out = in;
    juce::MidiBuffer midi;
    for (size_t pos = 0; pos < in.l.size(); pos += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, in.l.size() - pos);
        juce::AudioBuffer<float> b (2, n);
        b.copyFrom (0, 0, in.l.data() + pos, n);
        b.copyFrom (1, 0, in.r.data() + pos, n);
        p.processBlock (b, midi);
        std::memcpy (out.l.data() + pos, b.getReadPointer (0), sizeof (float) * (size_t) n);
        std::memcpy (out.r.data() + pos, b.getReadPointer (1), sizeof (float) * (size_t) n);
    }
    return out;
}

static double rms (const std::vector<float>& v, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i];
    return std::sqrt (s / (double) (b - a));
}
static double db (double x) { return 20.0 * std::log10 (std::max (x, 1e-12)); }

// vowel-like signal: glottal pulse train through two resonators
static Stereo vowel (double seconds, double f0, double f1, double f2, double amp)
{
    const size_t n = (size_t) (seconds * SR);
    Stereo s { std::vector<float> (n), std::vector<float> (n) };
    BiquadCoef r1 = BiquadCoef::bandpass (SR, f1, f1 / 160.0), r2 = BiquadCoef::bandpass (SR, f2, f2 / 220.0);
    BiquadState s1, s2;
    double phase = 0;
    for (size_t i = 0; i < n; ++i)
    {
        phase += f0 / SR;
        float x = 0;
        if (phase >= 1.0) { phase -= 1.0; x = 1.0f; }
        // a slightly smoothed pulse
        static float z = 0; z += (x - z) * 0.35f;
        const float y = biquadTick (r1, s1, z) * 1.0f + biquadTick (r2, s2, z) * 0.7f;
        s.l[i] = s.r[i] = y;
    }
    const double g = amp / rms (s.l, n / 4, n);
    for (size_t i = 0; i < n; ++i) { s.l[i] *= (float) g; s.r[i] *= (float) g; }
    return s;
}

static std::vector<float> avgSpectrum (const std::vector<float>& x, size_t from, size_t to, int order = 13)
{
    const int N = 1 << order;
    FFT fft (order);
    std::vector<float> acc ((size_t) N / 2, 0.0f), buf ((size_t) (2 * N));
    int frames = 0;
    for (size_t pos = from; pos + (size_t) N <= to; pos += (size_t) N / 2)
    {
        for (int i = 0; i < N; ++i)
        {
            const float w = (float) (0.5 - 0.5 * std::cos (2.0 * kPi * i / N));
            buf[(size_t) (2 * i)] = x[pos + (size_t) i] * w; buf[(size_t) (2 * i + 1)] = 0;
        }
        fft.transform (buf.data(), false);
        for (int k = 0; k < N / 2; ++k) acc[(size_t) k] += std::sqrt (buf[(size_t) (2 * k)] * buf[(size_t) (2 * k)] + buf[(size_t) (2 * k + 1)] * buf[(size_t) (2 * k + 1)]);
        ++frames;
    }
    for (auto& v : acc) v /= (float) std::max (1, frames);
    return acc;
}

static double envelopePeak (const std::vector<float>& spec, double lo, double hi, double f0, int order = 13)
{
    const int N = 1 << order;
    const double bin = SR / N;
    // smooth over about 2*f0 so individual harmonics do not decide
    const int w = std::max (1, (int) (f0 * 1.2 / bin));
    double best = 0, bf = 0;
    for (int k = (int) (lo / bin); k < (int) (hi / bin); ++k)
    {
        double s = 0; int c = 0;
        for (int j = -w; j <= w; ++j) { const int kk = k + j; if (kk > 0 && kk < N / 2) { s += (double) spec[(size_t) kk] * spec[(size_t) kk]; ++c; } }
        s /= c;
        if (s > best) { best = s; bf = k * bin; }
    }
    return bf;
}

static int failures = 0;
static void check (bool ok, const char* what) { std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", what); if (! ok) ++failures; }

int main()
{
    CR3AProcessor proc;
    proc.prepareToPlay (SR, 512);
    const int lat = proc.getLatencySamples();
    std::printf ("Reported latency: %d samples (%.1f ms)\n", lat, 1000.0 * lat / SR);

    // ------------------------------------------------------------------ T1 transparency
    std::printf ("T1 default settings are transparent\n");
    {
        double worst = 0;
        for (double f : { 80.0, 200.0, 1000.0, 4000.0, 9000.0, 14000.0 })
        {
            resetAll (proc); proc.prepareToPlay (SR, 512);
            const size_t n = (size_t) (SR * 1.0);
            Stereo in { std::vector<float> (n), std::vector<float> (n) };
            for (size_t i = 0; i < n; ++i) in.l[i] = in.r[i] = 0.25f * (float) std::sin (2 * kPi * f * (double) i / SR);
            auto out = render (proc, in);
            const double g = db (rms (out.l, n / 2, n - 100)) - db (rms (in.l, n / 2, n - 100));
            worst = std::max (worst, std::abs (g));
            std::printf ("    %6.0f Hz: %+.3f dB\n", f, g);
        }
        check (worst < 0.35, "magnitude within +-0.35 dB");
    }

    // ------------------------------------------------------------------ T2 formant
    std::printf ("T2 formant shifter moves formants, keeps pitch\n");
    {
        resetAll (proc); proc.prepareToPlay (SR, 512);
        auto in = vowel (6.0, 140.0, 700.0, 1800.0, 0.1);
        const size_t n = in.l.size();
        auto specIn = avgSpectrum (in.l, (size_t) (SR * 1.0), n);
        const double f1In = envelopePeak (specIn, 350, 1100, 140);

        setP (proc, pid::formant, -4.0f);
        auto out = render (proc, in);
        auto specOut = avgSpectrum (out.l, (size_t) (SR * 1.5), n);
        const double f1Out = envelopePeak (specOut, 300, 1100, 140);
        std::printf ("    F1 in %.0f Hz -> out %.0f Hz (expected about %.0f)\n", f1In, f1Out, f1In * std::pow (2.0, -4.0 / 12.0));
        check (std::abs (f1Out / f1In - std::pow (2.0, -4.0 / 12.0)) < 0.12, "F1 moved by -4 semitones (+-12 %)");

        // pitch: strongest low harmonic spacing around 140 Hz
        const int N = 1 << 13; const double bin = SR / N;
        int pk = 0; float m = 0;
        for (int k = (int) (100 / bin); k < (int) (180 / bin); ++k) if (specOut[(size_t) k] > m) { m = specOut[(size_t) k]; pk = k; }
        std::printf ("    f0 peak at %.1f Hz\n", pk * bin);
        check (std::abs (pk * bin - 140.0) < 6.0, "pitch preserved");

        setP (proc, pid::formant, 4.0f);
        proc.prepareToPlay (SR, 512);
        out = render (proc, in);
        specOut = avgSpectrum (out.l, (size_t) (SR * 1.5), n);
        const double f1Up = envelopePeak (specOut, 450, 1400, 140);
        std::printf ("    +4 st: F1 %.0f Hz (expected about %.0f)\n", f1Up, f1In * std::pow (2.0, 4.0 / 12.0));
        check (std::abs (f1Up / f1In - std::pow (2.0, 4.0 / 12.0)) < 0.15, "F1 moved by +4 semitones (+-15 %)");

        // level preserved at 0
        setP (proc, pid::formant, 0.0f);
        proc.prepareToPlay (SR, 512);
        out = render (proc, in);
        const double d = db (rms (out.l, (size_t) (SR * 2), n - 3000)) - db (rms (in.l, (size_t) (SR * 2) - (size_t) lat, n - 3000 - (size_t) lat));
        std::printf ("    0 st level change %+.2f dB\n", d);
        check (std::abs (d) < 0.3, "level unchanged at 0 st");
    }

    // ------------------------------------------------------------------ T3 level behaviour of modules
    std::printf ("T3 loudness of modules on a vocal-like signal (-18 dBFS rms)\n");
    {
        auto in = vowel (5.0, 190.0, 650.0, 1700.0, 0.125);
        const size_t n = in.l.size();
        struct Case { const char* name; std::vector<std::pair<const char*, float>> p; double lo, hi; };
        std::vector<Case> cases = {
            { "sat 50%",       { { pid::sat, 50 } }, -3.0, 4.0 },
            { "sat 100%",      { { pid::sat, 100 } }, -3.0, 6.0 },
            { "dist 50%",      { { pid::dist, 50 } }, -4.0, 8.0 },
            { "dist 100%",     { { pid::dist, 100 } }, -4.0, 10.0 },
            { "hyper 70%",     { { pid::hyper, 70 } }, -2.0, 6.0 },
            { "ott 50%",       { { pid::ott, 50 } }, -4.0, 8.0 },
            { "ott 100%",      { { pid::ott, 100 } }, -4.0, 10.0 },
            { "comp 60%",      { { pid::compAmt, 60 } }, -3.0, 5.0 },
            { "comp 100%",     { { pid::compAmt, 100 } }, -3.0, 8.0 },
            { "eq harsh 100%", { { pid::eqHarsh, 100 } }, -3.0, 1.0 },
            { "doubler 50%",   { { pid::dblMix, 50 } }, -2.0, 6.0 },
            { "quad 100%",     { { pid::dblMix, 100 }, { pid::dblType, 1 } }, -2.0, 8.0 },
            { "reverb 30%",    { { pid::revMix, 30 } }, -2.0, 6.0 },
            { "delay 30%",     { { pid::dlyMix, 30 } }, -2.0, 6.0 },
            { "width 200%",    { { pid::width, 200 } }, -3.0, 3.0 },
        };
        for (auto& c : cases)
        {
            resetAll (proc); proc.prepareToPlay (SR, 512);
            for (auto& pv : c.p) setP (proc, pv.first, pv.second);
            auto out = render (proc, in);
            const double d = db (rms (out.l, n / 2, n - 4000)) - db (rms (in.l, n / 2, n - 4000));
            std::printf ("    %-14s %+6.2f dB\n", c.name, d);
            char buf[96]; std::snprintf (buf, sizeof buf, "%s level in [%.0f, %.0f] dB", c.name, c.lo, c.hi);
            check (d >= c.lo && d <= c.hi, buf);
        }
    }

    // ------------------------------------------------------------------ T4 delay timing
    std::printf ("T4 delay timing (1/8 @120 bpm = 250 ms)\n");
    {
        resetAll (proc); proc.prepareToPlay (SR, 512);
        setP (proc, pid::dlyMix, 100); setP (proc, pid::dlyFb, 0); setP (proc, pid::dlyCut, 100); setP (proc, pid::mix, 100);
        const size_t n = (size_t) (SR * 1.0);
        Stereo in { std::vector<float> (n, 0.0f), std::vector<float> (n, 0.0f) };
        in.l[1000] = in.r[1000] = 0.5f;
        auto out = render (proc, in);
        size_t p0 = 0, p1 = 0; float m0 = 0, m1 = 0;
        for (size_t i = 0; i < n; ++i)
        {
            const float a = std::abs (out.l[i]);
            if (i < 1000 + (size_t) lat + 4000) { if (a > m0) { m0 = a; p0 = i; } }
            else if (a > m1) { m1 = a; p1 = i; }
        }
        std::printf ("    dry peak at %zu (expected %d), echo at %zu, spacing %zu (expected 12000)\n", p0, 1000 + lat, p1, p1 - p0);
        check (p0 >= (size_t) (1000 + lat - 150) && p0 <= (size_t) (1000 + lat + 150), "dry path latency compensated (+-3 ms, formant spreads the pulse)");
        check (std::abs ((double) (p1 - p0) - 12000.0) < 60.0, "echo spacing = 250 ms");
    }

    // ------------------------------------------------------------------ T5 reverb
    std::printf ("T5 reverb tail\n");
    {
        for (float dec : { 20.0f, 50.0f, 80.0f })
        {
            resetAll (proc); proc.prepareToPlay (SR, 512);
            setP (proc, pid::revMix, 100); setP (proc, pid::revDecay, dec);
            const size_t n = (size_t) (SR * 14.0);
            Stereo in { std::vector<float> (n, 0.0f), std::vector<float> (n, 0.0f) };
            in.l[100] = in.r[100] = 0.5f;
            auto out = render (proc, in);
            // Schroeder backward integration of the tail (starting 250 ms after the dry impulse)
            const size_t start = (size_t) (0.25 * SR);
            std::vector<double> edc (n - start);
            double acc = 0;
            for (size_t i = n; i-- > start;) { acc += (double) out.l[i] * out.l[i]; edc[i - start] = acc; }
            auto atDb = [&] (double target) -> double
            {
                for (size_t i = 0; i < edc.size(); ++i) if (10 * std::log10 (edc[i] / edc[0] + 1e-30) < target) return (double) i / SR;
                return -1.0;
            };
            const double t5 = atDb (-5.0), t25 = atDb (-25.0);
            const double t60 = (t5 > 0 && t25 > 0) ? (t25 - t5) * 3.0 : -1.0;     // -5..-25 dB slope extrapolated to -60 dB
            const double expect = 0.4 * std::pow (25.0, dec / 100.0);
            std::printf ("    decay %.0f%%: RT60 about %.2f s (target %.2f s)\n", dec, t60, expect);
            check (t60 > 0 && t60 > 0.5 * expect && t60 < 1.6 * expect, "RT60 near target");
        }
    }

    // ------------------------------------------------------------------ T6 stress / stability
    std::printf ("T6 random stress, all parameters\n");
    {
        juce::Random rnd (1234);
        bool ok = true; float peak = 0;
        for (int trial = 0; trial < 6 && ok; ++trial)
        {
            resetAll (proc); proc.prepareToPlay (SR, 512);
            for (auto* prm : proc.getParameters())
                if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                {
                    const juce::String id = r->paramID;
                    if (id == pid::bypass || id == pid::mono) continue;
                    r->setValueNotifyingHost (rnd.nextFloat());
                }
            const size_t n = (size_t) (SR * 4);
            Stereo in { std::vector<float> (n), std::vector<float> (n) };
            for (size_t i = 0; i < n; ++i) { in.l[i] = (rnd.nextFloat() * 2 - 1) * 0.4f; in.r[i] = (rnd.nextFloat() * 2 - 1) * 0.4f; }
            auto out = render (proc, in, 37 + trial * 101);
            for (size_t i = 0; i < n; ++i)
            {
                if (! std::isfinite (out.l[i]) || ! std::isfinite (out.r[i])) { ok = false; break; }
                peak = std::max (peak, std::max (std::abs (out.l[i]), std::abs (out.r[i])));
            }
        }
        std::printf ("    max output peak %.2f\n", peak);
        check (ok, "no NaN / Inf");
        check (peak < 8.0f, "output bounded");
    }

    // ------------------------------------------------------------------ T7 limiter
    std::printf ("T7 limiter ceiling\n");
    {
        resetAll (proc); proc.prepareToPlay (SR, 512);
        setP (proc, pid::inGain, 18.0f);
        auto in = vowel (3.0, 190.0, 650.0, 1700.0, 0.5);
        auto out = render (proc, in);
        float pk = 0; for (size_t i = (size_t) SR; i < out.l.size(); ++i) pk = std::max (pk, std::abs (out.l[i]));
        std::printf ("    peak %.4f (ceiling 0.9659)\n", pk);
        check (pk <= 0.9700f, "limited to -0.3 dBFS");
    }

    // ------------------------------------------------------------------ T8 CPU
    std::printf ("T8 CPU\n");
    {
        resetAll (proc); proc.prepareToPlay (SR, 512);
        for (auto id : { pid::eqLow, pid::eqMid, pid::eqAir }) setP (proc, id, 3.0f);
        setP (proc, pid::formant, -3.0f); setP (proc, pid::eqHarsh, 50); setP (proc, pid::compAmt, 50); setP (proc, pid::sat, 40);
        setP (proc, pid::dist, 40); setP (proc, pid::hyper, 40); setP (proc, pid::ott, 40); setP (proc, pid::dblMix, 40); setP (proc, pid::dblType, 1);
        setP (proc, pid::dlyMix, 30); setP (proc, pid::revMix, 30); setP (proc, pid::width, 150);
        auto in = vowel (10.0, 190.0, 650.0, 1700.0, 0.125);
        const auto t0 = std::chrono::steady_clock::now();
        render (proc, in);
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("    10 s of stereo audio in %.2f s  ->  %.1f %% of one core\n", sec, sec / 10.0 * 100.0);
        check (sec / 10.0 < 0.25, "below 25 % of one core (unoptimised build is allowed to be slower)");
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL OK" : "SOME CHECKS FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
