#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "DSP/PitchDetector.h"
#include "DSP/PsolaShifter.h"
#include "DSP/PitchCorrector.h"
#include "DSP/Scales.h"
#include "DSP/VocalFx.h"
#include "DSP/ChoirEngine.h"

namespace ids
{
    inline const char* key = "key";            inline const char* scale = "scale";
    inline const char* retune = "retune";      inline const char* humanize = "humanize";
    inline const char* mode = "mode";          inline const char* amount = "amount";
    inline const char* formant = "formant";    inline const char* mix = "mix";
    inline const char* deesser = "deesser";    inline const char* comp = "comp";
    inline const char* eq = "eq";              inline const char* lowLat = "lowlat";
    inline const char* outGain = "outgain";
    inline const char* harmOn = "harmon";      inline const char* harmMix = "harmmix";
    inline const char* harmSpread = "harmspread";
    inline const char* inType = "intype";
    inline const char* harmMode = "harmmode";   inline const char* choirStyle = "chstyle";
    inline const char* choirSize = "chsize";    inline const char* choirEns = "chens";
    inline const char* choirVib = "chvib";      inline const char* choirRoom = "chroom";
    inline const char* choirDrone = "chdrone";
}

class AtlasProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numVoices = 4;                     // harmony strips shown in the UI
    static constexpr int kVoices = 6;                       // internal shifters (choir uses 5 singers + drone)
    static constexpr int graphSize = 240;

    AtlasProcessor();
    ~AtlasProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Atlas Tune Pro"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // copies the selected key + scale into the 12 note toggles (called by the editor when key/scale change)
    void applyScaleToNotes();

    // meters / display data (read by the editor)
    std::atomic<float> inLevelDb { -100.0f }, outLevelDb { -100.0f };
    std::atomic<float> detectedMidi { -1.0f }, targetMidi { -1.0f };
    std::array<std::atomic<float>, graphSize> graph;
    std::atomic<int> graphPos { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float plainValue);
    void configureForMode (bool lowLatency, int inputType);
    void analyse();

    // DSP
    atlas::PitchDetector detector;
    atlas::PitchCorrector corrector;
    atlas::PsolaShifter lead;
    std::array<atlas::PsolaShifter, kVoices> harm;
    std::array<atlas::DelayLine, kVoices> voiceDelay;
    std::array<atlas::SingerDrift, kVoices> singer;
    std::array<float, kVoices> voiceDelaySamples {}, delayLfo {};
    juce::Reverb choirReverb;
    atlas::Biquad choirAir[2];
    bool choirWasOn = false;
    atlas::DeEsser deEsser;
    atlas::VocalCompressor compressor;
    atlas::VocalEq vocalEq;

    double sr = 44100.0;
    int hop = 128, hopCounter = 0;
    bool currentLowLat = false;
    int currentInType = 1;
    juce::SmoothedValue<float> wetSm, gainSm;
    int currentProgram = 0;

    // pitch state
    float T0 = 0.0f, TsynLead = 0.0f, formantLead = 1.0f;
    std::array<float, kVoices> TsynV {}, formantV {};
    std::array<bool, kVoices> voiceActive {};
    std::array<float, kVoices> voiceGainL {}, voiceGainR {};
    int heldNotes[128] {};

    // cached parameter pointers
    std::atomic<float> *pKey, *pScale, *pRetune, *pHuman, *pMode, *pAmount, *pFormant, *pMix, *pDeess, *pComp, *pEq,
        *pLowLat, *pInType, *pOutGain, *pHarmOn, *pHarmMix, *pHarmSpread,
        *pHarmMode, *pChStyle, *pChSize, *pChEns, *pChVib, *pChRoom, *pChDrone;
    std::array<std::atomic<float>*, 12> pNote {};
    std::array<std::atomic<float>*, numVoices> pVInt {}, pVType {}, pVLevel {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtlasProcessor)
};
