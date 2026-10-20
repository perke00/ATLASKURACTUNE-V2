#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace atlas
{
// Pitch-synchronous overlap-add (PSOLA style) shifter.
// The pitch is set directly through the synthesis period (Tsyn) so the output pitch is exact,
// and a separate formant factor re-reads each grain at a different speed (formant shift).
class PsolaShifter
{
public:
    static constexpr int N = 1 << 15;
    static constexpr int MASK = N - 1;

    void prepare()
    {
        in.assign (N, 0.0f);
        acc.assign (N, 0.0f);
        configure (600);
    }

    // maxPeriod = longest period (samples) that will be passed to process()
    void configure (int maxPeriodSamples)
    {
        maxT = (float) maxPeriodSamples;
        D = (int) std::ceil (3.3f * maxT) + 8;
        std::fill (in.begin(), in.end(), 0.0f);
        std::fill (acc.begin(), acc.end(), 0.0f);
        n = 0; nextMark = 0.0; resync = true; vmix = 0.0f; dry = 0.0f; epochs.clear();
    }

    int latency() const { return D; }
    float dryDelayed() const { return dry; }

    float process (float x, float T0, float Tsyn, float formant, bool dryWhenUnvoiced, bool active)
    {
        in[(size_t) (n & MASK)] = x;
        const int64_t idx = n - D;
        float out = 0.0f;

        if (idx >= 0)
        {
            dry = in[(size_t) (idx & MASK)];

            const bool voiced = active && T0 > 8.0f;
            if (voiced)
            {
                T0 = std::min (T0, maxT);
                Tsyn = std::max (Tsyn, 16.0f);
                formant = std::min (1.4f, std::max (0.7f, formant));

                if (resync || nextMark < (double) idx - T0)
                {
                    nextMark = (double) idx + T0;
                    epochs.clear();
                    resync = false;
                }
                while (nextMark - T0 <= (double) idx)
                {
                    placeGrain (nextMark, T0, Tsyn, formant);
                    nextMark += Tsyn;
                }
            }
            else
                resync = true;

            const float target = voiced ? 1.0f : 0.0f;
            vmix += 0.004f * (target - vmix);
            const float a = acc[(size_t) (idx & MASK)];
            out = a * vmix + (dryWhenUnvoiced ? dry * (1.0f - vmix) : 0.0f);
            acc[(size_t) (idx & MASK)] = 0.0f;
        }
        ++n;
        return out;
    }

private:
    float rd (int64_t i) const { return in[(size_t) (i & MASK)]; }

    // Epoch (pitch-mark) tracker: epochs are spaced ~T0 apart and refined by waveform correlation
    void extendEpochs (int64_t limit, double s, float T0)
    {
        if (epochs.empty())
        {
            const int64_t c = (int64_t) std::floor (s);
            const int r = (int) (T0 * 0.5f);
            float best = -1e9f; int64_t bi = c;
            for (int k = -r; k <= r; ++k) { const float v = rd (c + k); if (v > best) { best = v; bi = c + k; } }
            epochs.push_back (bi);
        }
        const int L = std::max (4, (int) (T0 * 0.25f));
        const int R = std::max (1, (int) (T0 * 0.1f));
        while (epochs.back() < limit)
        {
            const int64_t last = epochs.back();
            const int64_t guess = last + (int64_t) std::lround (T0);
            double bestScore = -1e30; int bestD = 0;
            for (int d = -R; d <= R; ++d)
            {
                double dot = 0.0, e2 = 1e-9;
                for (int k = -L; k <= L; ++k)
                {
                    const float a = rd (last + k), b = rd (guess + d + k);
                    dot += (double) a * b; e2 += (double) b * b;
                }
                const double sc = dot / std::sqrt (e2);
                if (sc > bestScore) { bestScore = sc; bestD = d; }
            }
            epochs.push_back (guess + bestD);
            if (epochs.size() > 24) epochs.erase (epochs.begin(), epochs.begin() + 8);
        }
    }

    void placeGrain (double s, float T0, float Tsyn, float fmt)
    {
        extendEpochs ((int64_t) std::ceil (s + T0 * 0.6), s, T0);
        int64_t bi = epochs.front();
        double bd = 1e30;
        for (auto e : epochs) { const double d = std::fabs ((double) e - s); if (d < bd) { bd = d; bi = e; } }

        const int64_t sc = (int64_t) std::floor (s);
        // grain half-width: one period normally, widened when pitch goes down so the grains still overlap
        const float Wd = std::min (std::max (T0, Tsyn), 1.6f * T0);
        const int half = (int) std::ceil (Wd);
        float gain = Tsyn / Wd;                 // keeps overlap-add level ~unity
        // empirical level compensation for large shifts (used by the harmony voices)
        const float ratio = Tsyn / T0;
        if (ratio < 1.0f)      gain *= std::pow (1.0f / ratio, 0.8f);
        else if (ratio < 1.6f) gain *= std::pow (ratio, 0.55f);
        const float pi = 3.14159265358979f;

        for (int64_t i = sc - half; i <= sc + half + 1; ++i)
        {
            const float j = (float) ((double) i - s);
            if (std::fabs (j) >= Wd) continue;
            const float w = 0.5f * (1.0f + std::cos (pi * j / Wd));
            const double pos = (double) bi + (double) j * fmt;
            const int64_t p0 = (int64_t) std::floor (pos);
            const float fr = (float) (pos - (double) p0);
            const float a = in[(size_t) (p0 & MASK)];
            const float b = in[(size_t) ((p0 + 1) & MASK)];
            acc[(size_t) (i & MASK)] += w * gain * (a + fr * (b - a));
        }
    }

    std::vector<float> in, acc;
    std::vector<int64_t> epochs;
    int64_t n = 0;
    double nextMark = 0.0;
    bool resync = true;
    float maxT = 600.0f, vmix = 0.0f, dry = 0.0f;
    int D = 1600;
};
} // namespace atlas
