#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

namespace atlas
{
using NoteMask = std::array<bool, 12>;

struct ScaleDef { const char* name; std::vector<int> degrees; };

inline const std::vector<ScaleDef>& scaleDefs()
{
    static const std::vector<ScaleDef> defs = {
        { "Chromatic",        { 0,1,2,3,4,5,6,7,8,9,10,11 } },
        { "Major",            { 0,2,4,5,7,9,11 } },
        { "Minor",            { 0,2,3,5,7,8,10 } },
        { "Harmonic Minor",   { 0,2,3,5,7,8,11 } },
        { "Melodic Minor",    { 0,2,3,5,7,9,11 } },
        { "Dorian",           { 0,2,3,5,7,9,10 } },
        { "Phrygian",         { 0,1,3,5,7,8,10 } },
        { "Lydian",           { 0,2,4,6,7,9,11 } },
        { "Mixolydian",       { 0,2,4,5,7,9,10 } },
        { "Major Pentatonic", { 0,2,4,7,9 } },
        { "Minor Pentatonic", { 0,3,5,7,10 } },
        { "Blues",            { 0,3,5,6,7,10 } },
    };
    return defs;
}

inline NoteMask makeMask (int scaleIndex, int root)
{
    NoteMask m{};
    const auto& defs = scaleDefs();
    const auto& d = defs[(size_t) std::max (0, std::min ((int) defs.size() - 1, scaleIndex))].degrees;
    for (int deg : d) m[(size_t) (((deg + root) % 12 + 12) % 12)] = true;
    return m;
}

inline int pitchClass (int note) { return ((note % 12) + 12) % 12; }

inline int countNotes (const NoteMask& m)
{
    int c = 0; for (bool b : m) c += b ? 1 : 0; return c;
}

// nearest allowed note (MIDI number) to a fractional MIDI pitch; prev gives a little hysteresis
inline int nearestNote (float midi, const NoteMask& maskIn, int prev, float hyst = 0.18f)
{
    NoteMask mask = maskIn;
    if (countNotes (mask) == 0) mask.fill (true);

    const int base = (int) std::floor (midi);
    int best = base; float bestD = 1e9f;
    for (int c = base - 7; c <= base + 8; ++c)
    {
        if (! mask[(size_t) pitchClass (c)]) continue;
        const float d = std::fabs (midi - (float) c);
        if (d < bestD) { bestD = d; best = c; }
    }
    if (prev >= 0 && mask[(size_t) pitchClass (prev)] && std::fabs (midi - (float) prev) <= bestD + hyst)
        return prev;
    return best;
}

// move 'steps' scale degrees up (positive) or down (negative) from a note
inline int stepInScale (int note, int steps, const NoteMask& maskIn)
{
    NoteMask mask = maskIn;
    if (countNotes (mask) == 0) mask.fill (true);
    if (steps == 0) return note;
    const int dir = steps > 0 ? 1 : -1;
    int remaining = std::abs (steps), n = note, guard = 0;
    while (remaining > 0 && guard++ < 96)
    {
        n += dir;
        if (mask[(size_t) pitchClass (n)]) --remaining;
    }
    return n;
}

// ---- key / note spelling (flats vs sharps follow the key signature, like a real tuner) ----------
inline const char* const* sharpNames() { static const char* n[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }; return n; }
inline const char* const* flatNames()  { static const char* n[] = { "C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B" }; return n; }

// offset (semitones) from the scale's tonic down to its relative major, and whether the scale is "major-like"
inline int relativeMajorOffset (int scaleIndex)
{
    switch (scaleIndex)
    {
        case 1: case 9: return 0;     // Major, Major Pentatonic
        case 5: return 2;             // Dorian
        case 6: return 4;             // Phrygian
        case 7: return 5;             // Lydian
        case 8: return 7;             // Mixolydian
        default: return 9;            // minor family / blues
    }
}

inline bool isMinorFamilyScale (int scaleIndex) { return relativeMajorOffset (scaleIndex) == 9; }

inline bool useFlats (int root, int scaleIndex)
{
    if (scaleIndex == 0) return false;                                  // chromatic: sharps
    const int rel = ((root - relativeMajorOffset (scaleIndex)) % 12 + 12) % 12;
    if (rel == 6) return root == 3 && isMinorFamilyScale (scaleIndex);   // Eb minor (6 flats) vs F#/Gb major (6 sharps)
    return rel == 5 || rel == 10 || rel == 3 || rel == 8 || rel == 1;   // F Bb Eb Ab Db major
}

inline std::string noteSpelling (int pitchClassValue, bool flats)
{
    const int pc = ((pitchClassValue % 12) + 12) % 12;
    return (flats ? flatNames() : sharpNames())[pc];
}

// "Eb Minor", "F# Major", ...
inline std::string keyLabel (int root, int scaleIndex)
{
    const auto& d = scaleDefs()[(size_t) std::max (0, std::min ((int) scaleDefs().size() - 1, scaleIndex))];
    if (scaleIndex == 0) return std::string (sharpNames()[((root % 12) + 12) % 12]) + " Chromatic";
    return noteSpelling (root, useFlats (root, scaleIndex)) + " " + d.name;
}

// relative major/minor shown under the key name ("Gb Major" for Eb Minor)
inline std::string relativeLabel (int root, int scaleIndex)
{
    const bool fl = useFlats (root, scaleIndex);     // relative key shares the key signature
    if (scaleIndex == 1)  return noteSpelling ((root + 9) % 12, fl) + " Minor";
    if (scaleIndex == 2)  return noteSpelling ((root + 3) % 12, fl) + " Major";
    return {};
}
} // namespace atlas

