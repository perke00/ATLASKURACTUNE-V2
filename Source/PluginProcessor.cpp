#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::StringArray keyNames { "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };
    const juce::StringArray inputTypes { "Soprano", "Alto / Tenor", "Low Male", "Instrument" };
    // detection range per input type: min Hz, max Hz
    const float inMinHz[] = { 170.0f, 110.0f, 75.0f, 80.0f };
    const float inMaxHz[] = { 1200.0f, 900.0f, 500.0f, 1500.0f };
    const juce::StringArray intervalNames { "Off", "Octave Down", "6th Down", "5th Down", "4th Down", "3rd Down",
                                            "3rd Up", "4th Up", "5th Up", "6th Up", "7th Up", "Octave Up" };
    // scale steps for each interval entry; octaves are handled as +-12 semitones
    const int intervalSteps[] = { 0, -12, -5, -4, -3, -2, 2, 3, 4, 5, 6, 12 };
    const juce::StringArray voiceTypes { "Soprano", "Alto", "Tenor", "Bass" };
    const float voiceFormant[] = { 1.18f, 1.07f, 0.93f, 0.82f };
    const float voicePan[] = { -0.55f, 0.55f, -0.25f, 0.25f };

    juce::String vid (const char* base, int i) { return juce::String (base) + juce::String (i + 1); }
}

juce::AudioProcessorValueTreeState::ParameterLayout AtlasProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::key, 1 }, "Key", keyNames, 0));
    StringArray scales;
    for (auto& s : atlas::scaleDefs()) scales.add (s.name);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::scale, 1 }, "Scale", scales, 1));

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::inType, 1 }, "Input Type", inputTypes, 1));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::retune, 1 }, "Retune Speed",
                                                  NormalisableRange<float> (0.0f, 400.0f, 0.1f, 0.4f), 15.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::humanize, 1 }, "Humanize",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::mode, 1 }, "Mode",
                                                   StringArray { "Classic", "Modern", "Auto" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::amount, 1 }, "Pitch Amount",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::formant, 1 }, "Throat",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::mix, 1 }, "Dry / Wet",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::deesser, 1 }, "De-Esser",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::comp, 1 }, "Compressor",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::eq, 1 }, "EQ",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::lowLat, 1 }, "Low Latency", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::outGain, 1 }, "Output",
                                                  NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::harmOn, 1 }, "Harmony", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::harmMix, 1 }, "Harmony Mix",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::harmSpread, 1 }, "Harmony Spread",
                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f));

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::harmMode, 1 }, "Harmony Mode", StringArray { "Harmony", "Female Choir" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::choirStyle, 1 }, "Choir Style",
                                                   StringArray { atlas::choirStyleNames()[0], atlas::choirStyleNames()[1], atlas::choirStyleNames()[2] }, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::choirSize, 1 }, "Choir Size", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::choirEns, 1 }, "Choir Ensemble", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 55.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::choirVib, 1 }, "Choir Vibrato", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::choirRoom, 1 }, "Choir Room", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::choirDrone, 1 }, "Choir Drone", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f));

    const int defInt[] = { 6, 8, 0, 0 };   // 3rd up, 5th up, off, off
    const int defType[] = { 1, 0, 2, 3 };  // alto, soprano, tenor, bass
    for (int v = 0; v < numVoices; ++v)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { vid ("vint", v), 1 }, "Voice " + String (v + 1) + " Interval", intervalNames, defInt[v]));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { vid ("vtype", v), 1 }, "Voice " + String (v + 1) + " Type", voiceTypes, defType[v]));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { vid ("vlevel", v), 1 }, "Voice " + String (v + 1) + " Level",
                                                      NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));
    }

    const auto mask = atlas::makeMask (1, 0);
    for (int i = 0; i < 12; ++i)
        l.add (std::make_unique<AudioParameterBool> (ParameterID { "note" + String (i), 1 }, "Note " + noteNames[i], mask[(size_t) i]));

    return l;
}

