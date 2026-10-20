#include "PluginEditor.h"
#include <BinaryData.h>

using namespace juce;

namespace
{
    const char* noteLabels[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    bool isBlackKey (int note) { const int pc = ((note % 12) + 12) % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
    constexpr float arcStart = MathConstants<float>::pi * 1.25f, arcEnd = MathConstants<float>::pi * 2.75f;
}

// ---------------------------------------------------------------------------------------------
Font ui::font (float size, int weight)
{
    static Typeface::Ptr faces[3];
    static bool loaded = false;
    if (! loaded)
    {
        loaded = true;
        faces[0] = Typeface::createSystemTypefaceFor (BinaryData::MontserratLight_ttf, (size_t) BinaryData::MontserratLight_ttfSize);
        faces[1] = Typeface::createSystemTypefaceFor (BinaryData::MontserratMedium_ttf, (size_t) BinaryData::MontserratMedium_ttfSize);
        faces[2] = Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, (size_t) BinaryData::MontserratSemiBold_ttfSize);
    }
    const int w = jlimit (0, 2, weight);
    if (faces[w] != nullptr)
        return Font (faces[w]).withHeight (size * 0.9f);
    return Font (size, w == 2 ? Font::bold : Font::plain);
}

void ui::drawSpaced (Graphics& g, const String& t, Rectangle<int> r, float size, float kerning, Justification j, bool bold)
{
    Font f = ui::font (size, bold ? 2 : 1);
    f.setExtraKerningFactor (kerning);
    g.setFont (f);
    g.drawText (t, r, j, false);
}

void ui::drawLogo (Graphics& g, Rectangle<float> r)
{
    const float w = r.getWidth(), t = w * 0.17f;
    Path p;
    p.startNewSubPath (r.getX() + t * 0.3f, r.getBottom());
    p.lineTo (r.getCentreX(), r.getY() + t * 0.2f);
    p.lineTo (r.getRight() - t * 0.3f, r.getBottom());
    ColourGradient grad (Colour (0xfff2f2f4), r.getX(), r.getY(), Colour (0xff8d8d94), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.strokePath (p, PathStrokeType (t, PathStrokeType::mitered, PathStrokeType::butt));
}

// ---------------------------------------------------------------------------------------------
AtlasLNF::AtlasLNF()
{
    setColour (ComboBox::textColourId, ui::text);
    setColour (PopupMenu::backgroundColourId, Colour (0xff131316));
    setColour (PopupMenu::textColourId, ui::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff2c2c32));
    setColour (PopupMenu::highlightedTextColourId, ui::bright);
    setColour (TextButton::textColourOffId, ui::dim);
    setColour (TextButton::textColourOnId, ui::bright);
    setColour (Label::textColourId, ui::dim);
}

void AtlasLNF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider& sl)
{
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float d = jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float r = d * 0.5f;
    const bool bipolar = (bool) sl.getProperties().getWithDefault ("bipolar", false);
    const float p = jlimit (0.0f, 1.0f, pos);

    // knob body
    g.setColour (Colour (0xff060607));
    g.fillEllipse (c.x - r, c.y - r, d, d);
    ColourGradient grad (Colour (0xff202025), c.x, c.y - r, Colour (0xff09090b), c.x, c.y + r, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r * 0.74f, c.y - r * 0.74f, d * 0.74f, d * 0.74f);
    g.setColour (Colour (0xff2a2a30));
    g.drawEllipse (c.x - r * 0.74f, c.y - r * 0.74f, d * 0.74f, d * 0.74f, 1.0f);

    const float arcR = r * 0.88f;
    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, arcStart, arcEnd, true);
    g.setColour (Colour (0xff25252a));
    g.strokePath (track, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));

    const float a = arcStart + p * (arcEnd - arcStart);
    const float a0 = bipolar ? arcStart + 0.5f * (arcEnd - arcStart) : arcStart;
    const float lo = jmin (a0, a), hi = jmax (a0, a);
    const float amount = bipolar ? std::fabs (p - 0.5f) * 2.0f : p;      // 0..1 glow strength

    if (hi - lo > 0.002f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, lo, hi, true);

        // layered halo: wide + faint -> narrow + strong, strength grows smoothly with the value
        const float strength = 0.25f + 0.75f * amount;
        for (int i = 5; i >= 1; --i)
        {
            const float f = (float) i / 5.0f;
            g.setColour (Colours::white.withAlpha (0.045f * strength * (1.15f - f)));
            g.strokePath (val, PathStrokeType (2.6f + 12.0f * f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (Colour (0xfff4f4f6));
        g.strokePath (val, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // soft light on the cap that follows the value
    {
        const float gr = r * 0.74f;
        ColourGradient glow (Colours::white.withAlpha (0.05f + 0.10f * amount), c.x, c.y, Colours::transparentWhite, c.x, c.y - gr, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - gr, c.y - gr, gr * 2.0f, gr * 2.0f);
    }

    Path tick;
    tick.startNewSubPath (0.0f, -r * 0.60f);
    tick.lineTo (0.0f, -r * 0.30f);
    g.setColour (Colour (0xfff4f4f6));
    g.strokePath (tick, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::rotation (a).translated (c.x, c.y));
}

void AtlasLNF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float sliderPos, float, float, Slider::SliderStyle, Slider&)
{
    const float cy = (float) y + (float) h * 0.5f;
    const float x0 = (float) x + 4.0f, x1 = (float) (x + w) - 4.0f;
    g.setColour (Colour (0xff26262b));
    g.fillRoundedRectangle (x0, cy - 2.0f, x1 - x0, 4.0f, 2.0f);
    g.setColour (Colour (0xffd9d9de));
    g.fillRoundedRectangle (x0, cy - 2.0f, jmax (0.0f, sliderPos - x0), 4.0f, 2.0f);
    g.setColour (Colours::white);
    g.fillEllipse (sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f);
}

void AtlasLNF::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&)
{
    const auto b = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (Colour (0xff0d0d10));
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (b, 5.0f, 1.0f);
    Path ar;
    const float cx = (float) w - 18.0f, cy = (float) h * 0.5f;
    ar.startNewSubPath (cx - 4.5f, cy - 2.0f); ar.lineTo (cx, cy + 3.0f); ar.lineTo (cx + 4.5f, cy - 2.0f);
    g.setColour (ui::text);
    g.strokePath (ar, PathStrokeType (1.5f));
}

void AtlasLNF::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (12, 1, box.getWidth() - 36, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void AtlasLNF::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? Colour (0xff1c1c21) : Colour (0xff0d0d10));
    g.fillRoundedRectangle (r, 5.0f);
    if (highlighted || down) { g.setColour (Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r, 5.0f); }
    g.setColour (on ? Colour (0xffe8e8ea) : ui::border);
    g.drawRoundedRectangle (r, 5.0f, on ? 1.3f : 1.0f);
}

