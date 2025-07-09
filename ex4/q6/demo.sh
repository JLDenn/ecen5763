#!/bin/bash

vidFile="../q5/2012-08-25-RoseBowl.mp4"

# start by ensuring the sample video is available
if [ ! -f "$vidFile" ]; then
	echo "The file 2012-08-25-RoseBowl.mp4 must be downloaded and available in the q5/ folder before executing"
	exit 1
fi


echo "Run the object detection using the saved model"
detect/hog -v="$vidFile"