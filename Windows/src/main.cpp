/**
 * File: main.cpp
 * Author: Sanjay Kannan
 * Author: Aidan Meacham
 * ---------------------
 * Initializes OpenFrameworks
 * and runs the windowed app.
 */

#include "ofMain.h"
#include "ofApp.h"

/**
 * Function: main
 * --------------
 * Sets up OpenFrameworks
 * and runs the window thread.
 */
int main() {
  // set up the OpenGL context in window
  ofSetupOpenGL(1024, 768, OF_WINDOW);

  // this kicks off the running of my app
  // can be OF_WINDOW or OF_FULLSCREEN
  // pass in width and height too:
  ofRunApp(new ofApp());
}

#ifdef _WIN32
// needed particularly for Windows platforms to hide the console window
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
  // set up the OpenGL context in window
  ofSetupOpenGL(1024, 768, OF_WINDOW);

  HWND hwnd = ofGetWin32Window();
  HICON hMyIcon = LoadIcon(hInstance, MAKEINTRESOURCE(102));
  SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM) hMyIcon);

  // this kicks off the running of my app
  // can be OF_WINDOW or OF_FULLSCREEN
  // pass in width and height too:
  ofRunApp(new ofApp());
}

// spooky
#endif
