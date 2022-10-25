# FFmpeg-folder
FFmpeg-folder is a simple Windows Context Menu tool that allows you to right click on a folder and reduce the filesize of all videos in the folder and all subfolders.

![example picture with right click context menu and "reduce filesize of videos" circled](https://i.imgur.com/A2dsPXx.png)

This was made because I have hundreds of 5min video game/desktop recording replays in their native format (hardware-encoded H.264 and H.265), which is significantly larger than the video codec this program uses (CPU libx265)

On average, a 5 minute replay was originally at ~1GB and reduced down to ~150MB, with no noticeable quality loss
## Features
- Video file tagging, so you don't re-encode files on later runs
- "Date Created" "Date Modified" "Date Accessed" attribute preservation for the new files
- Simple installer to add it to the right-click menu and forget about it
- Ctrl + C handling so you aren't left with a half-done file

## Video transcoding
FFmpeg-folder uses (you guessed it) [FFmpeg](https://ffmpeg.org/) to transcode files to the [H.265](https://trac.ffmpeg.org/wiki/Encode/H.265) video codec

H.265 was chosen because it's a pretty good middle ground between time & space efficiency for encoding and filesize. I specifically used the `-crf 28` and `-preset fast` because H.265 CRF 28 is visually equivalent to H.264 CRF 23, with half the file size, and H.264 CRF 23 is decently close to visually lossless.

On files specifically encoded for compactness, this might do very little to reduce filesize, or even increase it. Things like Movies & TV Shows are encoded more efficently than I could ever achieve.

## Dependencies
Only works on Windows, only tested on Windows 10

FFmpeg and FFprobe are the only dependencies, and you need to have the binary files on your PATH. I might figure out how to bundle them with the installer and see if I can fix that issue, but license stuff with that will be hard to figure out

## Installer
I created an installer with this program, so if you just download the release's `ffmpeg-folder-installer.msi` and run it, you will have the context menu item when you right click folders

## Disclaimer
This program sucks safety-wise. I just threw it all together over the course of a couple hours to make something work for my specific use case. If you use it on any files with non-latin characters in their filepath, it might break. If you use it on any files with the "comment" metadata field filled out, it will overwrite it.
