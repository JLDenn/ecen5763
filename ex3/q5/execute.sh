#!/bin/bash

path=`dirname $0`
make -C "$path"

mkdir -p "$path/frames"
ffmpeg -i "$path/../Dark-Room-Laser-Spot-with-Clutter.mpeg" -vf "scale=480:270" "$path/frames/dark_%04d.ppm"
"$path/convert" "$path/frames/dark_%04d.ppm" R

ffmpeg -framerate 30 -i "$path/frames/gray_%04d.pgm" -r 30 gray.mpeg

echo "Deleting individual frame images"
rm -r "$path/frames"