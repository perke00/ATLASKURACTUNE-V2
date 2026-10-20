#pragma once
#include <cmath>
#include <algorithm>

namespace atlas
{
constexpr double kPi = 3.14159265358979323846;
}

namespace atlas
{
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
    }
    void highPass (double sr, double f, double q)
    {
        const double w = 2.0 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peak (double sr, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highShelf (double sr, double f, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt (2.0), tA = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + tA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - tA),
             (A + 1) - (A - 1) * c + tA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - tA);
    }
};

class DeEsser
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        hp.highPass (sr, 6000.0, 0.8);
        hp2.highPass (sr, 6000.0, 0.8);
        att = (float) (1.0 - std::exp (-1.0 / (0.0007 * sr)));
        rel = (float) (1.0 - std::exp (-1.0 / (0.04 * sr)));
        env = 0; hp.reset(); hp2.reset();
    }
    float process (float x, float amount)
    {
        if (amount < 0.005f) return x;
        const float band = hp2.process (hp.process (x));
        const float a = std::fabs (band);
        env += (a > env ? att : rel) * (a - env);
        const float thrDb = -18.0f - 22.0f * amount;
        const float envDb = 20.0f * std::log10 (env + 1e-9f);
        const float grDb = std::max (0.0f, envDb - thrDb) * 0.75f;
        const float g = std::pow (10.0f, -std::min (grDb, 14.0f) / 20.0f);
        return x - band * (1.0f - g);
    }
private:
    double sr = 44100;
    Biquad hp, hp2;
    float env = 0, att = 0.1f, rel = 0.001f;
};

class VocalCompressor
{
public:
    void prepare (double sampleRate)
    {
        att = (float) (1.0 - std::exp (-1.0 / (0.004 * sampleRate)));
        rel = (float) (1.0 - std::exp (-1.0 / (0.09 * sampleRate)));
        env = 0;
    }
    void process (float& l, float& r, float amount)
    {
        if (amount < 0.005f) return;
        const float thr = -4.0f - 22.0f * amount;
        const float ratio = 1.5f + 4.5f * amount;
        const float makeup = -thr * (1.0f - 1.0f / ratio) * 0.4f;
        const float a = std::max (std::fabs (l), std::fabs (r));
        env += (a > env ? att : rel) * (a - env);
        const float lvl = 20.0f * std::log10 (env + 1e-9f);
        const float gr = std::max (0.0f, lvl - thr) * (1.0f - 1.0f / ratio);
        const float g = std::pow (10.0f, (makeup - gr) / 20.0f);
        l *= g; r *= g;
    }
private:
    float env = 0, att = 0.1f, rel = 0.001f;
};

class VocalEq
{
public:
    void prepare (double sampleRate) { sr = sampleRate; lastAmount = -1.0f; }
    void update (float amount)
    {
        if (std::fabs (amount - lastAmount) < 1e-4f) return;
        lastAmount = amount;
        for (int c = 0; c < 2; ++c)
        {
            low[c].highPass (sr, 70.0, 0.707);
            mid[c].peak (sr, 3200.0, 0.9, 2.5 * amount);
            air[c].highShelf (sr, 10000.0, 4.0 * amount);
        }
    }
    float process (int ch, float x, float amount)
    {
        if (amount < 0.005f) return x;
        return air[ch].process (mid[ch].process (low[ch].process (x)));
    }
private:
    double sr = 44100; float lastAmount = -1.0f;
    Biquad low[2], mid[2], air[2];
};
} // namespace atlas
