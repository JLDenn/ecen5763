#!/bin/bash


path=`dirname $0`

make -C "$path"

mkdir -p "$path/frames"
ffmpeg -i "$path/../Dark-Room-Laser-Spot-with-Clutter.mpeg" -vf "scale=480:270" "$path/frames/dark_%04d.ppm"
"$path/detect" "$path/frames/dark_%04d.ppm"
rm frames/*.ppm

ffmpeg -i "$path/../Light-Room-Laser-Spot-with-Clutter.mpeg" -vf "scale=480:270" "$path/frames/light_%04d.ppm"
"$path/detect" "$path/frames/light_%04d.ppm"
rm frames/*.ppm

