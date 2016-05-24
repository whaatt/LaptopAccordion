/**
 * File: mapper.cpp
 * Author: Sanjay Kannan
 * ---------------------
 * Maps combinations of mode mappings, key
 * presses, and scales to a MIDI pitch
 * for a given channel.
 */

#include "mapper.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

/**
 * Function: getNote
 * -----------------
 * Based on the current presets, get
 * a MIDI pitch from the pressed key.
 */
int Mapper::getNote(int key) {
  vector<int>& scaleNotes = scaleMap[scales[scaleIndex]];
  vector<int>& modeIndices = modeMap[modes[modeIndex]];
  int keyBase = keyMap[keys[keyIndex]];

  vector<int> notesMIDI = vector<int>(30);
  int scaleSize = scaleNotes.size();

  for (int i = -10; i < 20; i += 1) // keyBase is always the tenth note
    notesMIDI[i + 10] = keyBase + 12 * (i / scaleSize - (i < 0 && (i % scaleSize)))
      + scaleNotes[i % scaleSize + ((i < 0 && (i % scaleSize)) ? scaleSize : 0)];

  // finally find the keyboard location and map position to note
  int modePos = string("qwertyuiopasdfghjkl;zxcvbnm,./").find(key);
  int outputNote = notesMIDI[modeIndices[modePos]];

  if (outputNote >= 0 && outputNote <= 127) return outputNote;
  return (outputNote < 0) ? 0 : 127; // saturated math
}

/**
 * Function: getPosition
 * ---------------------
 * Get note position based on a
 * mapping of keyboard to scale.
 */
int Mapper::getPosition(int key) {
  // here we just care about which scale index we are playing
  int modePos = string("qwertyuiopasdfghjkl;zxcvbnm,./").find(key);
  return modeMap[modes[modeIndex]][modePos]; // always out of 30
}

/**
 * Function: init
 * --------------
 * Load predefined scales and
 * keyboard maps from file. Does
 * not do much error checking since
 * this is sort of an internal
 * component of the app.
 */
bool Mapper::init(const string scaleFileName, const string modeFileName) {
  keyMap.clear();
  modeMap.clear();
  scaleMap.clear();
  keys.clear();
  modes.clear();
  scales.clear();

  // TODO: maybe something to support displaying and selecting enharmonic notes
  string keysArray[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

  // build map from note to MIDI note
  for (int i = 0; i < 12; i += 1) {
    keys.push_back(keysArray[i]);
    keyMap[keysArray[i]] = i + 60;
  }

  string line; // for parsing by each line
  ifstream scaleFile(scaleFileName.c_str());
  // read in scale to note mapping
  while (getline(scaleFile, line)) {
    istringstream iSS(line);
    string scaleName;
    iSS >> scaleName;

    // treat scales like Harmonic_Minor as Harmonic Minor
    replace(scaleName.begin(), scaleName.end(), '_', ' ');
    scaleMap[scaleName] = vector<int>();
    scales.push_back(scaleName);

    int relativeNote;
    while (iSS >> relativeNote) 
      // read in the scale relative note positions
      scaleMap[scaleName].push_back(relativeNote);
  }

  ifstream modeFile(modeFileName.c_str());
  // read in button to index mapping
  while (getline(modeFile, line)) {
    istringstream iSS(line);
    string modeName;
    iSS >> modeName;

    // treat modes like Percussion_Mode as Percussion Mode
    replace(modeName.begin(), modeName.end(), '_', ' ');
    modeMap[modeName] = vector<int>();
    modes.push_back(modeName);

    int positionIndex;
    while (iSS >> positionIndex) 
      // read in the mode button position indices
      modeMap[modeName].push_back(positionIndex);
  }

  if (scales.size() == 0) return false;
  if (keys.size() == 0) return false;
  if (modes.size() == 0) return false;

  // initialize mapping
  initialized = true;
  setScaleIndex(0);
  setKeyIndex(0);
  setModeIndex(0);
  return true;
}

/**
 * Function: getScales
 * -------------------
 * Get a list of scales to be
 * used in a user interface.
 */
const vector<string>& Mapper::getScales() {
  // just an accessor really
  return scales;
}

/**
 * Function: getModes
 * ------------------
 * Get a list of modes to be
 * used in a user interface.
 */
const vector<string>& Mapper::getModes() {
  // just an accessor really
  return modes;
}

/**
 * Function: getKeys
 * -----------------
 * Get a list of keys to be
 * used in a user interface.
 */
const vector<string>& Mapper::getKeys() {
  // just an accessor really
  return keys;
}

/**
 * Function: setScaleIndex
 * -----------------------
 * Set the current scale index
 * to be used when mapping.
 */
bool Mapper::setScaleIndex(int index) {
  if (!initialized) return false;
  scaleIndex = index;
  return true;
}

/**
 * Function: setModeIndex
 * -----------------------
 * Set the current mode index
 * to be used when mapping.
 */
bool Mapper::setModeIndex(int index) {
  if (!initialized) return false;
  modeIndex = index;
  return true;
}

/**
 * Function: setKeyIndex
 * -----------------------
 * Set the current key index
 * to be used when mapping.
 */
bool Mapper::setKeyIndex(int index) {
  if (!initialized) return false;
  keyIndex = index;
  return true;
}
