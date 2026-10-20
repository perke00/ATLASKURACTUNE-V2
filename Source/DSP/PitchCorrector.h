#pragma once
#include <cmath>
#include <algorithm>
#include "Scales.h"

namespace atlas
{
struct CorrParams
{
    float retuneMs = 15.0f;   // 0 = hard / robotic snap
    float humanize = 0.2f;    // 0..1
    float amount   = 1.0f;    // 0..1
    int   mode     = 0;       // 0 classic, 1 modern, 2 auto
    NoteMask mask {};
};

// Pure pitch-correction logic (no JUCE) so it can be unit tested offline.
class PitchCorrector
{
public:
    struct Result
    {
        bool  voiced = false;
        bool  holding = false;    // briefly lost pitch: keep previous grain settings
        float freq = 0.0f;        // smoothed detected frequency (Hz)
        float detMidi = 0.0f;     // detected pitch (fractional MIDI)
        int   target = 0;         // note the pitch is snapped to
        float outMidi = 0.0f;     // corrected pitch (fractional MIDI)
    };

    void reset()
    {
        medCount = medPos = 0; corr = 0.0f; heldTime = 0.0f; prevTarget = -1; holdFrames = 0;
    }

    float correction() const { return corr; }

    // call once per analysis hop. 'found' / 'hz' come from the pitch detector.
    Result update (bool found, float hz, const CorrParams& p, float dt)
    {
        Result r;
        if (found)
        {
            const bool hard = p.retuneMs < 0.5f && p.mode != 1;

            // jump detection: a big, sudden pitch change must not be dragged through the median filter
            if (medCount > 0 && std::fabs (12.0f * std::log2 (hz / lastHz)) > 1.2f)
                medCount = 0;
            lastHz = hz;

            med[medPos] = hz; medPos = (medPos + 1) % 3; medCount = std::min (3, medCount + 1);
            float f = hz;
            if (medCount == 3 && ! hard)
            {
                const float a = med[0], b = med[1], c = med[2];
                f = std::max (std::min (a, b), std::min (std::max (a, b), c));
            }

            const float m = 69.0f + 12.0f * std::log2 (f / 440.0f);
            const int tgt = nearestNote (m, p.mask, prevTarget, hard ? 0.10f : 0.18f);
            if (tgt != prevTarget) { prevTarget = tgt; heldTime = 0.0f; } else heldTime += dt;

            const float dev = (float) tgt - m;
            float speed = p.retuneMs;
            switch (p.mode)
            {
                case 1:  speed = speed * 2.0f + 12.0f; break;
                case 2:  speed *= (std::fabs (dev) > 0.5f ? 0.5f : 1.6f); break;
                default: break;
            }
            speed *= 1.0f + p.humanize * 3.0f * std::min (heldTime, 1.0f);

            const float alpha = speed <= 0.5f ? 1.0f : 1.0f - std::exp (-1000.0f * dt / speed);
            corr += alpha * (dev * p.amount - corr);

            holdFrames = (int) (0.06f / dt) + 1;
            r.voiced = true; r.freq = f; r.detMidi = m; r.target = tgt; r.outMidi = m + corr;
        }
        else if (--holdFrames <= 0)
        {
            corr *= 0.9f; prevTarget = -1; medCount = 0;
            r.voiced = false;
        }
        else
            r.holding = true;
        return r;
    }

private:
    float med[3] { 0, 0, 0 }, lastHz = 0.0f;
    int medCount = 0, medPos = 0, prevTarget = -1, holdFrames = 0;
    float corr = 0.0f, heldTime = 0.0f;
};
} // namespace atlas
