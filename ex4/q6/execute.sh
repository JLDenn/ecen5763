#!/bin/bash

set -e

vidFile="../q5/2012-08-30-RoseBowl.mp4"

# start by ensuring the sample video is available
if [ ! -f "$vidFile" ]; then
	echo "The file 2012-08-30-RoseBowl.mp4 must be downloaded and available in the q5/ folder before executing"
	exit 1
fi

if [ ! -f "/usr/bin/opencv_annotation" ]; then
	echo "opencv_annotation must be installed (using something like 'sudo apt install libopencv-dev')"
	exit 1;
fi


echo "Running detection video... press 's' to save an image (which will be available for selecting objects later)."
echo "Ensure you collect both positive and negative images"
echo "Hit ESC when finished"
mkdir -p detect/frames
rm -rf detect/frames/*
make -C detect
detect/hog -v="$vidFile"



echo "Executing opencv_annotation to allow you to select objects from the captured frame images"
opencv_annotation -a="train/annotations/people.txt" -i=detect/frames/

echo "Sorting out the images into negative and positive folders (and cropping and resizing positives)"
make -C train/annotations
mkdir -p train/annotations/pos
mkdir -p train/annotations/neg
rm -rf train/annotations/pos/*
rm -rf train/annotations/neg/*

train/annotations/sort train/annotations/people.txt



echo "Perform the training of the SVM model using the images prepared"
make -C train
train/train -dw=64 -dh=128 -pd=train/annotations/pos -nd=train/annotations/neg -td=detect/frames -fn=train/ped64x128.xml -d -f


echo "Re-running the object detection using the new model"
detect/hog -v="$vidFile"