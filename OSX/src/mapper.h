/**
 * File: mapper.h
 * Author: Sanjay Kannan
 * ---------------------
 * Maps combinations of mode mappings, key
 * presses, and scales to a MIDI pitch
 * for a given channel.
 */

#ifndef MAPPER_H
#define MAPPER_H

#include <string>
#include <map>
#include <vector>
using namespace std;

// maps MIDI notes
class Mapper {
  public:
    // initialize mapper with scales and mode mappings
    bool init(const string scaleFileName, const string modeFileName);

    // get MIDI pitch for key
    int getNote(int key);

    // get mapped scale position
    int getPosition(int key);

    // accessors for graphical listing
    const vector<string>& getScales();
    const vector<string>& getKeys();
    const vector<string>& getModes();

    // mutators after initialization
    bool setScaleIndex(int index);
    bool setKeyIndex(int index);
    bool setModeIndex(int index);

  private:
    // map from keys to MIDI
    map<string, int> keyMap;
    vector<string> keys;

    // used for modes [keyboard mappings]
    map<string, vector<int> > modeMap;
    vector<string> modes;

    // used for scale position mapping
    map<string, vector<int> > scaleMap;
    vector<string> scales;

    // used for sanity check
    bool initialized = false;

    // mapping state
    int modeIndex;
    int scaleIndex;
    int keyIndex;
};

// guard
#endif