AtlasProcessor::AtlasProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ATLAS", createLayout())
{
    for (auto& g : graph) g.store (0.0f);

    pKey = apvts.getRawParameterValue (ids::key);        pScale = apvts.getRawParameterValue (ids::scale);
    pRetune = apvts.getRawParameterValue (ids::retune);  pHuman = apvts.getRawParameterValue (ids::humanize);
    pMode = apvts.getRawParameterValue (ids::mode);      pAmount = apvts.getRawParameterValue (ids::amount);
    pFormant = apvts.getRawParameterValue (ids::formant);pMix = apvts.getRawParameterValue (ids::mix);
    pDeess = apvts.getRawParameterValue (ids::deesser);  pComp = apvts.getRawParameterValue (ids::comp);
    pEq = apvts.getRawParameterValue (ids::eq);          pLowLat = apvts.getRawParameterValue (ids::lowLat);
    pInType = apvts.getRawParameterValue (ids::inType);
    pOutGain = apvts.getRawParameterValue (ids::outGain);pHarmOn = apvts.getRawParameterValue (ids::harmOn);
    pHarmMix = apvts.getRawParameterValue (ids::harmMix);pHarmSpread = apvts.getRawParameterValue (ids::harmSpread);
    pHarmMode = apvts.getRawParameterValue (ids::harmMode); pChStyle = apvts.getRawParameterValue (ids::choirStyle);
    pChSize = apvts.getRawParameterValue (ids::choirSize);  pChEns = apvts.getRawParameterValue (ids::choirEns);
    pChVib = apvts.getRawParameterValue (ids::choirVib);    pChRoom = apvts.getRawParameterValue (ids::choirRoom);
    pChDrone = apvts.getRawParameterValue (ids::choirDrone);
    for (int i = 0; i < 12; ++i) pNote[(size_t) i] = apvts.getRawParameterValue ("note" + juce::String (i));
    for (int v = 0; v < numVoices; ++v)
    {
        pVInt[(size_t) v] = apvts.getRawParameterValue (vid ("vint", v));
        pVType[(size_t) v] = apvts.getRawParameterValue (vid ("vtype", v));
        pVLevel[(size_t) v] = apvts.getRawParameterValue (vid ("vlevel", v));
    }
}

AtlasProcessor::~AtlasProcessor()
{
}

// --- key/scale -> note toggles ---------------------------------------------------------------
void AtlasProcessor::setParam (const juce::String& id, float plain)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

void AtlasProcessor::applyScaleToNotes()
{
    const auto mask = atlas::makeMask ((int) pScale->load(), (int) pKey->load());
    for (int i = 0; i < 12; ++i) setParam ("note" + juce::String (i), mask[(size_t) i] ? 1.0f : 0.0f);
}

// --- programs ---------------------------------------------------------------------------------
int AtlasProcessor::getNumPrograms() { return 6; }
const juce::String AtlasProcessor::getProgramName (int i)
{
    static const char* names[] = { "Default", "Hard Tune", "Natural Vocal", "Choir Harmony", "Live Low Latency", "Balkan Female Choir" };
    return names[juce::jlimit (0, 5, i)];
}

void AtlasProcessor::setCurrentProgram (int index)
{
    currentProgram = juce::jlimit (0, 5, index);
    setParam (ids::retune, 15); setParam (ids::humanize, 20); setParam (ids::amount, 100); setParam (ids::mode, 0);
    setParam (ids::formant, 50); setParam (ids::mix, 100); setParam (ids::lowLat, 0); setParam (ids::harmOn, 0); setParam (ids::harmMode, 0);
    switch (currentProgram)
    {
        case 1: setParam (ids::retune, 0); setParam (ids::humanize, 0); break;
        case 2: setParam (ids::retune, 70); setParam (ids::humanize, 55); setParam (ids::amount, 85); setParam (ids::mode, 1); break;
        case 3:
            setParam (ids::retune, 25); setParam (ids::harmOn, 1);
            setParam (vid ("vint", 0), 6); setParam (vid ("vtype", 0), 1);
            setParam (vid ("vint", 1), 8); setParam (vid ("vtype", 1), 0);
            setParam (vid ("vint", 2), 5); setParam (vid ("vtype", 2), 2);
            setParam (vid ("vint", 3), 1); setParam (vid ("vtype", 3), 3);
            break;
        case 5:
            setParam (ids::retune, 18); setParam (ids::humanize, 30); setParam (ids::harmOn, 1); setParam (ids::harmMode, 1);
            setParam (ids::choirStyle, 1); setParam (ids::choirSize, 70); setParam (ids::choirEns, 60); setParam (ids::choirVib, 45);
            setParam (ids::choirRoom, 40); setParam (ids::choirDrone, 40); setParam (ids::harmMix, 65);
            break;
        case 4: setParam (ids::lowLat, 1); setParam (ids::retune, 20); setParam (ids::mode, 2); break;
        default: break;
    }
}

// --- state -------------------------------------------------------------------------------------
void AtlasProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void AtlasProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

