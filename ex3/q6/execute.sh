#!/bin/bash

set -e

path=`dirname $0`
make -C "$path"

# create a directory to hold the intermediate frames
mkdir -p "$path/frames"
# extract the video frames
ffmpeg -i "$path/../Dark-Room-Laser-Spot-with-Clutter.mpeg" "$path/frames/dark_%04d.ppm"
# convert the color images frames to grayscale using Red channel
"$path/../q5/convert" "$path/frames/dark_%04d.ppm" R

# generate the object tracking image frames with the cross/box overlay
"$path/track" "$path/frames/gray_%04d.pgm" 
# encode the marked/tracked frames back into a video
ffmpeg -framerate 30 -i "$path/frames/marked_%04d.pgm" -r 30 "$path/track.mpeg"

echo "Deleting individual frame images"
rm -r "$path/frames"