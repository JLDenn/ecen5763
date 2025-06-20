#!/bin/bash

set -e

path=`dirname $0`
make -C "$path"

mkdir -p "$path/frames"
ffmpeg -i "$path/../Dark-Room-Laser-Spot-with-Clutter.mpeg"  "$path/frames/dark_%04d.ppm"
"$path/../q5/convert" "$path/frames/dark_%04d.ppm" R

"$path/track" "$path/frames/gray_%04d.pgm" 

ffmpeg -framerate 30 -i "$path/frames/marked_%04d.pgm" -r 30 "$path/track.mpeg"

echo "Deleting individual frame images"
rm -r "$path/frames"