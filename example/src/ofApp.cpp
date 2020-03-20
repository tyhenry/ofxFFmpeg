#include "ofApp.h"

void ofApp::setup()
{

	ofSetLogLevel( "ofxFFmpeg", OF_LOG_VERBOSE );

	webcam.setup( 640, 480 );

	// we'll draw into the fbo and then read back the pixels to record
	fbo.allocate( webcam.getWidth(), webcam.getHeight(), GL_RGB );

	font.load( "Courier New", 48 );
}

void ofApp::update()
{
	webcam.update();
}

void ofApp::draw()
{

	// choose a color to represent the recorder state
	ofColor recorderStateColor;
	if ( m_recorder.isRecording() ) {
		recorderStateColor = ofColor::red;
	} else if ( m_recorder.isReady() ) {
		recorderStateColor = ofColor::green;
	} else {
		// if not recording, but not ready, it's still processing a previous recording
		recorderStateColor = ofColor::yellow;
	}

	// draw into the fbo
	fbo.begin();
	{
		ofPushStyle();

		ofSetColor( 255 );
		webcam.draw( 0, 0, fbo.getWidth(), fbo.getHeight() );
		ofSetColor( recorderStateColor );

		// draw a pulsing circle
		ofDrawCircle( fbo.getWidth() / 2, fbo.getHeight() / 2, ( ( sin( ofGetElapsedTimef() * 6. ) * 0.5 + 0.5 ) + 0.5 ) * 100 + 20 );

		// draw a timestamp
		ofScale( fbo.getWidth() / 640, fbo.getHeight() / 480 );
		font.drawString( ofToString( m_recorder.isRecording() ? m_recorder.getRecordedDuration() : 0, 2 ) + "s", 20, 440 );

		ofPopStyle();
	}
	fbo.end();

	// add frame to the recording
	if ( m_recorder.isRecording() ) {

		fbo.readToPixels( mPix );  // use ofxFastFboReader to speed up fbo -> pixels read
		if ( mPix.getWidth() > 0 && mPix.getHeight() > 0 ) {
			m_recorder.addFrame( mPix );	// call this as often as possible, the recorder will accept the pixels when its ready for a new frame
		}
	}

	fbo.draw( 0, 0 );

	ofDrawBitmapStringHighlight( std::to_string( m_recorder.getRecordedDuration() ), 40, 45 );
	ofDrawBitmapStringHighlight( "FPS: " + std::to_string( ofGetFrameRate() ), 10, 16 );

	ofPushStyle();

	ofSetColor( recorderStateColor );

	ofDrawCircle( 10, 40, 10 );

	ofDrawBitmapStringHighlight( "Press spacebar to toggle recording.", 10, ofGetHeight() - 200 );
	ofPopStyle();
}

void ofApp::keyReleased( int key )
{

	if ( key == ' ' ) {
		// toggle recording
		if ( m_recorder.isRecording() ) {
			m_recorder.stop();

		} else if ( m_recorder.isReady() ) {
			m_recorder.start( m_recorderSettings );
		}
	}
}
