#include "ofMain.h"
#include "Movled.h"

int main( ){
    
    ofGLFWWindowSettings windowSettings;
    windowSettings.setGLVersion(3, 2);
    
    windowSettings.resizable = true;
    
    windowSettings.width = 800;  //settings["projector"]["width"];
    windowSettings.height = 600;// settings["projector"]["height"];
    windowSettings.setPosition(ofVec2f(200, 0));
    windowSettings.windowMode = OF_WINDOW; // OF_FULLSCREEN;
    shared_ptr<ofAppBaseWindow> mainWindow = ofCreateWindow(windowSettings);
    
    
    shared_ptr<Movled> mainApp(new Movled);
    
    ofRunApp(mainWindow, mainApp);
    ofRunMainLoop();
    
}
