#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace atlas
{
// One singer of the artificial choir. 'steps' are scale degrees relative to the sung note.
struct ChoirVoiceDef
{
    int   steps;      // scale steps above (+) / below (-) the sung note
    float cents;      // fixed tuning offset of this singer
    float pan;        // -1..1
    float formant;    // vocal-tract factor (>1 = brighter / younger female timbre)
    float delayMs;    // entrance timing offset
    float rate;       // vibrato rate in Hz
};

constexpr int kChoirVoices = 5;   // singers (a sixth voice is the drone)

inline const char* const* choirStyleNames()
{
    static const char* n[] = { "Women's Choir", "Balkan Seconds", "Open Fifths" };
    return n;
}
inline const char* choirStyleHint (int style)
{
    static const char* h[] = { "Soft parallel thirds and doubles", "Close seconds over a drone - village style", "Wide open fifths and octaves" };
    return h[std::max (0, std::min (2, style))];
}

inline const ChoirVoiceDef* choirStyle (int style)
{
    static const ChoirVoiceDef s0[kChoirVoices] = {
        { +2,  -4.0f, -0.55f, 1.10f, 14.0f, 5.3f },
        {  0,  +6.0f, +0.55f, 1.06f, 24.0f, 5.9f },
        { +4,  -2.0f, -0.85f, 1.16f, 31.0f, 5.0f },
        { -2,  +3.0f, +0.30f, 0.98f, 19.0f, 5.6f },
        { +7,  -5.0f, +0.85f, 1.20f, 38.0f, 6.1f } };
    static const ChoirVoiceDef s1[kChoirVoices] = {
        { +1,  -3.0f, -0.55f, 1.12f, 12.0f, 5.4f },
        { -1,  +4.0f, +0.55f, 1.04f, 26.0f, 5.8f },
        {  0,  +7.0f, -0.20f, 1.08f, 20.0f, 5.1f },
        { +1,  +6.0f, +0.85f, 1.18f, 34.0f, 6.0f },
        { -2,  -6.0f, -0.85f, 0.98f, 40.0f, 5.5f } };
    static const ChoirVoiceDef s2[kChoirVoices] = {
        { +4,  -3.0f, -0.55f, 1.14f, 15.0f, 5.2f },
        { -4,  +4.0f, +0.55f, 0.96f, 27.0f, 5.7f },
        { +7,  -4.0f, -0.85f, 1.22f, 33.0f, 5.9f },
        {  0,  +6.0f, +0.25f, 1.06f, 21.0f, 5.5f },
        { -7,  +3.0f, +0.85f, 0.90f, 39.0f, 5.0f } };
    switch (std::max (0, std::min (2, style))) { case 1: return s1; case 2: return s2; default: return s0; }
}

// Fractional delay line (linear interpolation) used for the singers' timing offsets
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        size = 1; while (size < maxSamples + 4) size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1; w = 0;
    }
    float process (float x, float delaySamples)
    {
        buf[(size_t) w] = x;
        float d = std::max (1.0f, std::min (delaySamples, (float) (size - 3)));
        const float rp = (float) w - d;
        int i0 = (int) std::floor (rp);
        const float fr = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        w = (w + 1) & mask;
        return a + fr * (b - a);
    }
private:
    std::vector<float> buf;
    int size = 1, mask = 0, w = 0;
};

// Natural singer imperfections: vibrato (own rate per singer) + slow random pitch drift, in cents
class SingerDrift
{
public:
    void seed (uint32_t s) { state = s ? s : 1u; phase = nextRand() * 6.2831853f; drift = 0.0f; depthMod = 0.8f + 0.4f * (nextRand() * 0.5f + 0.5f); }
    float update (float dt, float rateHz, float vibratoAmt, float driftAmt)
    {
        phase += 6.2831853f * rateHz * dt;
        if (phase > 6.2831853f) phase -= 6.2831853f;
        // low-passed random walk, ~ +-7 cents at full amount
        drift += 0.6f * dt * (nextRand() * 40.0f) - 1.4f * dt * drift;
        drift = std::max (-14.0f, std::min (14.0f, drift));
        return vibratoAmt * 24.0f * depthMod * std::sin (phase) + driftAmt * 0.5f * drift;
    }
private:
    float nextRand() { state = state * 1664525u + 1013904223u; return (float) ((state >> 8) & 0xffff) / 32768.0f - 1.0f; }  // -1..1
    uint32_t state = 1u;
    float phase = 0.0f, drift = 0.0f, depthMod = 1.0f;
};
} // namespace atlas
