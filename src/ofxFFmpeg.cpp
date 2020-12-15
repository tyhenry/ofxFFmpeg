#include "ofxFFmpeg.h"
// openFrameworks
#include "ofLog.h"
#include "ofSoundStream.h"
#include "ofVideoGrabber.h"

// Logging macros
#define LOG_ERROR() ofLogError( "ofxFFmpeg" ) << __FUNCTION__ << ": "
#define LOG_WARNING() ofLogWarning( "ofxFFmpeg" ) << __FUNCTION__ << ": "
#define LOG_NOTICE() ofLogNotice( "ofxFFmpeg" ) << __FUNCTION__ << ": "
#define LOG_VERBOSE() ofLogVerbose( "ofxFFmpeg" ) << __FUNCTION__ << ": "
#define LOG() LOG_NOTICE()

// Subprocess macros
#if defined( _WIN32 )
#define P_CLOSE( file ) _pclose( file )
#define P_OPEN( cmd ) _popen( cmd, "wb" )  // write binary?
#else
#define P_CLOSE( file ) pclose( file )
#define P_OPEN( cmd ) popen( cmd, "w" )
#endif

namespace ofxFFmpeg {

// -----------------------------------------------------------------
Recorder::Recorder()
{
}

// -----------------------------------------------------------------
Recorder::~Recorder()
{
	stop();
	if ( m_thread.joinable() ) m_thread.join();
}

// -----------------------------------------------------------------
bool Recorder::start( const RecorderSettings &settings )
{

	if ( isRecording() ) {
		LOG_WARNING() << "Can't start recording - already started.";
		return false;
	}

	if ( !isReady() ) {
		LOG_ERROR() << "Can't start recording - previous recording is still processing.";
		return false;
	}

	if ( settings.outputPath.empty() ) {
		LOG_ERROR() << "Can't start recording - output path is not set!";
		return false;
	}

	if ( ofFile::doesFileExist( ofToDataPath( m_settings.outputPath, true ), false ) && !m_settings.allowOverwrite ) {
		LOG_ERROR() << "The output file already exists and overwriting is disabled. Can't record to file: " << m_settings.outputPath;
		return false;
	}

	m_settings = settings;

	if ( m_settings.ffmpegPath.empty() ) {
		m_settings.ffmpegPath = "ffmpeg";
	}

	m_nAddedFrames = 0;

	std::string cmd               = m_settings.ffmpegPath;
	std::vector<std::string> args = {
	    "-y",   // overwrite
	    "-an",  // disable audio -- todo: add audio,

	    // input
	    "-r " + ofToString( m_settings.fps ),                      // input frame rate
	    "-s " + std::to_string( m_settings.videoResolution.x ) +   // input resolution x
	        "x" + std::to_string( m_settings.videoResolution.y ),  // input resolution y
	    "-f rawvideo",                                             // input codec
	    "-pix_fmt rgb24",                                          // input pixel format
	    m_settings.extraInputArgs,                                 // custom input args
	    "-i pipe:",                                                // input source (default pipe)

	    // output
	    "-r " + ofToString( m_settings.fps ),              // output frame rate
	    "-c:v " + m_settings.videoCodec,                   // output codec
	    "-b:v " + ofToString( m_settings.bitrate ) + "k",  // output bitrate kbps (hint)
	    m_settings.extraOutputArgs,                        // custom output args
	    m_settings.outputPath                              // output path
	};

	for ( const auto &arg : args ) {
		if ( !arg.empty() ) cmd += " " + arg;
	}

	if ( m_ffmpegPipe != nullptr ) {
		P_CLOSE( m_ffmpegPipe );
	}

	LOG() << "Starting recording with command...\n\t" << cmd << "\n";

	m_ffmpegPipe = P_OPEN( cmd.c_str() );

	if ( !m_ffmpegPipe ) {
		LOG_ERROR() << "Unable to start recording. Error: " << std::strerror( errno );
		return false;
	}
	return m_isRecording = true;
}

// -----------------------------------------------------------------
void Recorder::stop()
{
	m_isRecording = false;
}

bool Recorder::wantsFrame()
{
	if ( m_isRecording && m_ffmpegPipe ) {
		const float delta          = Seconds( Clock::now() - m_recordStartTime ).count() - getRecordedDuration();
		const size_t framesToWrite = delta * m_settings.fps;
		return framesToWrite > 0;
	}
	return false;
}

// -----------------------------------------------------------------
size_t Recorder::addFrame( const ofPixels &pixels )
{
	if ( !m_isRecording ) {
		LOG_ERROR() << "Can't add new frame - not in recording mode.";
		return 0;
	}

	if ( !m_ffmpegPipe ) {
		LOG_ERROR() << "Can't add new frame - FFmpeg pipe is invalid!";
		return 0;
	}

	if ( !pixels.isAllocated() ) {
		LOG_ERROR() << "Can't add new frame - input pixels not allocated!";
		return 0;
	}

	if ( m_nAddedFrames == 0 ) {
		if ( m_thread.joinable() ) m_thread.join();  //detach();
		m_thread          = std::thread( &Recorder::processFrame, this );
		m_recordStartTime = Clock::now();
		m_lastFrameTime   = m_recordStartTime;
	}

	// add new frame(s) at specified frame rate
	const float delta          = Seconds( Clock::now() - m_recordStartTime ).count() - getRecordedDuration();
	const size_t framesToWrite = delta * m_settings.fps;
	size_t written             = 0;
	ofPixels *pixPtr           = nullptr;

	// drop or duplicate frames to maintain constant framerate
	while ( m_nAddedFrames == 0 || framesToWrite > written ) {

		if ( !pixPtr ) {
			pixPtr = new ofPixels( pixels );  // copy pixel data
		}

		if ( written == framesToWrite - 1 ) {
			// only the last frame we produce owns the pixel data
			m_frames.produce( pixPtr );
		} else {
			// otherwise, we reference the data
			ofPixels *pixRef = new ofPixels();
			pixRef->setFromExternalPixels( pixPtr->getData(), pixPtr->getWidth(), pixPtr->getHeight(), pixPtr->getPixelFormat() );  // re-use already copied pointer
			m_frames.produce( pixRef );
		}

		++m_nAddedFrames;
		++written;
		m_lastFrameTime = Clock::now();
	}

	return written;
}

// -----------------------------------------------------------------
void Recorder::processFrame()
{
	while ( isRecording() ) {

		TimePoint lastFrameTime = Clock::now();
		const float framedur    = 1.f / m_settings.fps;

		while ( m_frames.size() ) {  // allows finish processing queue after we call stop()

			// feed frames at constant fps
			float delta = Seconds( Clock::now() - lastFrameTime ).count();

			if ( delta >= framedur ) {

				if ( !isRecording() ) {
					LOG_VERBOSE() << "Recording stopped, but finishing frame queue - " << m_frames.size() << " remaining frames at " << m_settings.fps << " fps";
				}

				ofPixels *pixels = nullptr;

				if ( m_frames.consume( pixels ) && pixels ) {
					const unsigned char *data = pixels->getData();
					const size_t dataLength   = m_settings.videoResolution.x * m_settings.videoResolution.y * 3;
					const size_t written      = m_ffmpegPipe ? fwrite( data, sizeof( char ), dataLength, m_ffmpegPipe ) : 0;

					if ( written <= 0 ) {
						LOG_WARNING() << "Unable to write the frame.";
					}

					pixels->clear();
					delete pixels;

					lastFrameTime = Clock::now();
				}
			}
		}
	}

	// close ffmpeg pipe once stopped recording

	if ( m_ffmpegPipe ) {
		if ( P_CLOSE( m_ffmpegPipe ) < 0 ) {
			LOG_ERROR() << "Error closing FFmpeg pipe: " << std::strerror( errno );
		}
	}

	m_ffmpegPipe   = nullptr;
	m_nAddedFrames = 0;
}
}  // namespace ofxFFmpeg
