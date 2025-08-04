/**
	This code can be used to record raw image frames from the camera. These frames can be used to assemble a video that can be 
	processed by the defender app at a later time (likely using the dry -D option)
*/

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <unistd.h>
#include <iostream>
#include <string>

using namespace std;
using namespace cv;

#define IMG_WIDTH			1280
#define IMG_HEIGHT			720

struct {
	int frameCount;
	const char *frameNameFormat;
	bool noDisplay;
	float frameRate;
}opts = {
		.frameCount = 0, 
		.frameNameFormat = "frames/f%05d.jpg", 
		.noDisplay = false, 
		.frameRate = 120.0,
};


/** @function main */
int main( int argc, char* const* argv )
{

	int opt;
	while( (opt = getopt(argc, argv, "c:n:dhr:") ) != -1){
		switch(opt){
		case 'c':
			opts.frameCount = atoi(optarg);
			cout << "Overriding frame count: " << opts.frameCount << endl;
			break;
		case 'n':
			opts.frameNameFormat = optarg;
			cout << "Overriding frame name format: " << opts.frameNameFormat << endl;
			break;
			
		case 'd':
			opts.noDisplay = true;
			cout << "Disabling live view" << endl;
			break;
			
		case 'r':
			opts.frameRate = atof(optarg);
			cout << "Limiting frame output rate to " << opts.frameRate << " fps" << endl;
			break;

		case 'h':
		default:
			fprintf(stderr, "Usage: %s \n"
				"\t[-c <frameCount> = number of frames to record]\n"
				"\t[-n <frameNameFormat> = name format to use for recorded frames (default: %s)]\n"
				"\t[-d = disable live view (allows for recording over ssh connection)]\n"
				"\t[-r <maxFrameRate> = Maximum frame rate output while recording (default: %3.2f)]\n",
				argv[0], opts.frameNameFormat, opts.frameRate);
			exit(-1);
		}
	}

	VideoCapture cap(0);
	if(!cap.isOpened()){
		cout << "Error opening the camera" << endl;
		return -1;
	}

	cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);

	struct timespec ts = {0}, ts_last = {0};

	int captureCount = opts.frameCount > 0 ? opts.frameCount : 0;
	Mat frame;
	int frameIndex = 0;

	while(!captureCount || frameIndex < captureCount){
		cap >> frame;
		if(frame.empty()){
			cout << "Error reading image frame" << endl;
			return -1;
		}
		
		clock_gettime(CLOCK_MONOTONIC, &ts);
		double dt = ((double)ts.tv_sec + ts.tv_nsec/1000000000.0) -  ((double)ts_last.tv_sec + ts_last.tv_nsec/1000000000.0);
		if(dt > 1/opts.frameRate){
			frameIndex++;
			
			char outname[256];
			snprintf(outname, sizeof(outname), opts.frameNameFormat, frameIndex);
			imwrite(outname, frame);
			
			ts_last = ts;
		}
		
		if(!opts.noDisplay){
			imshow("Live", frame);
			if(waitKey(1) == 27)
				break;
		}
	}


	cap.release();
    return 0;
}




