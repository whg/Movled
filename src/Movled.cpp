#include "Movled.h"
#include <regex>

void Movled::setup(){
    
    ofBackground(0);
    ofSetVerticalSync(false);
    ofSetFrameRate(30);
    
    
    ofxBaseGui::setDefaultWidth(ofGetWidth() / 2);
    
    mMainPanel.setup("Movled", "mainSettings.xml", 0, 0);
    mMainPanel.setAllowDragging(false);

    midiInputDevices.setName("MIDI Input");
    
    auto &midiDevices = ofxMidiIn::getPortList();
    for (const auto &deviceName : midiDevices) {
        midiInputDevices.addChoice(deviceName);
    }

    ofAddListener(midiInputDevices.changeEvent, this, &Movled::midiInputDeviceChange);
    mMainPanel.add(midiInputDevices);
    midiIn.openPort(midiInputDevices.getCurrentChoice());
    midiIn.addListener(this);

    mLogGui.setup("Log", "log.xml", ofGetWidth() / 2, 0);
    
    mCurrentMovieIndex = DONT_PLAY;


    connectToDmxDevice();

    loadMoviesFromFile();
    loadSettings();
    
    
    mMainPanel.add(mMasterGroup.setup("master"));
    
    mMasterGroup.add(mCurrentMoviePosition.set("position", 0, 0, 1));
    
    mStopButton.addListener(this, &Movled::stopMovie);
    mMasterGroup.add(mStopButton.setup("stop current movie"));
    
    
}


void Movled::update(){

    if (mCurrentMovieIndex != DONT_PLAY) {
        auto movie = mMovies[mCurrentMovieIndex];
        movie->video.update();
        
        if (movie->video.getIsMovieDone()) {
            stopMovie();
            logMessage(movie->name + "finished");
        }
        
        if (movie->video.isFrameNew()) {
            int w = mSettings["output"]["width"];
            int h = mSettings["output"]["height"];
            int step = mSettings["numPixelsPerLED"];

            auto pix = movie->video.getPixels();
            
            if (pix.getWidth() < w * step) {
                logMessage("movie for " + movie->name + " is not wide enough, needs " +
                            ofToString(w * step) + ", found " + ofToString(pix.getWidth()));
               mCurrentMovieIndex = DONT_PLAY;
            }
            
            if (pix.getHeight() < h * step) {
                logMessage("movie for " + movie->name + " is not high enough, needs " +
                           ofToString(h * step) + ", found " + ofToString(pix.getHeight()));
                mCurrentMovieIndex = DONT_PLAY;
            }
            
            if (mLEDColours.size() < w * h) {
                mLEDColours.resize(w * h);
                setWiringCache(mSettings["wiring"], w, h);
            }
            
            size_t index = 0;
            if (mSettings["movieOrigin"] == "bottomLeft") {
                for (int i = 0; i < w; i++) {
                    for (int j = 0; j < h; j++) {
                        auto col = pix.getColor(i * step, pix.getHeight() - j * step);
                        mLEDColours[mWiringCache[index++]] = col;
                    }
                }
            }
            else if (mSettings["movieOrigin"] == "topLeft") {
                for (int i = 0; i < w; i++) {
                    for (int j = 0; j < h; j++) {
                        auto col = pix.getColor(i * step, j * step);
                        mLEDColours[mWiringCache[index++]] = col;
                    }
                }
            }
            
            
            mCurrentMoviePosition = movie->video.getPosition();
            
            if (!mMovieTexture.isAllocated()) {
                mMovieTexture.allocate(w, h, GL_RGB);
            }
            
            mMovieTexture.loadData(pix);
        }
    }
    
    if (mDmx.isConnected()) {
        for (size_t i = 0; i < mLEDColours.size(); i++) {
            auto &colour = mLEDColours[i];
            mDmx.setLevel(i * 3 + 1, static_cast<unsigned char>(colour.r));
            mDmx.setLevel(i * 3 + 3, static_cast<unsigned char>(colour.g));
            mDmx.setLevel(i * 3 + 2, static_cast<unsigned char>(colour.b));
        }
    }
    else if (ofGetFrameNum() % 15 == 0) {
        connectToDmxDevice();
    }
    
    if (mSettings["constantRefresh"] && ofGetFrameNum() % 30 == 0) {
        loadSettings();
    }
}


void Movled::draw(){

    if (mMovieTexture.isAllocated()) {
        int scale = 4;
        mMovieTexture.draw(0, ofGetHeight() - (mMovieTexture.getHeight() * scale),
                           mMovieTexture.getWidth() * scale, mMovieTexture.getHeight() * scale);
    }
    
    mMainPanel.draw();
    mLogGui.draw();
}


void Movled::keyPressed(int key){}
void Movled::keyReleased(int key){}
void Movled::mouseMoved(int x, int y ){}
void Movled::mouseDragged(int x, int y, int button){}
void Movled::mousePressed(int x, int y, int button){}
void Movled::mouseReleased(int x, int y, int button){}

void Movled::windowResized(int w, int h) {
    int colWidth = w / 2;
    mMainPanel.setWidthElements(colWidth);
    mMainPanel.setSize(colWidth, mMainPanel.getHeight());

    mLogGui.setPosition(colWidth, 0);
    mLogGui.setWidthElements(colWidth);
    mLogGui.setSize(colWidth, mMainPanel.getHeight());


}

