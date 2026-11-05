#pragma once
#include "Utils.h"

namespace cr3a
{
/**
    Three-band upward + downward compressor ("OTT" style).
    Linkwitz-Riley (4th order) crossovers at 88 Hz and 2.5 kHz, stereo linked
    RMS detection, quiet material is lifted and loud material is pushed down
    inside every band. The knob is the depth: it scales the gain moves.
*/
class OTT
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const double f1 = 88.3, f2 = 2500.0;
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < 2; ++k)
            {
                lp1[c][k].c = BiquadCoef::lowpass (sr, f1);   hp1[c][k].c = BiquadCoef::highpass (sr, f1);
                lp2[c][k].c = BiquadCoef::lowpass (sr, f2);   hp2[c][k].c = BiquadCoef::highpass (sr, f2);
                apLp[c][k].c = lp2[c][k].c;                   apHp[c][k].c = hp2[c][k].c;
            }
        cMs = tauToCoef (0.006, sr);
        const double atk[3] = { 0.022, 0.014, 0.008 }, rel[3] = { 0.180, 0.130, 0.090 };
        for (int b = 0; b < 3; ++b) { atkC[b] = tauToCoef (atk[b], sr); relC[b] = tauToCoef (rel[b], sr); }
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < 2; ++k)
            {
                lp1[c][k].reset(); hp1[c][k].reset(); lp2[c][k].reset(); hp2[c][k].reset();
                apLp[c][k].reset(); apHp[c][k].reset();
            }
        for (int b = 0; b < 3; ++b) { ms[b] = 0.0f; gdb[b] = 0.0f; }
    }

    void process (float* l, float* r, int n, float depth) noexcept
    {
        if (depth < 0.0008f)
        {
            // keep the crossover states running so engaging is click free, no processing needed
            for (int b = 0; b < 3; ++b) gdb[b] *= 0.9f;
            return;
        }

        static const float thrD[3] = { -24.0f, -26.0f, -30.0f };
        static const float thrU[3] = { -46.0f, -48.0f, -52.0f };
        static const float mk[3]   = { 2.0f, 2.5f, 3.0f };
        const float kDown = 1.0f - 1.0f / 4.0f, kUp = 1.0f - 1.0f / 3.0f;

        for (int i = 0; i < n; ++i)
        {
            float bl[3], br[3];
            split (0, l[i], bl);
            split (1, r[i], br);
            float oL = 0.0f, oR = 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                const float pw = 0.5f * (bl[b] * bl[b] + br[b] * br[b]);
                ms[b] += (pw - ms[b]) * cMs;
                const float lvl = 4.342944819f * std::log (ms[b] + 1.0e-12f);          // 10 log10
                const float down = std::max (0.0f, lvl - thrD[b]) * kDown;
                float up = std::min (20.0f, std::max (0.0f, thrU[b] - lvl) * kUp);
                up *= clampf ((lvl + 78.0f) / 14.0f, 0.0f, 1.0f);                     // no noise lifting
                const float tgt = (up - down) * depth;
                gdb[b] += (tgt - gdb[b]) * (tgt < gdb[b] ? atkC[b] : relC[b]);
                const float g = dbToLin (gdb[b] + mk[b] * depth);
                oL += bl[b] * g;
                oR += br[b] * g;
            }
            l[i] = oL; r[i] = oR;
        }
    }

private:
    void split (int c, float x, float* out) noexcept
    {
        float lo = biquadTick (lp1[c][0].c, lp1[c][0].s[0], x);
        lo = biquadTick (lp1[c][1].c, lp1[c][1].s[0], lo);
        float rest = biquadTick (hp1[c][0].c, hp1[c][0].s[0], x);
        rest = biquadTick (hp1[c][1].c, hp1[c][1].s[0], rest);

        float mid = biquadTick (lp2[c][0].c, lp2[c][0].s[0], rest);
        mid = biquadTick (lp2[c][1].c, lp2[c][1].s[0], mid);
        float hi = biquadTick (hp2[c][0].c, hp2[c][0].s[0], rest);
        hi = biquadTick (hp2[c][1].c, hp2[c][1].s[0], hi);

        // all-pass the low band so that the three bands sum flat
        float a = biquadTick (apLp[c][0].c, apLp[c][0].s[0], lo);
        a = biquadTick (apLp[c][1].c, apLp[c][1].s[0], a);
        float b = biquadTick (apHp[c][0].c, apHp[c][0].s[0], lo);
        b = biquadTick (apHp[c][1].c, apHp[c][1].s[0], b);

        out[0] = a + b; out[1] = mid; out[2] = hi;
    }

    double sr = 48000.0;
    StereoBiquad lp1[2][2], hp1[2][2], lp2[2][2], hp2[2][2], apLp[2][2], apHp[2][2];
    float cMs = 0.01f, atkC[3] = {}, relC[3] = {}, ms[3] = {}, gdb[3] = {};
};
} // namespace cr3a