// ---------------------------------------------------------------------------------------------
Knob::Knob (AudioProcessorValueTreeState& apvts, const String& id, const String& title, const String& unit, bool bipolar) : unitText (unit), isBipolar (bipolar)
{
    addAndMakeVisible (slider);
    slider.getProperties().set ("bipolar", bipolar);
    slider.setRotaryParameters (arcStart, arcEnd, true);
    name.setText (title, dontSendNotification);
    name.setJustificationType (Justification::centred);
    name.setFont (ui::font (12.0f, 1));
    name.setColour (Label::textColourId, ui::dim);
    value.setJustificationType (Justification::centred);
    value.setFont (ui::font (14.0f, 1));
    value.setColour (Label::textColourId, ui::text);
    addAndMakeVisible (name);
    addAndMakeVisible (value);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, id, slider);
    slider.onValueChange = [this] { refresh(); };
    refresh();
}

void Knob::refresh()
{
    int v = roundToInt (slider.getValue());
    if (isBipolar) { v -= 50; value.setText ((v > 0 ? "+" : "") + String (v) + unitText, dontSendNotification); }
    else value.setText (String (v) + unitText, dontSendNotification);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    value.setBounds (r.removeFromBottom (20));
    name.setBounds (r.removeFromBottom (16));
    slider.setBounds (r);
}