void Movled::loadMoviesFromFile() {
    
    ofFile moviesFile(ofToDataPath("movies.csv"));
    ofBuffer buffer = moviesFile.readToBuffer();
    
    auto getParts = [this](string line, shared_ptr<Movie> output) {
        auto parts = ofSplitString(line, ",");
        if (parts.size() >= 3) {
            output->name = ofTrim(parts[0]);
            output->path.set(ofTrim(parts[1]));
            output->midiNote.set("note", ofFromString<int>(ofTrim(parts[2])), 0, 127);
            return true;
        }
        return false;
    };
    
    int lineNum = 0;
    for (auto line : buffer.getLines()) {
        if (lineNum > 0) {
            
            if (line.substr(0, 2) == "//") {
                cout << "skipping " << line << endl;
                continue;
            }
        
            auto movie = make_shared<Movie>();
            if (getParts(line, movie)) {
                if (movie->video.load(movie->path)) {
                    mMovies.push_back(movie);
                    addMovieToGui(movie);
                    logMessage("added for " + movie->name);
                }
                else {
                    logMessage("can't load video at " + string(movie->path));
                }
            }
            else if (line.length() > 1) { // don't complain on empty line
                logMessage("not enough info on line " + ofToString(lineNum + 1));
            }
            
        }
        lineNum++;
        
    }
    
    
}

void Movled::addMovieToGui(shared_ptr<Movie> movie) {
    
    
    shared_ptr<ofxGuiGroup> movieGroup = make_shared<ofxGuiGroup>();
    movieGroup->setup(movie->name);

    
    movie->triggerPlay.addListener(this, &Movled::playTrack);
    movieGroup->add(movie->triggerPlay);
//    movieGroup->add(movie->path);
    movieGroup->add(movie->midiNote);
    
    mMovieGroups.push_back(movieGroup);
    mMainPanel.add(movieGroup.get());
    
}

/////////// MIDI

void Movled::newMidiMessage(ofxMidiMessage& msg) {
    
    if (int(mSettings["midiChannel"]) == 0 || msg.channel == int(mSettings["midiChannel"])) {
        if (msg.status == MIDI_NOTE_ON) {
            bool foundMovie = false;
            for (size_t i = 0; i < mMovies.size() && !foundMovie; i++) {
                if (mMovies[i]->midiNote == msg.pitch) {
                    playMovieAtIndex(i);
                    foundMovie = true;
                }
            }
            if (!foundMovie) {
                logMessage("received note on for pitch " + ofToString(msg.pitch));
            }
        }
    }
}

void Movled::midiInputDeviceChange(ofxRadioGroupEventArgs &args) {
    midiIn.closePort();
    midiIn.openPort(args.name);
}

//////////// Logging

void Movled::logMessage(string message) {

    auto timestamp = ofGetTimestampString("%H:%M.%S");
    
    mLogMessages.push_front(timestamp + " | " + message);
    
    while (mLogMessages.size() > 10) {
        mLogMessages.pop_back();
    }
    
    mLogGui.clear();
    for (auto &message : mLogMessages) {
        mLogGui.add(message);
    }
}

//////////// Settings

void Movled::loadSettings() {
    std::ifstream f(ofToDataPath("settings.json"));
    mSettings = nlohmann::json::parse(f);
    
}

void Movled::setWiringCache(string scheme, int width, int height) {
    
    mWiringCache.resize(width * height);

    if (scheme == "rowMajor") {
        for (int y = 0, k = 0; y < height; y++) {
            for (int x = 0; x < width; x++, k++) {
                mWiringCache[k] = x + y * width;
            }
        }
    }
    else if (scheme == "columnMajor") {
        for (int x = 0, k = 0; x < width; x++) {
            for (int y = 0; y < height; y++, k++) {
                mWiringCache[k] = x + y * width;
            }
        }
    }
    else {
        logMessage("don't understand " + scheme + " wiring scheme");
    }
    
}

void Movled::connectToDmxDevice() {
    ofSerial serial;
    string dmxDevice;
    auto devices = serial.getDeviceList();
    for (ofSerialDeviceInfo device : devices) {
        string name = device.getDeviceName();
        if (name.find("tty.usb") != string::npos) {
            dmxDevice = device.getDevicePath();
        }
    }
    
    if (dmxDevice.find("dev") != string::npos) {
        mDmx.connect(dmxDevice, 512);
        logMessage("connected DMX device at " + dmxDevice);
    }
}

void Movled::playTrack(bool &b) {
    for (size_t i = 0; i < mMovies.size(); i++) {
        auto movie = mMovies[i];
        if (&b == &movie->triggerPlay.get()) {
            playMovieAtIndex(i);
        }
    }
}

void Movled::playMovieAtIndex(size_t index) {

    if (mCurrentMovieIndex != DONT_PLAY) {
        mMovies[mCurrentMovieIndex]->video.stop();
    }

    mCurrentMovieIndex = index;
    mMovies[index]->video.setFrame(0);
    mMovies[index]->video.play();
    logMessage("started playing " + mMovies[index]->name);
}

void Movled::stopMovie() {
    
    if (mCurrentMovieIndex != DONT_PLAY) {
        mMovies[mCurrentMovieIndex]->triggerPlay = false;
        mMovies[mCurrentMovieIndex]->video.stop();
        logMessage("stopped " + mMovies[mCurrentMovieIndex]->name);
    }
    
    mCurrentMoviePosition = 0;
    
    
    
    mCurrentMovieIndex = DONT_PLAY;
    
    
}