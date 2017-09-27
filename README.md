# pnacl_player
H.264 video player using Chrome's portable native client (pluginless native code in Chrome)

This project is a refactored and extended version of the video_decode example project from the Google Native Client SDK.

## Building

To build this project:

1) Install the Google Native Client SDK.
2) Download the source of this project by whatever method you prefer (such as the `Clone or download` button on github).
3) Extract/copy/move the project folder into `nacl_sdk/pepper_xx/examples/api/` where pepper_xx refers to your preferred stable version of the SDK.  For example on my system this readme file is located at `nacl_sdk/pepper_49/examples/api/pnacl_player/README.md`.  I don't know how important it is that you run the project from here, but that is how I do it.
3) Build by running "make.bat" in this project folder.  The VS solution is also wired up so you can use the BUILD menu in Visual Studio (created and tested in VS 2017 Community Edition).
4) Finalized, compressed output appears in the FinishedOutput subdirectory.

## Planned Features

* Proper frame timing, rather than decoding and rendering frames with no delay.  Some of the foundation is in place for this already.
* Audio playback of some sort.

## Example Implementation

For an example implementation, see this project: https://github.com/bp2008/ui3
