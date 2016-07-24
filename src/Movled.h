#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxDmx.h"
#include "ofxMidi.h"
#include "ofxRadioGroup.h"

#include "json.hpp"

enum {
    DONT_PLAY = -1
};

struct Movie {
    
    Movie() {
        triggerPlay.set("play", false);
    }

    string name;
    ofParameter<string> path;
    ofParameter<int> midiNote;
    ofVideoPlayer video;
    ofParameter<bool> triggerPlay;
};

class Movled : public ofBaseApp, public ofxMidiListener {

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    
    void windowResized(int w, int h);
	
    ofxPanel mMainPanel;
    vector<shared_ptr<ofxGuiGroup>> mMovieGroups;
    vector<shared_ptr<Movie>> mMovies;
    int mCurrentMovieIndex;
    
    void playMovieAtIndex(size_t index);
    void stopMovie();
    
protected:
    ofxMidiIn midiIn;
    ofxRadioGroup midiInputDevices;
    
public:
    void newMidiMessage(ofxMidiMessage& msg);
    void midiInputDeviceChange(ofxRadioGroupEventArgs &args);


protected:
    void loadMoviesFromFile();
    void addMovieToGui(shared_ptr<Movie> movie);
    
protected:
    ofxPanel mLogGui;
    
    deque<ofParameter<string>> mLogMessages;
    void logMessage(string message);
    
protected:
    nlohmann::json mSettings;
    void loadSettings();
    
protected:
    ofxDmx mDmx;
    void connectToDmxDevice();
    
    vector<ofColor> mLEDColours;
    vector<size_t> mWiringCache;
    void setWiringCache(string scheme, int width, int height);
    
public:
    void playTrack(bool &b);
    
protected:
    ofxGuiGroup mMasterGroup;
    ofParameter<float> mCurrentMoviePosition;
    ofxButton mStopButton;
    
protected:
    ofTexture mMovieTexture;
};

