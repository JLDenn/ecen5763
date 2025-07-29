#!/bin/bash

set -e

echo "Training requires a neg.txt with the relative paths (to this scripts directory) to the negative images that contain no objects of interest"
echo "It uses each of the images in annotations/pos/ directory to create the vec file that is then used to train the cascade.xml"


dir=`dirname $0`
positives="$dir/positives.txt"

posFileCount=`ls -1q $dir/annotations/pos/* | wc -l`
negFileCount=`cat $dir/neg.txt | wc -l`
let posImgCount=posFileCount*negFileCount
echo "Processing $posFileCount positive images and $negFileCount negative images..."

# rm -rf "$positives"
# for filename in $dir/annotations/pos/*; do
	# base=`basename $filename`
	
	# mkdir -p "$dir/out_$base"

	# $dir/opencv/build/bin/opencv_createsamples -img "$filename" -bg "$dir/neg.txt" -info "$dir/out_$base/info.txt" -num 128 -maxxangle 0.0 -maxyangle 0.0 -maxzangle 0.3 -bgcolor 78 -bgthresh 2 -w 64 -h 64
	
	# cat "$dir/out_$base/info.txt" | sed -e "s|^|out_$base/|" >> "$positives"
# done


# echo "Generating the vec file..."
# $dir/opencv/build/bin/opencv_createsamples -info "$positives" -bg "$dir/neg.txt" -vec "$dir/cascade.vec" -num $posImgCount -w 64 -h 64

# echo "Training..."

let posImgCount=$posImgCount-200
mkdir -p "$dir/cascade_out"
$dir/opencv/build/bin/opencv_traincascade -data "$dir/cascade_out" -vec "$dir/cascade.vec" -bg "$dir/neg.txt" -numPos $posImgCount -numNeg $negFileCount -numStages 20 -precalcValBufSize 1024 -precalcIdxBufSize 1024 -featureType LBP -minHitRate 0.995 -maxFalsAlarmRate 0.5 -w 64 -h 64

rm -rf "$dir/out_*.jpg"

echo "Complete. Results should be here: $dir/cascade_out/cascade.xml"

