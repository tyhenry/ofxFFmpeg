# ofxFFmpeg

A simplified `ffmpeg` video recorder.  
Spawns `ffmpeg` in a subprocress and pipes video frames to it at a constant framerate.

Code is a rewrite of [`ofxFfmpegRecorder`](https://github.com/Furkanzmc/ofxFFmpegRecorder).

# Features

- Record video by adding `ofPixels`

# Todo

- Add audio support (currently only video recording is supported)

# How to Use

 - See the example.

# Dependencies

 - openFrameworks 0.10+

 - `ffmpeg`  
  By default `ofxFFmpeg` will call `ffmpeg` from your PATH.  
  You may alternatively specify the full path to a local `ffmpeg` binary in `ofxFFmpeg::RecorderSettings`.  
  `ffmpeg` binaries are included in the `libs` directory for portability.

