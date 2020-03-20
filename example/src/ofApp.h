#pragma once
#include "ofMain.h"
#include "ofxFFmpeg.h"
#include "ofVideoGrabber.h"

class ofApp : public ofBaseApp{ 
public:

    void setup();
    void update();
    void draw();

    void keyReleased(int key);

private:
    ofxFFmpeg::Recorder m_recorder;
    ofxFFmpeg::RecorderSettings m_recorderSettings;
    ofVideoGrabber webcam;
    
    ofFbo fbo;
    ofPixels mPix;
	ofTrueTypeFont font;

};
