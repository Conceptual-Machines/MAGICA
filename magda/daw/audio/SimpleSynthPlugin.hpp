#pragma once

#include <tracktion_engine/tracktion_engine.h>

namespace magda::daw::audio {

namespace te = tracktion::engine;

//==============================================================================
/**
 * @brief Simple synth sound - applies to all notes and channels
 */
struct SimpleSynthSound : public juce::SynthesiserSound {
    bool appliesToNote(int) override {
        return true;
    }
    bool appliesToChannel(int) override {
        return true;
    }
};

//==============================================================================
/**
 * @brief Synth voice with sine/noise oscillator and ADSR envelope
 */
class SimpleSynthVoice : public juce::SynthesiserVoice {
  public:
    enum class Waveform { Sine = 0, Noise = 1 };

    SimpleSynthVoice();

    void setWaveform(Waveform wf) {
        waveform = wf;
    }
    void setADSR(float attack, float decay, float sustain, float release);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample,
                         int numSamples) override;

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

  private:
    Waveform waveform = Waveform::Sine;

    // Sine oscillator state
    double currentAngle = 0.0;
    double angleDelta = 0.0;

    // Noise generator
    juce::Random random;

    double level = 0.0;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
};

//==============================================================================
/**
 * @brief Simple synthesizer plugin for Tracktion Engine
 *
 * MIDI-triggered synth with:
 * - Sine or noise waveform
 * - ADSR envelope
 * - Level control
 * - Transport sync support (via external MIDI triggering)
 */
class SimpleSynthPlugin : public te::Plugin {
  public:
    SimpleSynthPlugin(te::PluginCreationInfo);
    ~SimpleSynthPlugin() override;

    //==============================================================================
    static const char* getPluginName() {
        return "Simple Synth";
    }
    static const char* xmlTypeName;

    juce::String getName() const override {
        return getPluginName();
    }
    juce::String getPluginType() override {
        return xmlTypeName;
    }
    juce::String getShortName(int) override {
        return "SimpleSynth";
    }
    juce::String getSelectableDescription() override {
        return getName();
    }

    //==============================================================================
    void initialise(const te::PluginInitialisationInfo&) override;
    void deinitialise() override;
    void reset() override;

    void applyToBuffer(const te::PluginRenderContext&) override;

    //==============================================================================
    bool takesMidiInput() override {
        return true;
    }
    bool takesAudioInput() override {
        return false;
    }
    bool isSynth() override {
        return true;
    }
    bool producesAudioWhenNoAudioInput() override {
        return true;
    }
    double getTailLength() const override;

    void restorePluginStateFromValueTree(const juce::ValueTree&) override;

    //==============================================================================
    // Parameters
    juce::CachedValue<float> waveformValue, levelValue;
    juce::CachedValue<float> attackValue, decayValue, sustainValue, releaseValue;

    te::AutomatableParameter::Ptr waveformParam, levelParam;
    te::AutomatableParameter::Ptr attackParam, decayParam, sustainParam, releaseParam;

  private:
    //==============================================================================
    juce::Synthesiser synthesiser;
    double sampleRate = 44100.0;
    int numVoices = 8;  // Polyphonic

    void updateVoiceParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthPlugin)
};

}  // namespace magda::daw::audio
