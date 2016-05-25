/**
 * File: ofApp.h
 * Author: Sanjay Kannan
 * Author: Aidan Meacham
 * ---------------------
 * Header file for the entire
 * OpenFrameworks application.
 */

#pragma once
#include <set>

#include "ofMain.h"
#include "ofxCv.h"
#include "mapper.h"
#include "bassMapper.h"
#include "synthesizer.h"

/**
 * Type: Note
 * ----------
 * A simple struct to hold
 * notes derived from MIDI.
 */
struct Note {
  int note;
  // in seconds
  double duration;

  // comparison functions for algorithms
  bool operator>(const Note& n) const { return note > n.note; }
  bool operator>=(const Note& n) const { return note >= n.note; }
  bool operator==(const Note& n) const { return note == n.note; }
  bool operator<=(const Note& n) const { return note <= n.note; }
  bool operator<(const Note& n) const { return note < n.note; }
};

// master OpenFrameworks runner
class ofApp : public ofBaseApp {
  public:
    void setup();
    void update();
    void draw();

    // some usual boilerplate
    void keyPressed(int key);
    void keyReleased(int key);
    void windowResized(int width, int height);

  private:
    // FluidSynth wrapper class
    Synthesizer* synth = NULL;
    int synthVol = 0;

    // initialize camera
    ofVideoGrabber camera;

    // Farneback is whole image flow
    // LK is flow for features
    ofxCv::FlowPyrLK lkFlow;

    // avoid note repeats by
    // tracking playing notes
    set<int> playing;

    // map keys to scales
    vector<string> scales;
    vector<string> keys;
    vector<string> modes;
    Mapper mapper;
    BassMapper bMapper;

    // play through files
    vector<string> filesMIDI;
    bool loadedMIDI = false;
    bool playThrough = false;
    bool hardMode = false;
    int filesIndex = 0;
    int songPosition = 0;
    map<int, int> keyPosMap;

    // built for every file
    vector<vector<Note>> song;
    vector<Note> topNotes;
    vector<char> songKeys;

    // hard mode coloring
    vector<int> previews;
    int highlight = -1;
    int highTime;
    int highDuration;

    // ignore side presses
    long long lastPressTime = 0;
    int debounceTime = 35;

    // mapping state
    vector<int> instruments;
    bool bassMode = false;
    int scaleIndex = 0;
    int keyIndex = 0;
    int modeIndex = 0;
    int instIndex = 0;

    // bellows state
    bool sounding = false;
    bool volumeBoost = false;
    float tiltSmooth = 0.0;
    float tiltSpeed = 0.0;
    float shakeSmooth = 0.0;
    float shakeSpeed = 0.0;
    float tiltDir = 0.0;
    long long lastTime = -1;
    int numFrames = 0;
    float tau = 500;
    double gain = 3.0;

    // graphics-related functions
    void drawBaffle(float pct);
    void drawKeys();

    // window-related stuff
    int wh; // window height
    int ww; // window width
    bool fulscr; // fullscreen
    bool fulscrToggled;

    // particle stuff
    int nPrtcl; // particle count
    vector<vector<float>> prtclPos;
    vector<ofColor> prtclColor;

    // particle or bellow
    bool skeumorph;

    // whether to draw help text
    // in the barebones view
    bool keybToggled = false;

    // baffle stuff
    float position;
    float compress;
    float velocity;

    // keyboard graphics stuff
    map<int, ofColor> color;
    float keybPosition;
    set<int> pressed;
    bool keybOn;

    // key press interval
    float avgDiff;

    // hell mode functions and state variables
    void drawLeder(float pos, float offset, float rotSpd, float fade);
    void drawNyan(float pos, float offset, float fade);

    bool hellMode;
    long long lastPress;
    int pressCounter;

    vector<int> pressHist;
    vector<float> flameHeight;
    vector<float> curFlame;

    ofImage leder;
    ofImage nyan;

    vector<float> lederPos;
    vector<float> lederOffset;
    vector<float> lederRotspd;
    long long hellStart;

    // mouse velocity stuff
    int lastX; // defined
    int lastY; // defined
    long long lastTimeM = -1;
    float xVel = 0.0;
    float yVel = 0.0;
    float xVelSm = 0.0;
    float yVelSm = 0.0;
    float xAcc = 0.0;
    float yAcc = 0.0;
    float vTau = 250;
    bool bend = false;
};
