#include "ofMain.h"
#include "ofApp.h"

int main()
{
    // Setup the GL context
    // Can be OF_WINDOW or OF_FULLSCREEN
    // Pass in width and height too:
    ofSetupOpenGL(640*2, 480*2, OF_WINDOW);

    // This kicks off the running of my app
    ofRunApp(new ofApp());

}
