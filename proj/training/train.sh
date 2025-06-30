#!/bin/bash

#Mannually annotate all of the positive images
opencv_annotation -a=pos.txt -i=pos/

# Convert all the annotated images to a vector file for all the positive samples
opencv_createsamples -info ./pos.txt -w 24 -h 24 -num 10000 -vec pos.vec

# Create folder for the output of the training 
mkdir -p cascade

# w/h must match above
# numPos must be < num boxes annotated (by a few)
# numNeg can be played with, "online best" is 2:1 - 1:2 (relative to the numPos)
# numStages more=better, to a point
opencv_traincascade -data cascade -vec pos.vec -bg bg.txt -w 24 -h 24 -numPos 150 -numNeg 75 -numStages 10 