// ---------------------------------------------------------------------------------------------
void CenterDial::paint (Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f, cy = 168.0f, R = 152.0f;

    for (int i = 0; i < 3; ++i)
    {
        g.setColour (Colours::white.withAlpha (0.05f - 0.012f * (float) i));
        g.drawEllipse (cx - R - 14.0f - 8.0f * (float) i, cy - R - 14.0f - 8.0f * (float) i, (R + 14.0f + 8.0f * (float) i) * 2.0f, (R + 14.0f + 8.0f * (float) i) * 2.0f, 1.0f);
    }
    g.setColour (Colour (0xff060607));
    g.fillEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f);

    Path track;
    track.addCentredArc (cx, cy, R - 10.0f, R - 10.0f, 0.0f, arcStart, arcEnd, true);
    g.setColour (Colour (0xff1b1b1f));
    g.strokePath (track, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const float outDb = proc.outLevelDb.load();
    const float frac = jlimit (0.0f, 1.0f, (outDb + 60.0f) / 60.0f);
    Path val;
    val.addCentredArc (cx, cy, R - 10.0f, R - 10.0f, 0.0f, arcStart, arcStart + frac * (arcEnd - arcStart) + 0.001f, true);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.strokePath (val, PathStrokeType (16.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::white.withAlpha (0.95f));
    g.strokePath (val, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour (Colour (0xff0c0c0e));
    g.fillEllipse (cx - R + 28.0f, cy - R + 28.0f, (R - 28.0f) * 2.0f, (R - 28.0f) * 2.0f);
    g.setColour (Colour (0xff1c1c20));
    g.drawEllipse (cx - R + 28.0f, cy - R + 28.0f, (R - 28.0f) * 2.0f, (R - 28.0f) * 2.0f, 1.0f);

    // ---- note display in the middle -----------------------------------------------------------
    const bool flats = atlas::useFlats ((int) proc.apvts.getRawParameterValue (ids::key)->load(), (int) proc.apvts.getRawParameterValue (ids::scale)->load());
    auto spell = [flats] (int n) { return String (atlas::noteSpelling (n, flats)); };
    const float det = proc.detectedMidi.load(), tgtNow = proc.targetMidi.load();
    const bool voiced = det > 0.0f && tgtNow > 0.0f;
    const int tgt = voiced ? (int) tgtNow : lastTarget;
    const float alpha = voiced ? 1.0f : 0.35f;

    g.setColour (ui::dim);
    ui::drawSpaced (g, "TARGET NOTE", { (int) cx - 90, (int) cy - 98, 180, 16 }, 11.0f, 0.4f, Justification::centred);

    // big note name (+ octave)
    const String pc = tgt >= 0 ? spell (tgt) : String ("--");
    const String oct = tgt >= 0 ? String (tgt / 12 - 1) : String();
    {
        Font big = ui::font (74.0f, 2);
        Font small = ui::font (22.0f, 1);
        const float wBig = big.getStringWidthFloat (pc), wSmall = small.getStringWidthFloat (oct);
        const float x0 = cx - (wBig + wSmall + 2.0f) * 0.5f;
        g.setColour (Colours::white.withAlpha (0.10f * alpha));
        g.setFont (big);
        g.drawText (pc, Rectangle<float> (x0, cy - 84.0f + 2.0f, wBig + 4.0f, 80.0f), Justification::centredLeft, false);
        g.setColour (ui::bright.withAlpha (alpha));
        g.drawText (pc, Rectangle<float> (x0, cy - 84.0f, wBig + 4.0f, 80.0f), Justification::centredLeft, false);
        g.setFont (small);
        g.setColour (ui::dim.withAlpha (alpha));
        g.drawText (oct, Rectangle<float> (x0 + wBig + 2.0f, cy - 52.0f, wSmall + 6.0f, 30.0f), Justification::centredLeft, false);
    }

    // cents meter
    {
        const float cents = voiced ? jlimit (-50.0f, 50.0f, (det - tgtNow) * 100.0f) : 0.0f;
        const Rectangle<float> bar (cx - 80.0f, cy + 8.0f, 160.0f, 4.0f);
        g.setColour (Colour (0xff1d1d21));
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (Colour (0xff3a3a40));
        g.fillRect (cx - 0.5f, bar.getY() - 4.0f, 1.0f, 12.0f);
        for (int i = -2; i <= 2; ++i) if (i != 0) g.fillRect (cx + (float) i * 40.0f - 0.5f, bar.getY() - 1.0f, 1.0f, 6.0f);
        if (voiced)
        {
            const float mx = cx + cents * 1.6f;
            const bool inTune = std::fabs (cents) < 6.0f;
            g.setColour (Colours::white.withAlpha (0.18f));
            g.fillEllipse (mx - 9.0f, bar.getCentreY() - 9.0f, 18.0f, 18.0f);
            g.setColour (inTune ? Colour (0xffffffff) : Colour (0xffc9c9cf));
            g.fillEllipse (mx - 5.0f, bar.getCentreY() - 5.0f, 10.0f, 10.0f);
        }
        g.setColour (ui::dim);
        g.setFont (ui::font (13.0f, 1));
        g.drawText (voiced ? ((cents >= 0 ? "+" : "") + String (roundToInt (cents)) + " cents") : String ("no signal"),
                    Rectangle<int> ((int) cx - 80, (int) cy + 18, 160, 18), Justification::centred);
    }

    // the notes that fit the scale around the current note
    if (tgt >= 0)
    {
        atlas::NoteMask mask {};
        for (int i = 0; i < 12; ++i) mask[(size_t) i] = proc.apvts.getRawParameterValue ("note" + String (i))->load() > 0.5f;
        const float slotW = 26.0f, y = cy + 46.0f;
        for (int k = -3; k <= 3; ++k)
        {
            const int n = k == 0 ? tgt : atlas::stepInScale (tgt, k, mask);
            const Rectangle<float> r (cx + (float) k * slotW - slotW * 0.5f, y, slotW, 24.0f);
            if (k == 0)
            {
                g.setColour (Colours::white.withAlpha (0.92f * alpha));
                g.fillRoundedRectangle (r.reduced (1.0f, 0.0f), 8.0f);
                g.setColour (Colour (0xff0c0c0e));
                g.setFont (ui::font (13.0f, 2));
            }
            else
            {
                g.setColour (Colours::white.withAlpha ((0.55f - 0.12f * (float) std::abs (k)) * (voiced ? 1.0f : 0.6f)));
                g.setFont (ui::font (13.0f, 1));
            }
            g.drawText (spell (n), r, Justification::centred);
        }
    }

    // recently sung notes
    if (! history.empty())
    {
        String h;
        const int cnt = (int) history.size();
        for (int i = 0; i < cnt; ++i) h << (i ? "  >  " : "") << spell (history[(size_t) i]) << String (history[(size_t) i] / 12 - 1);
        g.setColour (ui::dim.withAlpha (0.8f));
        g.setFont (ui::font (12.0f, 1));
        g.drawText (h, Rectangle<int> ((int) cx - 92, (int) cy + 80, 184, 18), Justification::centred);
    }

    // input level bar
    const float inDb = proc.inLevelDb.load();
    const Rectangle<float> bar (cx - 100.0f, 340.0f, 200.0f, 6.0f);
    g.setColour (Colour (0xff1d1d21));
    g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (Colour (0xffdcdce0));
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * jlimit (0.0f, 1.0f, (inDb + 60.0f) / 60.0f)), 3.0f);
    g.setColour (ui::dim);
    g.setFont (ui::font (14.0f, 1));
    g.drawText ("IN  " + String (jmax (-99.9f, inDb), 1) + " dB", Rectangle<int> ((int) cx - 100, 352, 200, 20), Justification::centred);
}

