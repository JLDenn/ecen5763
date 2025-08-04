#!/bin/bash

set -e

dir=`dirname $0`

echo "This command creates an annotation file that will then be used by $dir/annotations/sort to extract positive images for training"
echo "This script will overwrite $dir/chicken_annotations.txt as objects are manually annotated."

while true; do
    read -p "Proceed with annotation? (y/n): " choice
    case $choice in
        [yY]* ) echo "Running opencv_annotation..."; break;;
        [nN]* ) echo "Aborting"; exit;;
        * ) echo "Invalid input. Please answer 'y' or 'n'.";;
    esac
done

opencv_annotation -images=$dir/frames/ -annotations=$dir/chicken_annotations.txt

rm -rf $dir/annotations/pos/*
rm -rf $dir/annotations/neg/*
make -C $dir/annotations/
$dir/annotations/sort $dir/chicken_annotations.txt

echo "Use images in annotations/pos/ & annotations/neg/ for training using the windows tool \"Cascade Trainger GUI (Version 3.3.1)\""
