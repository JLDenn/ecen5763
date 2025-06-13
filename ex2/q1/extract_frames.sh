#!/bin/bash

echo "Export the first 100 image frames from a source video"

if [ "$#" -ne 2 ]; then
	echo "Usage: $0 <path to video> <output folder>"
	exit 1
fi

set -x
ffmpeg -i "$1" -vf trim=start_frame=0:end_frame=100 "$2/bbb_%03d.ppm"