// ---------------------------------------------------------------------------------------------
void PitchGraph::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0b0d));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);

    const float labelW = 34.0f;
    const auto plot = b.reduced (6.0f).withTrimmedRight (labelW);
    g.setColour (Colour (0xff1b1b1f));
    for (int i = 1; i < 8; ++i) g.drawVerticalLine (roundToInt (plot.getX() + plot.getWidth() * (float) i / 8.0f), plot.getY(), plot.getBottom());
    for (int i = 0; i <= 4; ++i) g.drawHorizontalLine (roundToInt (plot.getY() + plot.getHeight() * (float) i / 4.0f), plot.getX(), plot.getRight());
    g.setColour (ui::dim);
    g.setFont (ui::font (11.0f, 1));
    g.drawText ("+12", Rectangle<int> ((int) plot.getRight() + 4, (int) plot.getY() - 6, 30, 12), Justification::centredLeft);
    g.drawText ("0", Rectangle<int> ((int) plot.getRight() + 4, (int) plot.getCentreY() - 6, 30, 12), Justification::centredLeft);
    g.drawText ("-12", Rectangle<int> ((int) plot.getRight() + 4, (int) plot.getBottom() - 6, 30, 12), Justification::centredLeft);

    const int N = AtlasProcessor::graphSize, start = proc.graphPos.load();
    Path p;
    for (int i = 0; i < N; ++i)
    {
        const float v = proc.graph[(size_t) ((start + i) % N)].load();
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) (N - 1);
        const float y = plot.getCentreY() - (v / 12.0f) * plot.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (Colours::white.withAlpha (0.15f));
    g.strokePath (p, PathStrokeType (4.0f, PathStrokeType::curved));
    g.setColour (Colour (0xffe9e9ec));
    g.strokePath (p, PathStrokeType (1.4f, PathStrokeType::curved));
}

// ---------------------------------------------------------------------------------------------
PianoComponent::PianoComponent (AtlasProcessor& p) : proc (p)
{
    for (int i = 0; i < 12; ++i) noteParams[(size_t) i] = proc.apvts.getRawParameterValue ("note" + String (i));
}

void PianoComponent::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth() / (float) whiteCount, bw = w * 0.62f, bh = b.getHeight() * 0.62f;
    const int tgt = roundToInt (proc.targetMidi.load()), det = proc.detectedMidi.load() > 0 ? roundToInt (proc.detectedMidi.load()) : -1;

    auto inScale = [this] (int note) { return noteParams[(size_t) (note % 12)]->load() > 0.5f; };

    int wi = 0;
    for (int note = firstNote; note < firstNote + totalNotes; ++note)
    {
        if (isBlackKey (note)) continue;
        const Rectangle<float> k ((float) wi * w, 0.0f, w, b.getHeight());
        const bool active = (note == tgt);
        ColourGradient grad (active ? Colour (0xffe2e2e6) : Colour (0xff202024), 0.0f, 0.0f,
                             active ? Colour (0xff8e8e95) : Colour (0xff0e0e11), 0.0f, b.getHeight(), false);
        g.setGradientFill (grad);
        g.fillRect (k.reduced (0.5f, 0.0f));
        g.setColour (Colour (0xff2c2c31));
        g.drawRect (k, 1.0f);
        if (inScale (note))
        {
            g.setColour (active ? Colour (0xff111114) : Colours::white);
            g.fillEllipse (k.getCentreX() - 3.5f, b.getBottom() - 30.0f, 7.0f, 7.0f);
        }
        if (note == det && note != tgt)
        {
            g.setColour (Colours::white.withAlpha (0.4f));
            g.drawEllipse (k.getCentreX() - 6.0f, b.getBottom() - 32.5f, 12.0f, 12.0f, 1.0f);
        }
        if (note % 12 == 0)
        {
            g.setColour (active ? Colour (0xff111114) : Colour (0xff6d6d74));
            g.setFont (ui::font (10.0f, 1));
            g.drawText ("C" + String (note / 12 - 1), k.withTrimmedTop (b.getHeight() - 20.0f), Justification::centred);
        }
        ++wi;
    }
    wi = 0;
    for (int note = firstNote; note < firstNote + totalNotes; ++note)
    {
        if (! isBlackKey (note)) { ++wi; continue; }
        const Rectangle<float> k ((float) wi * w - bw * 0.5f, 0.0f, bw, bh);
        const bool active = (note == tgt);
        g.setColour (active ? Colour (0xffa4a4ab) : Colour (0xff060607));
        g.fillRect (k);
        g.setColour (Colour (0xff2a2a2f));
        g.drawRect (k, 1.0f);
        if (inScale (note))
        {
            g.setColour (active ? Colour (0xff111114) : Colours::white);
            g.fillEllipse (k.getCentreX() - 3.5f, k.getBottom() - 14.0f, 7.0f, 7.0f);
        }
    }
}

