/**
 * File: synthesizer.h
 * Author: Sanjay Kannan
 * ---------------------
 * Adapts YFluidSynth by Ge Wang
 * for use in OpenFrameworks apps.
 */

#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include <fluidsynth.h>
#include "ofMain.h"

// plays MIDI audio
class Synthesizer {
  public:
    Synthesizer();
    ~Synthesizer();

    // initialize synthesizer and load soundfont
    bool init(int rate, int polyphony, double gain, bool live);
    bool load(const char* path);

    // program change [set instrument]
    void setInstrument(int channel, int program);
    // control change [set global gain]
    void setGain(double gain);
    // control change [send control message]
    void controlChange(int channel, int dataTwo, int dataThree);
    // turn on a particular note on a particular channel
    void noteOn(int channel, float pitch, int velocity);
    // pitch bend an entire channel
    void pitchBend(int channel, float pitchDiff);
    // turn off a particular note on a channel
    void noteOff(int channel, int pitch);
    // turn off all notes on channel
    void allNotesOff(int channel);
    // synthesize stereo buffer of samples
    bool synthesize(float* buffer, unsigned int numFrames);

    // TODO: maybe make an accessor
    fluid_synth_t* synth;
    ofMutex synthLock;

  protected:
    fluid_settings_t* settings;
    fluid_audio_driver_t* driver;
};

// guard
#endif