// --- audio -------------------------------------------------------------------------------------
bool AtlasProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono()) && o == l.getMainInputChannelSet();
}

void AtlasProcessor::configureForMode (bool lowLatency, int inputType)
{
    currentLowLat = lowLatency;
    currentInType = juce::jlimit (0, 3, inputType);
    const float minHz = lowLatency ? juce::jmax (inMinHz[currentInType], 150.0f) : inMinHz[currentInType];
    detector.configure (minHz, inMaxHz[currentInType]);
    const int maxT = (int) std::ceil (sr / minHz);
    lead.configure (maxT);
    for (auto& h : harm) h.configure (maxT);
    hop = lowLatency ? 48 : 64;
    hopCounter = 0; corrector.reset(); T0 = 0.0f;
    voiceActive.fill (false);
    deEsser.prepare (sr);
    setLatencySamples (lead.latency());
}

void AtlasProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    detector.prepare (sr);
    lead.prepare();
    for (auto& h : harm) h.prepare();
    deEsser.prepare (sr);
    compressor.prepare (sr);
    vocalEq.prepare (sr);
    corrector.reset();
    for (int v = 0; v < kVoices; ++v)
    {
        voiceDelay[(size_t) v].prepare ((int) (0.09 * sr));
        singer[(size_t) v].seed (0x9e3779b9u * (uint32_t) (v + 1));
        delayLfo[(size_t) v] = 1.1f * (float) v;
    }
    choirReverb.setSampleRate (sr);
    choirReverb.reset();
    for (auto& a : choirAir) a.highShelf (sr, 5500.0, 2.5);
    wetSm.reset (sr, 0.03);  wetSm.setCurrentAndTargetValue (pMix->load() * 0.01f);
    gainSm.reset (sr, 0.03); gainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutGain->load()));
    configureForMode (pLowLat->load() > 0.5f, (int) pInType->load());
}

void AtlasProcessor::analyse()
{
    float hz = 0.0f, conf = 0.0f;
    const bool found = detector.detect (hz, conf);
    const float dt = (float) hop / (float) sr;

    // allowed notes: held MIDI notes win over the scale
    atlas::CorrParams cp;
    bool anyHeld = false;
    for (int n = 0; n < 128; ++n) if (heldNotes[n] > 0) { cp.mask[(size_t) (n % 12)] = true; anyHeld = true; }
    if (! anyHeld)
        for (int i = 0; i < 12; ++i) cp.mask[(size_t) i] = pNote[(size_t) i]->load() > 0.5f;
    cp.retuneMs = pRetune->load();
    cp.humanize = pHuman->load() * 0.01f;
    cp.amount   = pAmount->load() * 0.01f;
    cp.mode     = (int) pMode->load();

    const auto r = corrector.update (found, hz, cp, dt);

    if (r.voiced)
    {
        T0 = (float) sr / r.freq;
        auto periodFor = [this] (float midi) { return (float) sr / (440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f)); };
        TsynLead = periodFor (r.outMidi);

        const float fb = std::pow (2.0f, (pFormant->load() * 0.01f - 0.5f) * 0.8f);
        formantLead = fb;

        const bool hOn = pHarmOn->load() > 0.5f;
        const bool choirMode = hOn && pHarmMode->load() > 0.5f;
        const float spread = pHarmSpread->load() * 0.01f;
        for (auto& va : voiceActive) va = false;

        if (choirMode)
        {
            const auto* def = atlas::choirStyle ((int) pChStyle->load());
            const float size = pChSize->load() * 0.01f, ens = pChEns->load() * 0.01f, vib = pChVib->load() * 0.01f;
            const int nActive = juce::jlimit (1, atlas::kChoirVoices, 1 + (int) std::lround (size * 4.0f));
            for (int v = 0; v < atlas::kChoirVoices; ++v)
            {
                if (v >= nActive) continue;
                voiceActive[(size_t) v] = true;
                const float off = (float) (atlas::stepInScale (r.target, def[v].steps, cp.mask) - r.target);
                const float cents = def[v].cents * ens + singer[(size_t) v].update (dt, def[v].rate, vib, ens);
                TsynV[(size_t) v] = periodFor (r.outMidi + off + cents * 0.01f);
                formantV[(size_t) v] = juce::jlimit (0.7f, 1.4f, fb * def[v].formant);
                delayLfo[(size_t) v] += 6.2831853f * 0.33f * dt;
                if (delayLfo[(size_t) v] > 6.2831853f) delayLfo[(size_t) v] -= 6.2831853f;
                const float baseMs = 3.0f + def[v].delayMs * ens;
                voiceDelaySamples[(size_t) v] = (float) sr * 0.001f * baseMs * (1.0f + 0.12f * ens * std::sin (delayLfo[(size_t) v]));
            }
            // drone ("ison"): holds the tonic of the key, following the singer's vowels
            if (pChDrone->load() > 0.5f)
            {
                const int root = (int) pKey->load();
                const int d = (((r.target - root) % 12) + 12) % 12;
                const float cents = 2.0f * ens + singer[5].update (dt, 4.8f, vib * 0.3f, ens * 0.5f);
                voiceActive[5] = true;
                TsynV[5] = periodFor ((float) (r.target - d) + cents * 0.01f);
                formantV[5] = juce::jlimit (0.7f, 1.4f, fb * 1.02f);
                voiceDelaySamples[5] = (float) sr * 0.001f * (8.0f + 6.0f * ens);
            }
        }
        else
        {
            for (int v = 0; v < numVoices; ++v)
            {
                const int ii = (int) pVInt[(size_t) v]->load();
                const bool act = hOn && ii > 0 && pVLevel[(size_t) v]->load() > 0.5f;
                voiceActive[(size_t) v] = act;
                if (! act) continue;

                float off;
                if (ii == 1) off = -12.0f; else if (ii == 11) off = 12.0f;
                else off = (float) (atlas::stepInScale (r.target, intervalSteps[ii], cp.mask) - r.target);
                off += ((v % 2) ? 1.0f : -1.0f) * spread * 0.14f * (1.0f + 0.35f * (float) v);   // tiny detune = choir width
                TsynV[(size_t) v] = periodFor (r.outMidi + off);
                formantV[(size_t) v] = juce::jlimit (0.7f, 1.4f, fb * voiceFormant[juce::jlimit (0, 3, (int) pVType[(size_t) v]->load())]);
            }
        }

        detectedMidi.store (r.detMidi);
        targetMidi.store ((float) r.target);
    }
    else if (! r.holding)
    {
        T0 = 0.0f;
        detectedMidi.store (-1.0f); targetMidi.store (-1.0f);
    }

    const int gp = graphPos.load();
    graph[(size_t) gp].store (juce::jlimit (-12.0f, 12.0f, corrector.correction()));
    graphPos.store ((gp + 1) % graphSize);
}

void AtlasProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = numCh; c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, n);
    if (numCh < 1 || n < 1) return;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) heldNotes[msg.getNoteNumber() & 127] = 1;
        else if (msg.isNoteOff()) heldNotes[msg.getNoteNumber() & 127] = 0;
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) std::fill (std::begin (heldNotes), std::end (heldNotes), 0);
    }

    const bool wantLow = pLowLat->load() > 0.5f;
    const int wantType = juce::jlimit (0, 3, (int) pInType->load());
    if (wantLow != currentLowLat || wantType != currentInType) configureForMode (wantLow, wantType);

    wetSm.setTargetValue (pMix->load() * 0.01f);
    const float deess = pDeess->load() * 0.01f, comp = pComp->load() * 0.01f, eq = pEq->load() * 0.01f;
    gainSm.setTargetValue (juce::Decibels::decibelsToGain (pOutGain->load()));
    const bool hOn = pHarmOn->load() > 0.5f;
    const bool choirMode = hOn && pHarmMode->load() > 0.5f;
    const float hMix = pHarmMix->load() * 0.01f;
    vocalEq.update (eq);

    // per-voice mix gains
    if (choirMode)
    {
        const auto* def = atlas::choirStyle ((int) pChStyle->load());
        const int nActive = juce::jlimit (1, atlas::kChoirVoices, 1 + (int) std::lround (pChSize->load() * 0.04f));
        const float norm = 1.0f / std::sqrt ((float) nActive);
        const float widthF = juce::jlimit (0.3f, 1.0f, 0.4f + 0.9f * pHarmSpread->load() * 0.01f);
        for (int v = 0; v < kVoices; ++v)
        {
            float pan = 0.0f, lvl = 0.0f;
            if (v < atlas::kChoirVoices) { pan = def[v].pan * widthF; lvl = norm * 0.55f; }
            else { pan = 0.0f; lvl = pChDrone->load() * 0.01f * 0.5f; }
            const float ang = (juce::jlimit (-1.0f, 1.0f, pan) * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
            voiceGainL[(size_t) v] = std::cos (ang) * lvl * hMix * 1.414f;
            voiceGainR[(size_t) v] = std::sin (ang) * lvl * hMix * 1.414f;
        }
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.62f; rp.damping = 0.45f; rp.width = 1.0f; rp.freezeMode = 0.0f;
        rp.dryLevel = 1.0f; rp.wetLevel = pChRoom->load() * 0.01f * 0.30f;
        choirReverb.setParameters (rp);
        if (! choirWasOn) choirReverb.reset();
    }
    else
    {
        for (int v = 0; v < kVoices; ++v)
        {
            const float pan = v < numVoices ? voicePan[v] : 0.0f;
            const float ang = (pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
            const float lvl = v < numVoices ? pVLevel[(size_t) v]->load() * 0.01f * hMix * 0.8f : 0.0f;
            voiceGainL[(size_t) v] = std::cos (ang) * lvl * 1.414f;
            voiceGainR[(size_t) v] = std::sin (ang) * lvl * 1.414f;
        }
    }
    choirWasOn = choirMode;

    auto* l = buffer.getWritePointer (0);
    auto* r = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    float inPeak = 0.0f, outPeak = 0.0f;

    constexpr int chunk = 128;
    float leadBuf[chunk], dryBuf[chunk], hlBuf[chunk], hrBuf[chunk];

    for (int start = 0; start < n; start += chunk)
    {
        const int m = juce::jmin (chunk, n - start);

        // pass 1: pitch analysis, lead + harmony / choir voices
        for (int k = 0; k < m; ++k)
        {
            const int i = start + k;
            float in = r ? 0.5f * (l[i] + r[i]) : l[i];
            if (! std::isfinite (in)) in = 0.0f;
            inPeak = juce::jmax (inPeak, std::fabs (in));

            detector.push (in);
            if (++hopCounter >= hop) { hopCounter = 0; analyse(); }

            const float x = deEsser.process (in, deess);
            leadBuf[k] = lead.process (x, T0, TsynLead, formantLead, true, true);
            dryBuf[k] = lead.dryDelayed();

            float hl = 0.0f, hr = 0.0f;
            for (int v = 0; v < kVoices; ++v)
            {
                const bool act = hOn && voiceActive[(size_t) v];
                float y = harm[(size_t) v].process (x, T0, TsynV[(size_t) v], formantV[(size_t) v] > 0.0f ? formantV[(size_t) v] : 1.0f, false, act);
                if (choirMode) y = voiceDelay[(size_t) v].process (y, voiceDelaySamples[(size_t) v]);
                hl += y * voiceGainL[(size_t) v];
                hr += y * voiceGainR[(size_t) v];
            }
            if (choirMode) { hl = choirAir[0].process (hl); hr = choirAir[1].process (hr); }
            hlBuf[k] = hl; hrBuf[k] = hr;
        }

        // choir room (reverb on the choir bus only)
        if (choirMode && pChRoom->load() > 0.5f)
        {
            // reverb adds its own wet on top of the (dry = 1) choir signal
            choirReverb.processStereo (hlBuf, hrBuf, m);
        }

        // pass 2: dry/wet, vocal chain, output
        for (int k = 0; k < m; ++k)
        {
            const int i = start + k;
            const float wet = wetSm.getNextValue(), outGain = gainSm.getNextValue();
            float outL = dryBuf[k] * (1.0f - wet) + (leadBuf[k] + hlBuf[k]) * wet;
            float outR = dryBuf[k] * (1.0f - wet) + (leadBuf[k] + hrBuf[k]) * wet;

            outL = vocalEq.process (0, outL, eq);
            outR = vocalEq.process (1, outR, eq);
            compressor.process (outL, outR, comp);
            outL *= outGain; outR *= outGain;

            if (! std::isfinite (outL)) outL = 0.0f;
            if (! std::isfinite (outR)) outR = 0.0f;
            outPeak = juce::jmax (outPeak, std::fabs (outL), std::fabs (outR));
            l[i] = outL;
            if (r) r[i] = outR;
        }
    }

    const float inDb = juce::Decibels::gainToDecibels (inPeak, -100.0f), outDb = juce::Decibels::gainToDecibels (outPeak, -100.0f);
    inLevelDb.store (juce::jmax (inDb, inLevelDb.load() - 1.5f));
    outLevelDb.store (juce::jmax (outDb, outLevelDb.load() - 1.5f));
}

juce::AudioProcessorEditor* AtlasProcessor::createEditor() { return new AtlasEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AtlasProcessor(); }