int PianoComponent::noteAt (Point<float> pt) const
{
    const float w = (float) getWidth() / (float) whiteCount, bw = w * 0.62f, bh = (float) getHeight() * 0.62f;
    int wi = 0;
    // black keys first (they sit on top)
    for (int note = firstNote; note < firstNote + totalNotes; ++note)
    {
        if (! isBlackKey (note)) { ++wi; continue; }
        if (Rectangle<float> ((float) wi * w - bw * 0.5f, 0.0f, bw, bh).contains (pt)) return note;
    }
    wi = 0;
    for (int note = firstNote; note < firstNote + totalNotes; ++note)
    {
        if (isBlackKey (note)) continue;
        if (Rectangle<float> ((float) wi * w, 0.0f, w, (float) getHeight()).contains (pt)) return note;
        ++wi;
    }
    return -1;
}

void PianoComponent::mouseDown (const MouseEvent& e)
{
    const int note = noteAt (e.position);
    if (note < 0) return;
    if (auto* p = proc.apvts.getParameter ("note" + String (note % 12)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
}

// ---------------------------------------------------------------------------------------------
AtlasEditor::AtlasEditor (AtlasProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      retuneK (p.apvts, ids::retune, "RETUNE SPEED", " ms"), humanK (p.apvts, ids::humanize, "HUMANIZE", " %"),
      amountK (p.apvts, ids::amount, "PITCH AMOUNT", " %"), formantK (p.apvts, ids::formant, "THROAT", "", true),
      mixK (p.apvts, ids::mix, "DRY / WET", " %"), deessK (p.apvts, ids::deesser, "DE-ESSER", " %"),
      compK (p.apvts, ids::comp, "COMPRESSOR", " %"), eqK (p.apvts, ids::eq, "EQ", " %"),
      hMixK (p.apvts, ids::harmMix, "MIX", " %"), hSpreadK (p.apvts, ids::harmSpread, "WIDTH", " %"),
      chSizeK (p.apvts, ids::choirSize, "SIZE", " %"), chEnsK (p.apvts, ids::choirEns, "ENSEMBLE", " %"),
      chVibK (p.apvts, ids::choirVib, "VIBRATO", " %"), chRoomK (p.apvts, ids::choirRoom, "ROOM", " %"),
      chDroneK (p.apvts, ids::choirDrone, "DRONE", " %"),
      dial (p), graph (p), piano (p)
{
    setLookAndFeel (&lnf);
    setSize (1100, 800);

    // presets
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    // input type, key & scale
    {
        auto* inParam = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (ids::inType));
        inputBox.addItemList (inParam->choices, 1);
        addAndMakeVisible (inputBox);
        inputAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::inType, inputBox);

        auto* keyParam = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (ids::key));
        keyBox.addItemList (keyParam->choices, 1);
        addAndMakeVisible (keyBox);
        keyAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::key, keyBox);
        keyBox.onChange = [this] { proc.applyScaleToNotes(); repaint(); };

        auto* scaleParam = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (ids::scale));
        scaleBox.addItemList (scaleParam->choices, 1);
        addAndMakeVisible (scaleBox);
        scaleAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::scale, scaleBox);
        scaleBox.onChange = [this] { proc.applyScaleToNotes(); repaint(); };
    }

    // mode
    const char* modeNames[] = { "Classic", "Modern", "Auto" };
    modeAtt = std::make_unique<ParameterAttachment> (*proc.apvts.getParameter (ids::mode), [this] (float v)
    {
        for (int i = 0; i < 3; ++i) modeBtns[(size_t) i].setToggleState (i == (int) v, dontSendNotification);
    });
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeBtns[(size_t) i];
        b.setButtonText (modeNames[i]);
        b.setRadioGroupId (202);
        b.onClick = [this, i] { modeAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    modeAtt->sendInitialUpdate();

    lowLatBtn.setClickingTogglesState (true);
    lowLatAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::lowLat, lowLatBtn);
    addAndMakeVisible (lowLatBtn);
    {
        const char* hmNames[] = { "Harmony", "Female Choir" };
        harmModeAtt = std::make_unique<ParameterAttachment> (*proc.apvts.getParameter (ids::harmMode), [this] (float v)
        {
            for (int i = 0; i < 2; ++i) harmModeBtns[(size_t) i].setToggleState (i == (int) v, dontSendNotification);
        });
        for (int i = 0; i < 2; ++i)
        {
            auto& b = harmModeBtns[(size_t) i];
            b.setButtonText (hmNames[i]);
            b.setRadioGroupId (203);
            b.onClick = [this, i] { harmModeAtt->setValueAsCompleteGesture ((float) i); updateHarmonyLayout(); };
            addAndMakeVisible (b);
        }
        harmModeAtt->sendInitialUpdate();

        auto* csParam = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (ids::choirStyle));
        choirStyleBox.addItemList (csParam->choices, 1);
        addAndMakeVisible (choirStyleBox);
        choirStyleAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::choirStyle, choirStyleBox);
        choirStyleBox.onChange = [this] { repaint(); };
        for (auto* k : { &chSizeK, &chEnsK, &chVibK, &chRoomK, &chDroneK }) addAndMakeVisible (*k);
    }
    harmBtn.setClickingTogglesState (true);
    harmBtn.onClick = [this] { updateHarmonyLayout(); };
    harmAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::harmOn, harmBtn);
    addAndMakeVisible (harmBtn);

    for (auto* k : { &retuneK, &humanK, &amountK, &formantK, &mixK, &deessK, &compK, &eqK, &hMixK, &hSpreadK }) addAndMakeVisible (*k);
    addAndMakeVisible (dial);
    addAndMakeVisible (graph);
    addAndMakeVisible (piano);

    outSlider.setRange (-12.0, 12.0, 0.1);
    addAndMakeVisible (outSlider);
    outAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids::outGain, outSlider);

    auto* intParam = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter ("vint1"));
    auto* typeParam = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter ("vtype1"));
    for (int v = 0; v < AtlasProcessor::numVoices; ++v)
    {
        auto& s = strips[(size_t) v];
        s.interval.addItemList (intParam->choices, 1);
        s.type.addItemList (typeParam->choices, 1);
        addAndMakeVisible (s.interval); addAndMakeVisible (s.type); addAndMakeVisible (s.level);
        s.iAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "vint" + String (v + 1), s.interval);
        s.tAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "vtype" + String (v + 1), s.type);
        s.lAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "vlevel" + String (v + 1), s.level);
    }
    setSize (1100, 800);
    updateHarmonyLayout();
    startTimerHz (30);
}

