#pragma once
#include "PluginProcessor.h"

namespace ui
{
    const juce::Colour bg       { 0xff09090b };
    const juce::Colour panel    { 0xff101013 };
    const juce::Colour border   { 0xff2a2a2f };
    const juce::Colour text     { 0xffe8e8ea };
    const juce::Colour dim      { 0xff8c8c93 };
    const juce::Colour bright   { 0xfff4f4f6 };

    void drawSpaced (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, float size, float kerning,
                     juce::Justification j, bool bold = false);
    void drawLogo (juce::Graphics& g, juce::Rectangle<float> r);
    // embedded Montserrat: weight 0 = light, 1 = medium, 2 = semibold
    juce::Font font (float size, int weight = 1);
}

class AtlasLNF : public juce::LookAndFeel_V4
{
public:
    AtlasLNF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui::font (14.0f, 1); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return ui::font (12.5f, 1); }
};

// rotary knob + caption + value read-out
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title, const juce::String& unit, bool bipolar = false);
    void resized() override;
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
private:
    void refresh();
    juce::Label name, value;
    juce::String unitText;
    bool isBipolar = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class CenterDial : public juce::Component
{
public:
    explicit CenterDial (AtlasProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick();
private:
    AtlasProcessor& proc;
    int lastTarget = -1;
    std::vector<int> history;
};

class PitchGraph : public juce::Component
{
public:
    explicit PitchGraph (AtlasProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    AtlasProcessor& proc;
};

class PianoComponent : public juce::Component
{
public:
    explicit PianoComponent (AtlasProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    int noteAt (juce::Point<float>) const;
    AtlasProcessor& proc;
    std::array<std::atomic<float>*, 12> noteParams {};
    static constexpr int firstNote = 36, whiteCount = 35, totalNotes = 60;
};

class AtlasEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AtlasEditor (AtlasProcessor&);
    ~AtlasEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void panel (juce::Graphics&, juce::Rectangle<int> r, const juce::String& title);

    AtlasProcessor& proc;
    AtlasLNF lnf;
    juce::Image background;

    juce::ComboBox presetBox, scaleBox, keyBox, inputBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAtt, keyAtt, inputAtt;
    std::unique_ptr<juce::ParameterAttachment> modeAtt, harmModeAtt;
    std::array<juce::TextButton, 2> harmModeBtns;
    juce::ComboBox choirStyleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choirStyleAtt;
    Knob chSizeK, chEnsK, chVibK, chRoomK, chDroneK;
    bool choirOpen = false;
    std::array<juce::TextButton, 3> modeBtns;
    juce::TextButton lowLatBtn { "Low Latency" }, harmBtn { "ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lowLatAtt, harmAtt;

    Knob retuneK, humanK, amountK, formantK, mixK, deessK, compK, eqK, hMixK, hSpreadK;
    CenterDial dial;
    PitchGraph graph;
    PianoComponent piano;
    juce::Slider outSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAtt;

    struct Strip
    {
        juce::ComboBox interval, type;
        juce::Slider level { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> iAtt, tAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lAtt;
        juce::Rectangle<int> area;
    };
    std::array<Strip, AtlasProcessor::numVoices> strips;

    void updateHarmonyLayout();
    bool harmOpen = false;
    juce::Rectangle<int> keyPanel, tunePanel, modePanel, pitchPanel, vocalPanel, harmPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtlasEditor)
};
