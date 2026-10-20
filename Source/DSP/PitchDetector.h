#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace atlas
{
// YIN pitch detector running on a decimated copy of the signal (~11 kHz) so it is cheap enough to run every few ms.
class PitchDetector
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        dec = std::max (1, (int) std::lround (sampleRate / 11025.0));
        drate = sr / dec;
        buf.assign (8192, 0.0f);
        diff.reserve (1024); cmnd.reserve (1024);
        mask = 8191;
        reset();
    }

    void configure (float minHz, float maxHz)
    {
        tauMin = std::max (2, (int) std::floor (drate / maxHz));
        tauMax = (int) std::ceil (drate / minHz);
        W = tauMax;
        diff.assign ((size_t) tauMax + 2, 0.0f);
        cmnd.assign ((size_t) tauMax + 2, 1.0f);
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        wp = 0; acc = 0.0f; cnt = 0;
    }

    inline void push (float x)
    {
        acc += x;
        if (++cnt == dec)
        {
            buf[(size_t) wp] = acc / (float) dec;
            wp = (wp + 1) & mask;
            acc = 0.0f; cnt = 0;
        }
    }

    // returns true if a voiced pitch was found
    bool detect (float& hz, float& confidence, float gateLinear = 0.003f)
    {
        const int total = W + tauMax + 1;
        const int start = wp - total;
        auto x = [&] (int i) { return buf[(size_t) ((start + i) & mask)]; };

        double energy = 0.0;
        for (int j = 0; j < W; ++j) { const float v = x (tauMax + j); energy += (double) v * v; }
        if (std::sqrt (energy / W) < gateLinear) return false;

        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float s = 0.0f;
            for (int j = 0; j < W; ++j)
            {
                const float d = x (j) - x (j + tau);
                s += d * d;
            }
            diff[(size_t) tau] = s;
        }

        cmnd[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            running += diff[(size_t) tau];
            cmnd[(size_t) tau] = running > 1e-9f ? diff[(size_t) tau] * (float) tau / running : 1.0f;
        }

        int best = -1;
        const float thr = 0.15f;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < thr)
            {
                while (tau + 1 < tauMax && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0)
        {
            float mn = 1e9f;
            for (int tau = tauMin; tau < tauMax; ++tau)
                if (cmnd[(size_t) tau] < mn) { mn = cmnd[(size_t) tau]; best = tau; }
            if (best < 0 || mn > 0.35f) return false;
        }

        float tauF = (float) best;
        if (best > 1 && best < tauMax)
        {
            const float a = cmnd[(size_t) best - 1], b = cmnd[(size_t) best], c = cmnd[(size_t) best + 1];
            const float den = a - 2.0f * b + c;
            if (std::abs (den) > 1e-9f) tauF += 0.5f * (a - c) / den;
        }

        hz = (float) (drate / tauF);
        confidence = 1.0f - cmnd[(size_t) best];
        return hz > 20.0f;
    }

private:
    double sr = 44100.0, drate = 11025.0;
    int dec = 4, tauMin = 10, tauMax = 100, W = 100;
    std::vector<float> buf, diff, cmnd;
    int mask = 8191, wp = 0, cnt = 0;
    float acc = 0.0f;
};
} // namespace atlas