AtlasEditor::~AtlasEditor() { stopTimer(); setLookAndFeel (nullptr); }

void CenterDial::tick()
{
    const float t = proc.targetMidi.load();
    if (t > 0.0f)
    {
        const int n = (int) t;
        if (n != lastTarget)
        {
            lastTarget = n;
            history.push_back (n);
            if (history.size() > 4) history.erase (history.begin());
        }
    }
}

void AtlasEditor::timerCallback()
{
    if (harmBtn.getToggleState() != harmOpen || (proc.apvts.getRawParameterValue (ids::harmMode)->load() > 0.5f) != choirOpen) updateHarmonyLayout();
    dial.tick();
    dial.repaint(); graph.repaint(); piano.repaint();
}

void AtlasEditor::updateHarmonyLayout()
{
    harmOpen = harmBtn.getToggleState();
    choirOpen = proc.apvts.getRawParameterValue (ids::harmMode)->load() > 0.5f;
    resized();
    repaint();
}

void AtlasEditor::resized()
{
    // background texture (rebuilt only when the size changes)
    if (background.getWidth() != getWidth() || background.getHeight() != getHeight())
    {
        background = Image (Image::RGB, jmax (1, getWidth()), jmax (1, getHeight()), false);
        Graphics g (background);
        g.setGradientFill (ColourGradient (Colour (0xff101012), 0.0f, 0.0f, Colour (0xff050506), 0.0f, (float) getHeight(), false));
        g.fillAll();
        Random rnd (7);
        for (int i = 0; i < 2600; ++i)
        {
            const float x = rnd.nextFloat() * (float) getWidth(), y = rnd.nextFloat() * (float) getHeight();
            g.setColour (Colours::white.withAlpha (rnd.nextFloat() * 0.03f));
            g.drawLine (x, y, x + rnd.nextFloat() * 26.0f - 8.0f, y + rnd.nextFloat() * 14.0f - 4.0f, 1.0f + rnd.nextFloat() * 1.5f);
        }
    }

    presetBox.setBounds (getWidth() - 28 - 250, 38, 250, 34);

    const int L = 28, LW = 330, R = 742, RW = 330, top = 92;
    keyPanel   = { L, top, LW, 190 };
    tunePanel  = { L, top + 200, LW, 150 };
    modePanel  = { L, top + 360, LW, 108 };
    pitchPanel = { R, top, RW, 262 };
    vocalPanel = { R, top + 272, RW, 196 };
    harmPanel  = { 28, 572, 1044, harmOpen ? 122 : 42 };

    inputBox.setBounds (keyPanel.getX() + 16, keyPanel.getY() + 52, keyPanel.getWidth() - 32, 32);
    keyBox.setBounds   (keyPanel.getX() + 16, keyPanel.getY() + 108, 108, 32);
    scaleBox.setBounds (keyPanel.getX() + 16 + 116, keyPanel.getY() + 108, keyPanel.getWidth() - 32 - 116, 32);

    retuneK.setBounds (tunePanel.getX() + 20, tunePanel.getY() + 34, 130, 108);
    humanK.setBounds (tunePanel.getRight() - 150, tunePanel.getY() + 34, 130, 108);
    {
        const int bw = (modePanel.getWidth() - 32 - 12) / 3;
        for (int i = 0; i < 3; ++i) modeBtns[(size_t) i].setBounds (modePanel.getX() + 16 + i * (bw + 6), modePanel.getY() + 36, bw, 28);
        lowLatBtn.setBounds (modePanel.getX() + 16, modePanel.getY() + 72, modePanel.getWidth() - 32, 26);
    }

    graph.setBounds (pitchPanel.getX() + 16, pitchPanel.getY() + 38, pitchPanel.getWidth() - 32, 112);
    {
        const int kw = (pitchPanel.getWidth() - 24) / 3;
        amountK.setBounds (pitchPanel.getX() + 12, pitchPanel.getY() + 158, kw, 98);
        formantK.setBounds (pitchPanel.getX() + 12 + kw, pitchPanel.getY() + 158, kw, 98);
        mixK.setBounds (pitchPanel.getX() + 12 + 2 * kw, pitchPanel.getY() + 158, kw, 98);
        deessK.setBounds (vocalPanel.getX() + 12, vocalPanel.getY() + 44, kw, 140);
        compK.setBounds (vocalPanel.getX() + 12 + kw, vocalPanel.getY() + 44, kw, 140);
        eqK.setBounds (vocalPanel.getX() + 12 + 2 * kw, vocalPanel.getY() + 44, kw, 140);
    }

    dial.setBounds (374, top, 350, 380);
    outSlider.setBounds (374 + 85, top + 400, 190, 22);

    harmBtn.setBounds (harmPanel.getX() + 170, harmPanel.getY() + 9, 64, 24);
    for (int i = 0; i < 2; ++i)
    {
        harmModeBtns[(size_t) i].setBounds (harmPanel.getX() + 250 + i * 118, harmPanel.getY() + 9, 112, 24);
        harmModeBtns[(size_t) i].setVisible (harmOpen);
    }
    hMixK.setVisible (harmOpen); hSpreadK.setVisible (harmOpen);
    hMixK.setBounds (harmPanel.getX() + 16, harmPanel.getY() + 38, 100, 80);
    hSpreadK.setBounds (harmPanel.getX() + 124, harmPanel.getY() + 38, 100, 80);
    {
        const int sx = harmPanel.getX() + 250, sw = (harmPanel.getRight() - 14 - sx) / 4;
        const bool showStrips = harmOpen && ! choirOpen, showChoir = harmOpen && choirOpen;
        for (int v = 0; v < AtlasProcessor::numVoices; ++v)
        {
            auto& s = strips[(size_t) v];
            const int x = sx + v * sw;
            s.area = { x, harmPanel.getY() + 40, sw - 10, 78 };
            s.interval.setBounds (x, harmPanel.getY() + 42, sw - 18, 26);
            s.type.setBounds (x, harmPanel.getY() + 71, sw - 18, 26);
            s.level.setBounds (x, harmPanel.getY() + 98, sw - 18, 18);
            for (auto* c : { (juce::Component*) &s.interval, (juce::Component*) &s.type, (juce::Component*) &s.level }) c->setVisible (showStrips);
        }
        choirStyleBox.setVisible (showChoir);
        choirStyleBox.setBounds (sx, harmPanel.getY() + 58, 200, 30);
        const int kx = sx + 224, kw = (harmPanel.getRight() - 16 - kx) / 5;
        int i = 0;
        for (auto* k : { &chSizeK, &chEnsK, &chVibK, &chRoomK, &chDroneK })
        {
            k->setVisible (showChoir);
            k->setBounds (kx + i * kw, harmPanel.getY() + 38, kw, 80);
            ++i;
        }
    }

    const int py = harmPanel.getBottom() + 12;
    piano.setBounds (28, py, 1044, 786 - py);
}

