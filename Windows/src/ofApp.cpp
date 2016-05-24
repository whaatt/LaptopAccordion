/**
 * File: ofApp.cpp
 * Author: Sanjay Kannan
 * Author: Aidan Meacham
 * ---------------------
 * Implentation for the entire
 * OpenFrameworks application.
 */

#include "ofApp.h"
#include <sstream>
#include <math.h>
#include <dirent.h>
#include "MIDI/MidiFile.h"

using namespace ofxCv;
using namespace cv;

/**
 * Function: getMIDIFiles
 * ----------------------
 * Gets the MIDI files in a directory
 */
bool getMIDIFiles(vector<string>& list, string dirName) {
  DIR *dir; struct dirent *ent;
  // ugly shit that looks like C code
  if ((dir = opendir(dirName.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      char* name = ent -> d_name;
      size_t len = strlen(name);

      if (len > 4 && strcmp(name + len - 4, ".mid") == 0)
        list.push_back(dirName + "/" + name); // add MIDI to list
    }

    // clean up
    closedir(dir);
    return true;
  }

  // could not open
  return false;
}

/**
 * Function: buildSongVector
 * -------------------------
 * Builds a vector of vectors representing
 * all of the notes in a song. Each inner
 * vector represents notes played at a
 * particular time together.
 */
void buildSongVector(vector<vector<Note>>& song, vector<char>& songKeys,
  vector<Note>& topNotes, string fileName) {
  MidiFile songMIDI; // from Midifile library
  songMIDI.read(fileName);

  songMIDI.linkNotePairs();
  songMIDI.doTimeAnalysis();
  songMIDI.joinTracks();

  MidiEvent* event;
  int simulIndex = -1;
  song.clear(); // empty old song
  int deltaTick = -1;

  // iterate through list of note on events and add them to song
  for (int evIdx = 0; evIdx < songMIDI[0].size(); evIdx += 1) {
    event = &songMIDI[0][evIdx];
    if (!event -> isNoteOn()) continue;

    if (event -> tick != deltaTick) {
      deltaTick = event -> tick;
      song.push_back(vector<Note>());
      simulIndex += 1;
    }

    Note newNote;
    newNote.note = (int) (*event)[1];
    newNote.duration = event -> getDurationInSeconds();
    song[simulIndex].push_back(newNote);
  }

  if (song.size() == 0) return; // empty
  string hardKeys("fghj"); // six notes guitar hero style
  Note lastNote = *max_element(song[0].begin(), song[0].end());
  int lastKeyIndex = 3; // corresponds to j
  songKeys.push_back(hardKeys[lastKeyIndex]);
  topNotes.push_back(lastNote);

  // determine hard mode key mappings [jank]
  for (int i = 1; i < song.size(); i += 1) {
    Note currNote = *max_element(song[i].begin(), song[i].end());
    int diff = currNote.note - lastNote.note; // determines key interval

    int nextKeyIndex;
    if (diff == 0) nextKeyIndex = lastKeyIndex; // same note
    else if (diff > 0 && diff < 3) nextKeyIndex = (lastKeyIndex + 1) % hardKeys.size();
    else if (diff > 2 && diff < 5) nextKeyIndex = (lastKeyIndex + 2) % hardKeys.size();
    else if (diff < 0 && diff > -3) nextKeyIndex = (lastKeyIndex - 1) % hardKeys.size();
    else if (diff < -2 && diff > -5) nextKeyIndex = (lastKeyIndex - 2) % hardKeys.size();
    else if (diff > 4) nextKeyIndex = (lastKeyIndex + 3) % hardKeys.size();
    else nextKeyIndex = (lastKeyIndex - 3) % hardKeys.size();

    // normalize negative mods to positive before append
    if (nextKeyIndex < 0) nextKeyIndex += hardKeys.size();
    songKeys.push_back(hardKeys[nextKeyIndex]);
    topNotes.push_back(currNote);

    // update last values
    lastNote = currNote;
    lastKeyIndex = nextKeyIndex;
  }
}

/**
 * Function: setup
 * ---------------
 * Initializes the camera.
 */
void ofApp::setup() {
  // initialize camera
  camera.initGrabber(640, 480);
  ofSetWindowTitle("Laptop Accordion");

// platform prefix
#ifdef _WIN32
  string prefix("data/"); // Windows bundles
  string ofPrefix(""); // Windows bundles
#else
  string prefix("../../../data/"); // OSX bundles
  string ofPrefix("../../../data/"); // OSX bundles
#endif

  // modes just contains keyboard modes [irrelevant here]
  mapper.init(prefix + "scales.txt", prefix + "modes.txt");
  bMapper.init(prefix + "basses.txt");

  if (getMIDIFiles(filesMIDI, prefix + "MIDI"))
    loadedMIDI = true; // successful load

  // get UI listing variables
  scales = mapper.getScales();
  keys = mapper.getKeys();
  modes = mapper.getModes();

  // initialize synthesizer
  synth = new Synthesizer();
  synth -> init(44100, 256, 3.0, true);
  synth -> load((prefix + "primary.sf2").c_str());

  // load MIDI instrument number from file
  ifstream inst(prefix + "instrument.txt");
  int instCode = 21; // default instrument
  inst >> instCode; // read integer from file
  synth -> setInstrument(1, instCode - 1);

  // initialize graphics
  ofBackground(190,30,45);
  wh = ofGetWindowHeight();
  ww = ofGetWindowWidth();
  position = 0.25;
  velocity = 0;

  // particle mode
  skeumorph = true;
  nPrtcl = 200;
  prtclColor.resize(nPrtcl);
  prtclPos.resize(nPrtcl);

  // initialize nPrtcl particles
  for (int i = 0; i < nPrtcl; i += 1) {
    prtclPos[i].resize(2);
    prtclPos[i][0] = (float) rand() / RAND_MAX;
    prtclPos[i][1] = (float) rand() / RAND_MAX;
    prtclColor[i] = ofColor(ofRandom(0, 255),
    ofRandom(0, 255), ofRandom(0, 255));
  }

  // hell mode stuff
  hellMode = false;
  leder.load(ofPrefix + "lederhosen.png");
  nyan.load(ofPrefix + "nyan.png");
  flameHeight.resize(20);
  curFlame.resize(20);

  // initialize flame levels
  for (int i = 0; i < 20; i += 1)
    curFlame[i] = 1.0;

  // track press frequency for hell mode
  lastPress = ofGetElapsedTimeMillis();
  pressCounter = 0;
  pressHist.resize(10);

  lederPos.resize(10);
  lederOffset.resize(10);
  lederRotspd.resize(10);

  // initialize lederdudes
  for (int i = 0; i < 10; i += 1) {
    lederPos[i] = wh * (float) rand() / RAND_MAX;
    lederOffset[i] = ww * (float) rand() / RAND_MAX;
    lederRotspd[i] = 2 * (float) rand() / RAND_MAX - 1;
  }

  // onscreen keys stuff
  keybPosition = -ww;
  keybOn = false;
  fulscr = false;
}

/**
 * Function: update
 * ----------------
 * Grabs frame and updates optical
 * flow values. Performs exponential
 * smoothing on flow values.
 */
void ofApp::update() {
  // get new frame
  camera.update();

  // new frame found
  if (camera.isFrameNew()) {
    numFrames += 1;

    // start with base values on first call
    if (lastX == -1 || lastY == -1) {
      lastX = ofGetMouseX();
      lastY = ofGetMouseY();
      return;
    }

    // get the current time in ms
    long long now = ofGetElapsedTimeMillis();
    if (lastTimeM == -1) lastTimeM = now;

    float dT = now - lastTimeM;
    if (dT == 0) return;
    lastTimeM = now;

    // tau is the decay time constant
    float alpha = 1.0 - exp(-dT / vTau);

    // update raw values
    int newX = ofGetMouseX();
    int newY = ofGetMouseY();
    xVel = (float) (newX - lastX) / dT;
    yVel = (float) (newY - lastY) / dT;
    lastX = newX;
    lastY = newY;

    // smooth the raw velocity values and update accel
    float xVelSmNew = alpha * xVel + (1.0 - alpha) * xVelSm;
    float yVelSmNew = alpha * yVel + (1.0 - alpha) * yVelSm;
    xAcc = (xVelSmNew - xVelSm) / dT;
    yAcc = (yVelSmNew - yVelSm) / dT;
    xVelSm = xVelSmNew;
    yVelSm = yVelSmNew;

    if (bend) // pitch bend with touchpad
      synth -> pitchBend(1, -yVelSm < -1.0 
        ? -1.0 : (-yVelSm > 1.0 ? 1.0 : -yVelSm));

    // below this line is
    // smoothing shit for
    // tilt detection

    // get the current time in ms
    if (lastTime == -1) lastTime = now;

    dT = now - lastTime;
    lastTime = now;

    // tau is the decay time constant
    alpha = 1.0 - exp(-dT / tau);

    lkFlow.calcOpticalFlow(camera);
    if (numFrames % 10 == 0) lkFlow.resetFeaturesToTrack();
    vector<ofVec2f> flows = lkFlow.getMotion();

    float flowX = 0.0;
    float flowY = 0.0;
    float flowYDir = 0.0;
    float avgFlowX = 0.0;
    float avgFlowY = 0.0;

    // find the absolute average of all flows
    for (int i = 0; i < flows.size(); i += 1) {
      // TODO: better built-in way to do this?
      flowX += abs(flows[i].x);
      flowY += abs(flows[i].y);
      flowYDir += flows[i].y;
    }

    avgFlowX = flowX / (float) flows.size();
    avgFlowY = flowY / (float) flows.size();
    tiltSpeed = avgFlowY; // accordion on Y-axis
    shakeSpeed = avgFlowX; // shaking on X-axis
    if (tiltSpeed != tiltSpeed) return; // NaN

    // formula for exponentially-weighted moving average
    tiltSmooth = alpha * tiltSpeed + (1.0 - alpha) * tiltSmooth;
    shakeSmooth = alpha * shakeSpeed + (1.0 - alpha) * shakeSmooth;
    tiltDir = flowYDir;

    int volume;
    // use bellow velocity to update the channel synth velocity
    if (!volumeBoost) volume = std::min(127, (int) (tiltSmooth / 45.0 * 127.0));
    else volume = std::min(127, (int) (tiltSmooth / 15.0 * 127.0));
    int diffIncrement = volume - synthVol; // avoid jumpy changes with slew

    // update the synth volume by an slew in direction of tilt velocity
    synthVol = (int) ((float) synthVol * 0.9 + (float) diffIncrement * 0.1);
    synth -> controlChange(1, 7, synthVol);
    sounding = synthVol > 5;
  }

  // slew keyboard on and offscreen
  if (keybOn) keybPosition = 0; //keybPosition * .9;
  else if (fulscrToggled) keybPosition = -ww * 2;
  else keybPosition = -ww; //keybPosition + (-ww - keybPosition) * .1;

  // reset toggle state
  if (fulscrToggled)
    fulscrToggled = false;

  // compress bellows between 0.25 and 0.5 using a linear easing function
  if (tiltDir > 0) position = position + (1.0 - position) * tiltSmooth * .0005;
  else position = position - position * tiltSmooth * .0005;
  compress = position * 0.25 + 0.25;

  // ease hell mode when no presses
  avgDiff += (250 - avgDiff) * 0.04;
}

/**
 * Function: draw
 * --------------
 * Just some testing code.
 */
void ofApp::draw() {
  // get window params
  wh = ofGetWindowHeight();
  ww = ofGetWindowWidth();

  // bellows mode
  if (skeumorph) {
    // draw baffles
    ofPushMatrix();
      ofBackground(190, 30, 45);
      for (int i = 0; i < 10; i += 1) {
        ofPushMatrix();
          // translate based on compression factor
          ofTranslate(0, i * wh / 5 * compress * 2);
          drawBaffle(compress);
        ofPopMatrix();
      }

      // translate all baffles
      ofTranslate(0, wh, 0);
    ofPopMatrix();
  }

  else {
    // draw particles
    ofPushMatrix();
    ofPushStyle();

      ofBackground(0, 0, 0);
      // update particle positions
      for (int i = 0; i < nPrtcl; i += 1) {
        ofSetColor(prtclColor[i]); // randomized before
        ofDrawRectangle(prtclPos[i][0] * ww, prtclPos[i][1] * wh + i / 30 * compress * wh / 4 - 30, 5, 5);
      }

    ofPopStyle();
    ofPopMatrix();
  }

  // keyboard
  ofPushMatrix();
    // draw keys with alpha
    ofTranslate(keybPosition, 0);

    ofPushStyle();
      ofEnableAlphaBlending();
      int alpha = skeumorph ? 180 : 0;
      ofSetColor(255, 255, 255, alpha);
      ofDrawRectangle(0, 0, ww, wh);
      ofDisableAlphaBlending();
    ofPopStyle();

    drawKeys();
  ofPopMatrix();

  // hell stuff
  if (hellMode) {
    float hellFade;
    if (avgDiff > 250) hellFade = 0;
    //else if (avgDiff < 50) hellFade = 255;
    else hellFade = (float) (250 - avgDiff) / 250.0 * 255;
    ofEnableAlphaBlending();

    // bellows mode
    if (skeumorph) {
      for (int i = 0; i < 10; i += 1) // draw updated lederdudes
        drawLeder(lederPos[i], lederOffset[i], lederRotspd[i], hellFade);

      ofPushMatrix();
      ofPushStyle();
        ofTranslate(ww,0);

        ofBeginShape();
          ofSetColor(255, 0, 0, hellFade);
          ofVertex(0,0);

          // update flame heights
          for (int i = 0; i < 20; i += 1) {
            if (ofGetElapsedTimeMillis() % 3 == 0)
              flameHeight[i] = (float) rand() / RAND_MAX;
            curFlame[i] = curFlame[i] + (flameHeight[i] - curFlame[i]) * .09;
            ofVertex(-ww * curFlame[i], i * wh / 20 + ww / 40);
          }

          ofVertex(0,wh);
        ofEndShape(false);

        ofBeginShape();
          ofSetColor(255, 255, 0, hellFade);
          ofVertex(0,0);

          // inner flame I think
          for(int i = 0; i < 20; i += 1)
            ofVertex(-ww*curFlame[i]*.5,i*wh/20+ww/40);

          ofVertex(0,wh);
        ofEndShape(false);
      ofPopMatrix();
      ofPopStyle();
    }

    else { // particles
      for (int i = 0; i < 10; i += 1) // draw updated nyan
        drawNyan(lederPos[i], lederOffset[i], hellFade);

      // draw all rainbows
      ofPushMatrix();
      ofPushStyle();
        ofTranslate(ww,0);

        ofBeginShape();
          ofSetColor(255, 0, 0, hellFade);
          ofVertex(0, 0);

          for(int i = 0; i < 20; i += 1) {
            if(ofGetElapsedTimeMillis() % 3 == 0)
              flameHeight[i] = (float) rand() / RAND_MAX;
            curFlame[i] = curFlame[i] + (flameHeight[i] - curFlame[i]) *.09;
            ofVertex(-ww * curFlame[i] * .6, i * wh / 20 + ww / 40);
          }

          ofVertex(0,wh);
        ofEndShape(false);

        ofBeginShape();
          ofSetColor(255,255,0,hellFade);
          ofVertex(0,0);

          for (int i = 0; i < 20; i += 1)
            ofVertex(-ww * curFlame[i] * .5, i * wh / 20 + ww / 40);

          ofVertex(0, wh);
        ofEndShape(false);

        ofBeginShape();
          ofSetColor(0, 255, 0, hellFade);
          ofVertex(0, 0);

          for(int i = 0; i < 20; i += 1)
            ofVertex(-ww * curFlame[i] * .4, i * wh / 20 + ww / 40);

          ofVertex(0,wh);
        ofEndShape(false);

        ofBeginShape();
          ofSetColor(0, 0, 255, hellFade);
          ofVertex(0, 0);

          for(int i = 0; i < 20; i += 1)
            ofVertex(-ww * curFlame[i] * .3, i * wh / 20 + ww / 40);

          ofVertex(0,wh);
        ofEndShape(false);

        ofBeginShape();
          ofSetColor(255, 0, 255, hellFade);
          ofVertex(0,0);

          for(int i = 0; i < 20; i += 1)
            ofVertex(-ww * curFlame[i] * .2, i * wh / 20 + ww / 40);

          ofVertex(0,wh);
        ofEndShape(false);
      ofPopMatrix();
      ofPopStyle();
    }
  }

  if (!keybToggled)
    ofDrawBitmapString("Welcome to Laptop Accordion 0.0.1!\n" + // welcome
      string("Toggle Keyboard With Backslash (\\)"), ww / 2 - 130, 20, 2);

  // for good measure
  ofDisableAlphaBlending();
}

/**
 * Function: keyPressed
 * --------------------
 * Handles key presses.
 */
void ofApp::keyPressed(int key) {
  // start playing a given note
  if (!bassMode && ((key >= 'a' && key <= 'z') ||
      key == ';' || key == ',' || key == '.' || key == '/')) {
    if (!playThrough) {
      int note = mapper.getNote(key);
      if (playing.count(note)) return;

      // note is not already playing: turn it on
      synth -> noteOn(1, note, 127);
      playing.insert(note);
      pressed.insert(key);
    }

    else {
      // avoid multiple key presses
      // even those we are not handling
      if (pressed.count(key)) return;
      pressed.insert(key);

      // bellows not moving [hard mode only]
      if (hardMode && !sounding) return;

      if (keyPosMap.find(key) != keyPosMap.end())
        return; // already handling this key press

      long long now = ofGetElapsedTimeMillis();
      if (now - lastPressTime < debounceTime)
        return; // likely an accidental key mash

      if (hardMode && key != highlight)
        return; // wrong key played

      keyPosMap[key] = songPosition; // turn off shit by the key
      for (int i = 0; i < song[songPosition].size(); i += 1) {
        int note = song[songPosition][i].note;
        synth -> noteOn(1, note, 127);
      }

      // colorings
      if (hardMode) {
        previews.clear();
        if (songPosition + 1 < song.size()) highlight = songKeys[songPosition + 1];
        if (songPosition + 2 < song.size()) previews.push_back(songKeys[songPosition + 2]);
        if (songPosition + 3 < song.size()) previews.push_back(songKeys[songPosition + 3]);
        if (songPosition + 4 < song.size()) previews.push_back(songKeys[songPosition + 4]);
        if (songPosition + 5 < song.size()) previews.push_back(songKeys[songPosition + 5]);
        if (songPosition + 6 < song.size()) previews.push_back(songKeys[songPosition + 6]);
        if (songPosition + 7 < song.size()) previews.push_back(songKeys[songPosition + 7]);
      }

      // move to next
      lastPressTime = now;
      songPosition += 1;
    }

    int red = 170;
    int green = (255 + 221 + rand() % 34) / 2;
    int blue = (200 + 200 + rand() % 55) / 2;
    ofColor random(red, green, blue);
    // ofColor green(170, 255, 170);
    color[key] = random;

    // hell mode activation by key frequency
    long long thisPress = ofGetElapsedTimeMillis();
    int pressDiff = thisPress - lastPress;
    lastPress=thisPress;
    pressCounter++;

    // buffer of the last ten diff values
    pressHist[pressCounter % 10] = pressDiff;

    // find avg diff
    int diffSum = 0;
    for(int i = 0; i < 10; i += 1)
      diffSum = diffSum + pressHist[i];
    avgDiff = diffSum / 10;

    // trigger hell mode < 250
    if (avgDiff < 250) hellMode = true;
    else hellMode = false;
  }

  // start playing a given set of bass notes
  else if (bassMode && ((key >= 'a' && key <= 'z') ||
    (key >= '4' && key <= '9') || key == '0' ||
    key == ',' || key == '-' || key == '.' ||
    key == ';' || key == '[' || key == '=')) {
    vector<int> notes = bMapper.getNotes(key);
    bool foundPlaying = false; // avoid retrigger

    // play each of the bass notes
    for (size_t i = 0; i < notes.size(); i += 1) {
      if (playing.count(notes[i])) {
        foundPlaying = true;
        continue;
      }

      // note is not already playing: turn it on
      synth->noteOn(1, notes[i], 127);
      playing.insert(notes[i]);
      pressed.insert(key);
    }

    if (foundPlaying) return; // TODO: remove?
    // hell mode activation by key frequency
    long long thisPress = ofGetElapsedTimeMillis();
    int pressDiff = thisPress - lastPress;
    lastPress = thisPress;
    pressCounter++;

    // buffer of the last ten diff values
    pressHist[pressCounter % 10] = pressDiff;

    // find avg diff
    int diffSum = 0;
    for (int i = 0; i < 10; i += 1)
      diffSum = diffSum + pressHist[i];
    avgDiff = diffSum / 10;

    // trigger hell mode < 250
    if (avgDiff < 250) hellMode = true;
    else hellMode = false;
  }

  // press backslash for
  // onscreen keyboard
  if (key == '\\') {
    keybOn = !keybOn;
    keybToggled = true;
  }

  // ` for fullscreen
  if (key == '`') {
    fulscr = !fulscr;
    fulscrToggled = true;
    ofSetFullscreen(fulscr);
  }

  if (key == '1' && !playThrough)
    bassMode = !bassMode;

  // toggle hard mode for play through
  if (key == '0' && !playThrough && !bassMode)
    hardMode = !hardMode;

  // toggle play through
  if (key == '=' && !bassMode) {
    // toggle off
    if (playThrough) {
      playThrough = false;
      synth -> allNotesOff(1);
      pressed.clear();
      previews.clear();
      highlight = -1;
      return;
    }

    // TODO: error message this
    if (!loadedMIDI) return;
    playThrough = true;
    songPosition = 0;

    // calculate note lengths and positions
    buildSongVector(song, songKeys, topNotes, filesMIDI[filesIndex]);

    // bad song passed
    if (!song.size()) {
      playThrough = false;
      return;
    }

    // highlights
    if (hardMode) {
      previews.clear();
      if (song.size() > 0) highlight = songKeys[0];
      if (song.size() > 1) previews.push_back(songKeys[1]);
      if (song.size() > 2) previews.push_back(songKeys[2]);
      if (song.size() > 3) previews.push_back(songKeys[3]);
      if (song.size() > 4) previews.push_back(songKeys[4]);
      if (song.size() > 5) previews.push_back(songKeys[5]);
      if (song.size() > 6) previews.push_back(songKeys[6]);
    }
  }

  // change scale [e.g. major] with [ and key [e.g. C#] with ]
  if (key == ']') mapper.setKeyIndex(keyIndex = ++keyIndex % keys.size());
  if (key == ']') bMapper.setKeyIndex(keyIndex); // key index reset in previous
  if (key == '[') mapper.setScaleIndex(scaleIndex = ++scaleIndex % scales.size());

  // change mode [keyboard layout schematic, e.g. inc by rows] with '
  if (key == '\'') mapper.setModeIndex(modeIndex = ++modeIndex % modes.size());

  // change the selected song in directory with - when not in playthrough mode
  if (key == '-' && !playThrough && !bassMode) filesIndex = ++filesIndex % filesMIDI.size();

  // press 9 for skeumorphism
  if (key == '9' && !bassMode) skeumorph = !skeumorph;

  // press 2 for toggling volume boost
  if (key == '2') volumeBoost = !volumeBoost;

  // set gain values with arrows
  if ((key == OF_KEY_LEFT && !bassMode) || // inverted switcher in bass mode
      (key == OF_KEY_RIGHT && bassMode)) gain = gain < 9.8 ? gain + 0.2 : gain;
  if ((key == OF_KEY_RIGHT && !bassMode) || // inverted switcher in bass mode
      (key == OF_KEY_LEFT && bassMode)) gain = gain > 0.2 ? gain - 0.2 : gain;
  if (key == OF_KEY_LEFT || key == OF_KEY_RIGHT) synth -> setGain(gain);

  // press 8 for toggling pitch bend
  if (key == '8' && !bassMode) bend = !bend;
  if (!bend) synth -> pitchBend(1, 0);
}

/**
 * Function: keyReleased
 * ---------------------
 * Handles key releases.
 */
void ofApp::keyReleased(int key) {
  // stop playing a given note
  if (!bassMode && ((key >= 'a' && key <= 'z') ||
      key == ';' || key == ',' || key == '.' || key == '/')) {
    if (!playThrough) {
      int note = mapper.getNote(key);
      if (!playing.count(note)) return;

      // note is playing: turn it off
      synth -> noteOff(1, note);
      playing.erase(note);
      pressed.erase(key);
    }

    else {
      // do nothing if key pressed but was initially ignored
      if (pressed.count(key) && !keyPosMap.count(key)) {
        pressed.erase(key); // reset key state
        return;
      }

      // turn off all notes in the time vector for the given key
      for (int i = 0; i < song[keyPosMap[key]].size(); i += 1) {
        int note = song[keyPosMap[key]][i].note;
        synth -> noteOff(1, note);
      }

      // remove the key from map
      pressed.erase(key);
      keyPosMap.erase(keyPosMap.find(key));

      // song is over so disable play through
      if (songPosition >= song.size()) {
        synth -> allNotesOff(1);
        playThrough = false;
        pressed.clear();
        previews.clear();
        highlight = -1;
      }
    }
  }

  // stop playing a given set of bass notes
  else if (bassMode && ((key >= 'a' && key <= 'z') ||
    (key >= '4' && key <= '9') || key == '0' ||
    key == ',' || key == '-' || key == '.' ||
    key == ';' || key == '[' || key == '=')) {
    vector<int> notes = bMapper.getNotes(key);

    // stop each of the bass notes
    for (size_t i = 0; i < notes.size(); i += 1) {
      if (!playing.count(notes[i])) continue;

      // note is playing: turn it off
      synth->noteOff(1, notes[i]);
      playing.erase(notes[i]);
      pressed.erase(key);
    }
  }
}

/**
 * Function: drawLeder
 * --------------------
 * Draws a single lederhosen dude.
 */
void ofApp::drawLeder(float pos, float offset, float rotSpd, float fade) {
  ofPushStyle();
    ofSetColor(255, 255, 255, fade);

    ofPushMatrix();
      ofTranslate(fmod(ofGetElapsedTimeMillis() / 2 + offset, ww + leder.getWidth()) - leder.getWidth(), pos);
      ofRotate(fmod(rotSpd * ofGetElapsedTimeMillis() / 12, 360));
      leder.draw(-leder.getWidth() / 2, -leder.getHeight() / 2);
    ofPopMatrix();
  ofPopStyle();
}

/**
 * Function: drawNyan
 * --------------------
 * Draws a single nyan cat.
 */
void ofApp::drawNyan(float pos, float offset, float fade){
  ofPushStyle();
    ofSetColor(255, 255, 255, fade);

    ofPushMatrix();
      ofTranslate(fmod(ofGetElapsedTimeMillis() / 2 + offset, ww + nyan.getWidth()) - nyan.getWidth(), pos);
      nyan.draw(-nyan.getWidth() / 2, -nyan.getHeight() / 2);
    ofPopMatrix();
  ofPopStyle();
}


/**
 * Function: drawBaffle
 * --------------------
 * Draws a single skeumorphic baffle.
 */
void ofApp::drawBaffle(float pct) {
  ofPushStyle();
  ofSetLineWidth(2);
  ofSetColor(ofColor::black);
  // draw black baffle lines
  ofDrawLine(0, 0, ww / 10, wh / 5 * pct);
  ofDrawLine(0, 0, ww / 10, wh / 5 * pct);
  ofDrawLine(ww, 0, ww - ww / 10, wh / 5 * pct);
  ofDrawLine(ww / 10, wh / 5 * pct, 0, wh / 5 * pct * 2);
  ofDrawLine(ww - ww / 10, wh / 5 * pct, ww, wh / 5 * pct * 2);
  ofDrawLine(ww / 10, wh / 5 * pct, ww - ww / 10, wh / 5 * pct);
  ofPopStyle();

  ofPushStyle();
  ofSetLineWidth(4);
  ofSetColor(255, 222, 23);
  // draw yellow midline
  ofDrawLine(0, 0, ww, 0);
  ofPopStyle();
}

/**
 * Function: drawKeys
 * ------------------
 * Draws onscreen keyboard.
 */
void ofApp::drawKeys(){
  float keyWidth = ww / 12 - (ww / 12) * .1;
  float keyHeight = keyWidth; // squares

  stringstream gs; gs << gain;
  ofSetColor(ofColor(0, 0, 255));
  string mPath(filesMIDI[filesIndex]);
  string MIDIFile(mPath.substr(mPath.find_last_of("/\\") + 1));
  ofDrawBitmapString("Toggle Keyboard With Backslash (\\)\n" +
                     string("Toggle Graphical Style With (9)\n") +
                     string("Toggle Fullscreen With Tick (`)\n\n") +
                     string("Current Scale: ") + scales[scaleIndex] + " ([)\n" +
                     string("Current Key: ") + keys[keyIndex] + " (])\n" +
                     string("Current Mode: ") + modes[modeIndex] + " (')\n\n" +
                     string("Bass Override: ") + (bassMode ? string("Enabled") : string("Disabled")) + " (1)\n" +
                     string("Volume Boost: ") + (volumeBoost ? string("Enabled") : string("Disabled")) + " (2)\n" +
                     string("Pitch Bend: ") + (bend ? string("Enabled") : string("Disabled")) + " (8)\n" +
                     string("Gain Level: ") + gs.str() + " (Arrows)\n\n" +
                     string("Selected Song: ") + MIDIFile.substr(0, MIDIFile.size() - 4) + // strip off .mid
                     string(" (-)\nPlayer Mode: ") + (playThrough ? string("Running") : string("Stopped")) +
                     string(" (=)\nHard Mode: ") + (hardMode ? string("On") : string("Off")) + " (0)", 10, 20, 2);

  if (!hardMode && !bassMode) {
    string topChars = "qwertyuiop";
    string midChars = "asdfghjkl;";
    string botChars = "zxcvbnm,./";

    // print out the top chars
    for (int i = 1; i < 11; i += 1) {
      if (pressed.count(topChars[i - 1])) ofSetColor(color[topChars[i - 1]]);
      else ofSetColor(ofColor(255, 255, 255)); // default is white

      ofDrawRectRounded(i * ww / 12 - 25, wh / 2 - keyHeight / 2 - keyHeight * 1.1,
        2, keyWidth, keyHeight, 10, 10, 10, 10); // position and size
      string letter(1, topChars[i - 1]); // for drawing bitmap string

      ofPushStyle();
        ofSetColor(ofColor::black);
        ofDrawBitmapString(letter, i * ww / 12 - 25 + keyWidth / 2,
          wh / 2 - keyHeight / 2 - keyHeight * 1.1 + keyHeight / 2, 2);
      ofPopStyle();
    }

    // print out the middle chars
    for (int i = 1; i < 11; i += 1) {
      if (pressed.count(midChars[i - 1])) ofSetColor(color[midChars[i - 1]]);
      else ofSetColor(ofColor(255, 255, 255)); // default is white

      ofDrawRectRounded(i * ww / 12, wh / 2 - keyHeight / 2, 2,
        keyWidth, keyHeight, 10, 10, 10, 10); // position and size
      string letter(1, midChars[i - 1]); // for drawing bitmap string

      ofPushStyle();
        ofSetColor(ofColor::black);
        ofDrawBitmapString(letter, i * ww / 12 + keyWidth / 2, wh / 2, 2);
      ofPopStyle();
    }

    // print out the bottom chars
    for (int i = 1; i < 11; i += 1) {
      if (pressed.count(botChars[i - 1])) ofSetColor(color[botChars[i - 1]]);
      else ofSetColor(ofColor(255, 255, 255)); // default is white

      ofDrawRectRounded(i * ww / 12 + 25, wh / 2 + keyHeight / 2 + keyHeight * .1,
        2, keyWidth, keyHeight, 10, 10, 10, 10); // position and size
      string letter(1, botChars[i - 1]); // for drawing bitmap string

      ofPushStyle();
        ofSetColor(ofColor::black);
        ofDrawBitmapString(letter, i * ww / 12 + 25 + keyWidth / 2,
          wh / 2 + keyHeight + keyHeight *.1, 2);
      ofPopStyle();
    }
  }

  else if (!bassMode) {
    ofPushStyle();
      ofTranslate(ww / 2, wh / 2);
      ofRotateZ(90); // easy view
      ofTranslate(-ww / 2, -wh / 2);

      string hardLetters = "fghj";
      // print Guitar Hero note grid
      for (int i = 4; i < 8; i += 1) {
        string letter(1, hardLetters[i - 4]);
        ofColor faded(240, 240, 240);

        ofSetColor(faded);
        if (previews.size() > 5 && hardLetters[i - 4] == previews[5]) ofSetColor(ofColor(170, 170, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 - 7 * keyHeight / 2 - keyHeight * .3, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofSetColor(faded);
        if (previews.size() > 4 && hardLetters[i - 4] == previews[4]) ofSetColor(ofColor(170, 170, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 - 5 * keyHeight / 2 - keyHeight * .2, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofSetColor(faded);
        if (previews.size() > 3 && hardLetters[i - 4] == previews[3]) ofSetColor(ofColor(170, 170, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 - 3 * keyHeight / 2 - keyHeight * .1, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofSetColor(faded);
        if (previews.size() > 2 && hardLetters[i - 4] == previews[2]) ofSetColor(ofColor(170, 170, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 - keyHeight / 2, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofSetColor(faded);
        if (previews.size() > 1 && hardLetters[i - 4] == previews[1]) ofSetColor(ofColor(170, 170, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 + keyHeight / 2 + keyHeight * .1, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofSetColor(faded);
        if (previews.size() > 0 && hardLetters[i - 4] == previews[0]) ofSetColor(ofColor(170, 170, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 + 3 * keyHeight / 2 + keyHeight * .2, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofSetColor(ofColor(255, 255, 255));
        if (highlight != -1 && hardLetters[i - 4] == highlight) ofSetColor(ofColor(125, 125, 255));
        ofDrawRectRounded(i * ww / 12, wh / 2 + 5 * keyHeight / 2 + keyHeight * .3, 2,
            keyWidth, keyHeight, 10, 10, 10, 10);

        ofPushStyle();
          ofSetColor(ofColor::black);
          ofDrawBitmapString(letter, i * ww / 12 + keyWidth / 2, wh / 2 + keyHeight * 3.3, 2);
        ofPopStyle();
      }

    // end rotate
    ofPopStyle();
  }
}

/**
 * Function: windowResized
 * -----------------------
 * Handle window resizing.
 */
void ofApp::windowResized(int width, int height) {
  // override custom window sizing
  // ofSetWindowShape(1024, 768);
  wh = height;
  ww = width;

  // rescale lederdudes
  for(int i = 0; i < 10; i += 1) {
    lederPos[i] = wh * (float) rand() / RAND_MAX;
    lederOffset[i] = ww * (float) rand() / RAND_MAX;
  }
}
