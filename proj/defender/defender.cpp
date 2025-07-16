/**
	@author	Justin Denning
	@date	12 July 2025
	
	@Description

	
**/

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <chrono>

#include "pantilt.hpp"

//Define the image size
#define IMG_WIDTH 	1280
#define IMG_HEIGHT	720


//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME		"Live Video"

//Define the application quit key (esc)
#define ESCAPE 		27

struct {
	bool test;
	char *video;		//NULL (no -v argument) indicates use camera 
	int minArea;
	int motionExpansion;	
}opts = {};

using namespace cv;
using namespace std;




int findMotion(Mat frame, Mat *avg, vector<vector<Vec2i>> *contours){
	
	Mat blurred;
	Mat temp;

	
	if(avg->empty()){
		GaussianBlur(frame, blurred, Size(21, 21), 0);
		blurred.convertTo(*avg, CV_32F, 1.0/255.0);
		return 0;
	}

	avg->convertTo(temp, CV_8UC1);
	GaussianBlur(frame, blurred, Size(21, 21), 0);
		
	Mat diff, threshFrame;
	absdiff(blurred, temp, diff);
	accumulateWeighted(blurred, *avg, 0.2);
	
	threshold( diff, threshFrame, 40, 255, THRESH_BINARY);
	
	Mat dilateFrame;
	dilate(threshFrame, dilateFrame, getStructuringElement(MORPH_RECT, Size(3,3)), Point(-1,-1), 20);
	
	vector<vector<Vec2i>> rawContours;
	findContours(dilateFrame, rawContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	
	for(int i=0;i<rawContours.size();i++){
		if(contourArea(rawContours[i]) > opts.minArea)
			contours->push_back(rawContours[i]);
	}

	return contours->size();
}






int main(int argc, char *argv[]){
	
	//------------------------------------------------------------------
	//	Process CLI arguments
	int opt;
	//Defaults
	opts.minArea = 1000;
	opts.motionExpansion = 10;
	
	while( (opt = getopt(argc, argv, "tv:a:e:") ) != -1){
		switch(opt){
		case 't':
			opts.test = true;
			break;
		case 'v':
			opts.video = optarg;
			cout << "Using " << opts.video << endl;
			break;
			
		case 'a':
			opts.minArea = atoi(optarg);
			break;
			
		case 'e':
			opts.motionExpansion = atoi(optarg);
			break;
		
		default:
			fprintf(stderr, "Usage: %s \n\t[-t = test run] \n\t[-v video/path = use video file] \n\t[-a minArea = motion threshold] \n\t[-e expansion = distance to expand motion rect]", argv[0]);
			exit(-1);
		}
	}
	
	//Instantiate the pan & tilt object (and run tests is directed)
	PanTilt pt;
	if(opts.test){
		if(pt.test())
			exit(-1);
	}
	
	

	//Create the capture instance which will open the camera
	VideoCapture cap;
	if(opts.video)
		cap.open(opts.video);
	else
		cap.open(0);
	
	if(!cap.isOpened()){
		cout << "Unable to open video/camera for input" << endl;
		exit(-1);
	}
	
	//Configure the camera frame dimensions
	if(!opts.video){
		cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
		cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
	}
	
	//Create our display window
	namedWindow(WINDOW_NAME);
	
	
	Mat avgBackgroundFrame;
	Mat clrFrame;
	
	while(1){
		//Read frame and verify it is valid
		cap >> clrFrame;
		if(clrFrame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		Mat grayFrame;
		cvtColor(clrFrame, grayFrame, COLOR_BGR2GRAY);
		
		vector<vector<Vec2i>> contours;
		int count = findMotion(grayFrame, &avgBackgroundFrame, &contours);
		
		if(count)
			cout << "Found " << count << " contours" << endl;
		
		Mat annotatedFrame;
		clrFrame.copyTo(annotatedFrame);
		
		for(int i=0;i<count;i++){
			Rect r = boundingRect(contours.at(i));
			rectangle(annotatedFrame, r, Scalar(0,255,0), 2);
			
			
			//Motion detected in rectangle r, check for chicken
			r.x -= opts.motionExpansion;
			if(r.x < 0) r.x = 0;
			r.y -= opts.motionExpansion;
			if(r.y < 0) r.y = 0;
			r.width += opts.motionExpansion*2;
			if(r.x + r.width > annotatedFrame.cols) r.width = annotatedFrame.cols-r.x;
			r.height += opts.motionExpansion*2;
			if(r.y + r.height > annotatedFrame.rows) r.height = annotatedFrame.rows-r.y;
			
			Mat motionSubFrame(grayFrame, r);
			imshow("ROI", motionSubFrame);
		}
	
		
		
		//Display the image
		imshow(WINDOW_NAME, annotatedFrame);
		
		//Give the system time to actually render the image to the screen, and check for an escape key press
		//	which will cause us to quit. 
		if(waitKey(1) == ESCAPE)
			break;
	}

	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME);


	
	//Return success
	return 0;
}