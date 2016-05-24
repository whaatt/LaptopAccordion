/**
 * File: synthesizer.cpp
 * Author: Sanjay Kannan
 * ---------------------
 * Adapts YFluidSynth by Ge Wang
 * for use in OpenFrameworks apps.
 */

#include "synthesizer.h"
#include <iostream>
using namespace std;

/**
 * Constructor: Synthesizer
 * ------------------------
 * Sets FluidSynth objects to NULL.
 */
Synthesizer::Synthesizer()
  : settings(NULL), synth(NULL), driver(NULL) {}

/**
 * Destructor: Synthesizer
 * -----------------------
 * Cleans up FluidSynth objects.
 */
Synthesizer::~Synthesizer() {
  // lock synth
  synthLock.lock();

  // clean up FluidSynth objects
  if (synth) delete_fluid_synth(synth);
  if (settings) delete_fluid_settings(settings);
  if (driver) delete_fluid_audio_driver(driver);

  synth = NULL;
  settings = NULL;
  driver = NULL;

  // unlock synth
  synthLock.unlock();
}

/**
 * Function: init
 * --------------
 * Sets synthesizer sampling rate
 * and max polyphony voices.
 */
bool Synthesizer::init(int rate, int polyphony, double gain, bool live) {
  if (synth != NULL) {
    // avoid potential reinitialization of synth
    cerr << "Synthesizer already initialized." << endl;
    return false;
  }

  // lock synth
  synthLock.lock();

  // TODO: add a non-verbose mode to constructor
  cerr << "Initializing synthesizer object." << endl;

  // instantiate settings
  settings = new_fluid_settings();
  // set sample rate in fluidsynth settings
  fluid_settings_setnum(settings, (char*) "synth.sample-rate", (double) rate);

  // set default gain in fluidsynth settings
  fluid_settings_setnum(settings, (char*) "synth.gain", (double) gain);

  // set polyphony and bound
  if (polyphony <= 0) polyphony = 1;
  else if (polyphony > 256) polyphony = 256;
  fluid_settings_setint(settings, (char*) "synth.polyphony", polyphony);

  // instantiate the synth
  synth = new_fluid_synth(settings);

  if (live) { // go ahead and play FluidSynth live if live mode has been set
    char* defaultDriver = fluid_settings_getstr_default(settings, "audio.driver");
    fluid_settings_setstr(settings, "audio.driver", defaultDriver);
    driver = new_fluid_audio_driver(settings, synth);
  }

  // unlock synth
  synthLock.unlock();
  return synth != NULL;
}

/**
* Function: gain
* --------------
* Sets synthesizer sampling rate
* and max polyphony voices.
*/
void Synthesizer::setGain(double gain) {
  if (synth == NULL) return; // sanity

  synthLock.lock(); // lock synth
  // set default gain in fluidsynth settings
  fluid_settings_setnum(settings, (char*) "synth.gain", (double) gain);
  synthLock.unlock(); // unlock synth
}

/**
 * Function: load
 * --------------
 * Loads a SoundFont file into the
 * synthesizer and overwrite presets.
 */
bool Synthesizer::load(const char* path) {
  if(synth == NULL) return false;

  // lock synth
  synthLock.lock();

  // load soundfont and catch any errors in doing so
  if (fluid_synth_sfload(synth, path, true) == -1) {
    cerr << "Cannot load font file: " << path << "." << endl;

    // unlock synth
    synthLock.unlock();
    return false;
  }

  // unlock synth
  synthLock.unlock();
  return true;
}

/**
 * Function: setInstrument
 * -----------------------
 * Changes channel program, which
 * is basically setting an instrument.
 */
void Synthesizer::setInstrument(int channel, int program) {
  if (synth == NULL) return;
  if (program < 0 || program > 127) return;

  synthLock.lock(); // lock synth
  fluid_synth_program_change(synth, channel, program);
  synthLock.unlock(); // unlock synth
}

/**
 * Function: controlChange
 * -----------------------
 * Sends a control message.
 */
void Synthesizer::controlChange(int channel, int dataTwo, int dataThree) {
  if (synth == NULL) return;
  if (dataTwo < 0 || dataTwo > 127) return;

  synthLock.lock(); // lock synth
  fluid_synth_cc(synth, channel, dataTwo, dataThree);
  synthLock.unlock(); // unlock synth
}

/**
 * Function: noteOn
 * ----------------
 * Turns a note on for a channel
 * at a given pitch and velocity.
 */
void Synthesizer::noteOn(int channel, float pitch, int velocity) {
  // sanity check on synth
  if (synth == NULL) return;

  // get an integer pitch
  // int pitchI = (int) (pitch + .5f);
  // find the bend difference
  // float diff = pitch - pitchI;

  // lock synth
  synthLock.lock();
  
  // if bend needed
  // if (diff != 0)
    // apply the necessary bend to the note [TODO: does this need a reset]
    // fluid_synth_pitch_bend(synth, channel, (int) (8192 + diff * 8191));

  // sound note with the given velocity
  fluid_synth_noteon(synth, channel, pitch, velocity);

  // unlock synth
  synthLock.unlock();
}

/**
 * Function: pitchBend
 * -------------------
 * Bends a note corresponding
 * to a given pitch difference.
 */
void Synthesizer::pitchBend(int channel, float pitchDiff) {
  // sanity check on synth
  if (synth == NULL) return;

  // lock synth
  synthLock.lock();

  // pitch bend [TODO: figure out exactly what pitchDiff means]
  fluid_synth_pitch_bend(synth, channel, (int) (8192 + pitchDiff * 8191));

  // unlock synth
  synthLock.unlock();
}

/**
 * Function: noteOff
 * -----------------
 * Turns a particular note
 * off on a specific channel.
 */
void Synthesizer::noteOff(int channel, int pitch) {
  // sanity check on synth
  if (synth == NULL) return;

  synthLock.lock(); // lock synth
  fluid_synth_noteoff(synth, channel, pitch);
  synthLock.unlock(); // unlock synth
}

/**
 * Function: allNotesOff
 * ---------------------
 * Stops notes on a channel.
 */
void Synthesizer::allNotesOff(int channel) {
  // send all notes off control message
  controlChange(channel, 120, 0x7B);
}

/**
 * Function: synthesize
 * --------------------
 * Synthesizes a stereo buffer of
 * samples for use external to synth.
 */
bool Synthesizer::synthesize(float* buffer, unsigned int numFrames) {
  // sanity check on synth
  if (synth == NULL) return false;

  synthLock.lock(); // lock synth
  int retVal = fluid_synth_write_float(synth, numFrames, buffer, 0, 2, buffer, 1, 2);
  synthLock.unlock(); // unlock synth

  // return success
  return retVal == 0;
}
