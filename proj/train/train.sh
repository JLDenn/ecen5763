#!/bin/bash

set -x
set -e

dir=`dirname $0`

#Mannually annotate all of the positive images
#opencv_annotation -a=$dir/chicks.txt -i=$dir/frames/

make -C $dir/annotations
$dir/annotations/sort -r=$dir $dir/chicks.txt

make -C $dir/trainer
$dir/trainer/train -dw=128 -dh=128 -pd=$dir/annotations/pos -nd=$dir/annotations/neg -td=$dir/annotations/pos -fn=chick.xml -d -f