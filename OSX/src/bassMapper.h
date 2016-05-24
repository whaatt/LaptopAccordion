/**
* File: bassMapper.h
* Author: Sanjay Kannan
* ---------------------
* Maps bass key presses to the
* appropriate notes or chords.
*/

#ifndef BASS_MAPPER_H
#define BASS_MAPPER_H

#include <string>
#include <map>
#include <vector>
using namespace std;

// maps MIDI notes
class BassMapper {
  public:
    // initialize mapper with bass mapping
    bool init(const string bassFileName);

    // get MIDI pitches for key
    vector<int> getNotes(int key);

    // mutators after initialization
    bool setKeyIndex(int index);

  private:
    // map from keys to MIDI
    map<string, int> keyMap;
    map<char, vector<int>> noteMaps;
    vector<string> keys;

    // used for sanity check
    bool initialized = false;

    // mapping state
    int keyIndex;
};

// guard
#endif
