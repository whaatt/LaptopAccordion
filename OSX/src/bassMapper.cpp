/**
* File: bassMapper.cpp
* Author: Sanjay Kannan
* ---------------------
* Maps bass notes to multiple
* MIDI pitches for a channel.
*/

#include "bassMapper.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

/**
* Function: getNotes
* ------------------
* Based on the current presets, get
* MIDI pitches from the pressed key.
*/
vector<int> BassMapper::getNotes(int key) {
  vector<int> keyNotes = noteMaps[key];
  int keyBase = keyMap[keys[keyIndex]];

  // add base note [no pun intended] to each note
  for (size_t i = 0; i < keyNotes.size(); i += 1) {
    keyNotes[i] += keyBase;

    // clamp note to range
    if (keyNotes[i] < 0)
      keyNotes[i] = 0;
    else if (keyNotes[i] > 127)
      keyNotes[i] = 127;
  }

  // all notes played by caller
  return keyNotes;
}

/**
* Function: init
* --------------
* Load predefined basses from
* file. Does not do much error
* checking since this is sort
* of an internal component of
* the app.
*/
bool BassMapper::init(const string bassFileName) {
  keyMap.clear();
  keys.clear();

  // TODO: maybe something to support displaying and selecting enharmonic notes
  string keysArray[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

  // build map from note to MIDI note
  for (int i = 0; i < 12; i += 1) {
    keys.push_back(keysArray[i]);
    keyMap[keysArray[i]] = i + 48;
  }

  string line; // for parsing by each line
  ifstream bassFile(bassFileName.c_str());
  // read in scale to note mapping
  while (getline(bassFile, line)) {
    istringstream iSS(line);
    char bassKey;
    iSS >> bassKey;

    // new vector of basses for given char
    noteMaps[bassKey] = vector<int>();

    int noteOffset;
    while (iSS >> noteOffset)
      // read in the bass note offset positions
      noteMaps[bassKey].push_back(noteOffset);
  }

  // initialize mapping
  initialized = true;
  setKeyIndex(0);
  return true;
}

/**
* Function: setKeyIndex
* -----------------------
* Set the current key index
* to be used when mapping.
*/
bool BassMapper::setKeyIndex(int index) {
  if (!initialized) return false;
  keyIndex = index;
  return true;
}
