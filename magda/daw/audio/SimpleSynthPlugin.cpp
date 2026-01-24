#include "SimpleSynthPlugin.hpp"

namespace magda::daw::audio {

namespace te = tracktion::engine;

const char* SimpleSynthPlugin::xmlTypeName = "simplesynth";

//==============================================================================
// SimpleSynthVoice Implementation
//==============================================================================

SimpleSynthVoice::SimpleSynthVoice() {
    adsrParams.attack = 0.01f;
    adsrParams.decay = 0.1f;
    adsrParams.sustain = 0.8f;
    adsrParams.release = 0.2f;
    adsr.setParameters(adsrParams);
}

void SimpleSynthVoice::setADSR(float attack, float decay, float sustain, float release) {
    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    adsr.setParameters(adsrParams);
}

bool SimpleSynthVoice::canPlaySound(juce::SynthesiserSound* sound) {
    return dynamic_cast<SimpleSynthSound*>(sound) != nullptr;
}

void SimpleSynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*,
                                 int /*currentPitchWheelPosition*/) {
    currentAngle = 0.0;
    level = velocity * 0.15;
    angleDelta = juce::MathConstants<double>::twoPi *
                 juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) / getSampleRate();

    adsr.setSampleRate(getSampleRate());
    adsr.noteOn();
}

void SimpleSynthVoice::stopNote(float /*velocity*/, bool allowTailOff) {
    if (allowTailOff) {
        adsr.noteOff();
    } else {
        adsr.reset();
        clearCurrentNote();
    }
}

void SimpleSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample,
                                       int numSamples) {
    adsr.setSampleRate(getSampleRate());

    for (int i = 0; i < numSamples; ++i) {
        float env = adsr.getNextSample();
        float sample = 0.0f;

        if (waveform == Waveform::Sine) {
            // Sine wave
            sample = static_cast<float>(std::sin(currentAngle) * level * env);
            currentAngle += angleDelta;
        } else {
            // White noise
            sample = (random.nextFloat() * 2.0f - 1.0f) * static_cast<float>(level * env);
        }

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addSample(channel, startSample, sample);

        ++startSample;
    }

    if (!adsr.isActive())
        clearCurrentNote();
}

//==============================================================================
// SimpleSynthPlugin Implementation
//==============================================================================

SimpleSynthPlugin::SimpleSynthPlugin(te::PluginCreationInfo info) : Plugin(info) {
    auto um = getUndoManager();

    // Waveform: 0 = Sine, 1 = Noise
    waveformValue.referTo(state, te::IDs::waveform, um, 0.0f);
    waveformParam = addParam(
        "waveform", "Waveform", {0.0f, 1.0f}, [](float v) { return v < 0.5f ? "Sine" : "Noise"; },
        [](const juce::String& s) { return s.equalsIgnoreCase("Noise") ? 1.0f : 0.0f; });

    // Level (dB)
    levelValue.referTo(state, te::IDs::level, um, -12.0f);
    levelParam = addParam("level", "Level", {-60.0f, 0.0f, -12.0f, 4.0f}, "dB");

    // ADSR
    attackValue.referTo(state, te::IDs::attack, um, 0.01f);
    attackParam = addParam("attack", "Attack", {0.001f, 5.0f, 0.01f}, "s");

    decayValue.referTo(state, te::IDs::decay, um, 0.1f);
    decayParam = addParam("decay", "Decay", {0.001f, 5.0f, 0.1f}, "s");

    sustainValue.referTo(state, te::IDs::sustain, um, 0.8f);
    sustainParam = addParam("sustain", "Sustain", {0.0f, 1.0f});

    releaseValue.referTo(state, te::IDs::release, um, 0.2f);
    releaseParam = addParam("release", "Release", {0.001f, 10.0f, 0.2f}, "s");

    // Initialize synthesiser
    synthesiser.clearVoices();
    synthesiser.clearSounds();

    synthesiser.addSound(new SimpleSynthSound());

    for (int i = 0; i < numVoices; ++i)
        synthesiser.addVoice(new SimpleSynthVoice());
}

SimpleSynthPlugin::~SimpleSynthPlugin() {
    notifyListenersOfDeletion();
}

void SimpleSynthPlugin::initialise(const te::PluginInitialisationInfo& info) {
    sampleRate = info.sampleRate;
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void SimpleSynthPlugin::deinitialise() {
    synthesiser.allNotesOff(0, false);
}

void SimpleSynthPlugin::reset() {
    synthesiser.allNotesOff(0, false);
}

double SimpleSynthPlugin::getTailLength() const {
    return releaseValue.get();
}

void SimpleSynthPlugin::updateVoiceParameters() {
    float attack = juce::jlimit(0.001f, 5.0f, attackParam->getCurrentValue());
    float decay = juce::jlimit(0.001f, 5.0f, decayParam->getCurrentValue());
    float sustain = juce::jlimit(0.0f, 1.0f, sustainParam->getCurrentValue());
    float release = juce::jlimit(0.001f, 10.0f, releaseParam->getCurrentValue());

    SimpleSynthVoice::Waveform wf = waveformParam->getCurrentValue() < 0.5f
                                        ? SimpleSynthVoice::Waveform::Sine
                                        : SimpleSynthVoice::Waveform::Noise;

    // Update all voices
    for (int i = 0; i < synthesiser.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<SimpleSynthVoice*>(synthesiser.getVoice(i))) {
            voice->setWaveform(wf);
            voice->setADSR(attack, decay, sustain, release);
        }
    }
}

void SimpleSynthPlugin::applyToBuffer(const te::PluginRenderContext& fc) {
    if (fc.destBuffer == nullptr)
        return;

    // Update voice parameters from current parameter values
    updateVoiceParameters();

    // Get level in linear from dB
    float levelDb = levelParam->getCurrentValue();
    float levelLinear = juce::Decibels::decibelsToGain(levelDb);

    // Render MIDI to audio
    if (fc.bufferForMidiMessages && !fc.bufferForMidiMessages->isEmpty()) {
        synthesiser.renderNextBlock(*fc.destBuffer, *fc.bufferForMidiMessages, fc.bufferStartSample,
                                    fc.bufferNumSamples);
    } else {
        // No MIDI - just advance time
        juce::MidiBuffer emptyMidi;
        synthesiser.renderNextBlock(*fc.destBuffer, emptyMidi, fc.bufferStartSample,
                                    fc.bufferNumSamples);
    }

    // Apply level
    fc.destBuffer->applyGain(fc.bufferStartSample, fc.bufferNumSamples, levelLinear);
}

void SimpleSynthPlugin::restorePluginStateFromValueTree(const juce::ValueTree& v) {
    te::copyPropertiesToCachedValues(v, waveformValue, levelValue, attackValue, decayValue,
                                     sustainValue, releaseValue);

    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();
}

}  // namespace magda::daw::audio