void AtlasEditor::panel (Graphics& g, Rectangle<int> r, const String& title)
{
    g.setColour (ui::panel.withAlpha (0.92f));
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);
    g.setColour (ui::text);
    ui::drawSpaced (g, title, r.withTrimmedLeft (16).withHeight (34), 13.0f, 0.2f, Justification::centredLeft);
}

void AtlasEditor::paint (Graphics& g)
{
    g.drawImageAt (background, 0, 0);

    // ---- header: wordmark -------------------------------------------------------------------
    ui::drawLogo (g, { 34.0f, 20.0f, 44.0f, 38.0f });
    {
        Font fa = ui::font (31.0f, 0), ft = ui::font (31.0f, 2);
        fa.setExtraKerningFactor (0.34f); ft.setExtraKerningFactor (0.34f);
        const float wa = fa.getStringWidthFloat ("ATLAS"), wt = ft.getStringWidthFloat ("TUNE");
        const float x0 = 96.0f, y0 = 14.0f;
        g.setColour (Colour (0xffcfcfd4));
        g.setFont (fa);
        g.drawText ("ATLAS", Rectangle<float> (x0, y0, wa + 6.0f, 40.0f), Justification::centredLeft, false);
        g.setColour (ui::bright);
        g.setFont (ft);
        g.drawText ("TUNE", Rectangle<float> (x0 + wa + 18.0f, y0, wt + 6.0f, 40.0f), Justification::centredLeft, false);

        // PRO badge
        const Rectangle<float> badge (x0 + wa + 18.0f + wt + 20.0f, y0 + 9.0f, 40.0f, 20.0f);
        g.setColour (ui::bright);
        g.drawRoundedRectangle (badge, 4.0f, 1.2f);
        Font fp = ui::font (11.0f, 2); fp.setExtraKerningFactor (0.25f);
        g.setFont (fp);
        g.drawText ("PRO", badge.translated (1.0f, 0.0f), Justification::centred, false);
    }
    g.setColour (ui::dim);
    ui::drawSpaced (g, "VOCAL PITCH CORRECTION", { 98, 54, 360, 16 }, 10.5f, 0.42f, Justification::centredLeft);

    g.setColour (Colour (0xff8a8a92));
    ui::drawSpaced (g, "PRODBYPERKE", { getWidth() - 28 - 250, 16, 250, 16 }, 10.5f, 0.5f, Justification::centredRight);
    g.setColour (ui::border);
    g.drawHorizontalLine (80, 28.0f, (float) getWidth() - 28.0f);

    panel (g, keyPanel, "KEY & SCALE");
    panel (g, tunePanel, "TUNE");
    panel (g, modePanel, "MODE");
    panel (g, pitchPanel, "PITCH");
    panel (g, vocalPanel, "VOCAL CONTROL");
    panel (g, harmPanel, "HARMONY ENGINE");

    // key panel captions + key read-out (spelled like the key signature: Eb Minor, F# Major ...)
    g.setColour (ui::dim);
    ui::drawSpaced (g, "INPUT TYPE", { keyPanel.getX() + 18, keyPanel.getY() + 36, 120, 14 }, 9.5f, 0.35f, Justification::centredLeft);
    ui::drawSpaced (g, "KEY", { keyPanel.getX() + 18, keyPanel.getY() + 92, 60, 14 }, 9.5f, 0.35f, Justification::centredLeft);
    ui::drawSpaced (g, "SCALE", { keyPanel.getX() + 18 + 116, keyPanel.getY() + 92, 80, 14 }, 9.5f, 0.35f, Justification::centredLeft);
    {
        const int key = (int) proc.apvts.getRawParameterValue (ids::key)->load();
        const int scl = (int) proc.apvts.getRawParameterValue (ids::scale)->load();
        g.setColour (ui::bright);
        ui::drawSpaced (g, String (atlas::keyLabel (key, scl)), { keyPanel.getX() + 18, keyPanel.getY() + 148, 200, 32 }, 20.0f, 0.12f, Justification::centredLeft, true);
        const auto rel = atlas::relativeLabel (key, scl);
        if (! rel.empty())
        {
            g.setColour (ui::dim);
            ui::drawSpaced (g, "REL. " + String (rel).toUpperCase(), { keyPanel.getRight() - 18 - 150, keyPanel.getY() + 148, 150, 32 }, 10.0f, 0.2f, Justification::centredRight);
        }
    }

    if (harmOpen && choirOpen)
    {
        g.setColour (ui::dim);
        const int sx = harmPanel.getX() + 250;
        ui::drawSpaced (g, "STYLE", { sx, harmPanel.getY() + 42, 120, 14 }, 9.5f, 0.35f, Justification::centredLeft);
        ui::drawSpaced (g, String (atlas::choirStyleHint (choirStyleBox.getSelectedItemIndex())).toUpperCase(),
                        { sx, harmPanel.getY() + 92, 215, 24 }, 8.5f, 0.12f, Justification::topLeft);
    }

    g.setColour (ui::dim);
    ui::drawSpaced (g, "OUTPUT", { 374 + 12, 92 + 400, 70, 22 }, 10.0f, 0.3f, Justification::centredLeft);
}
