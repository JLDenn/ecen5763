/**
	The base for this code came from code that was initially used for Exercise 1 (face detection)
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
}opts = {.frameCount = 0, .frameNameFormat = "frames/f%05d.jpg", .noDisplay = false};


/** @function main */
int main( int argc, char* const* argv )
{

	int opt;
	while( (opt = getopt(argc, argv, "c:n:dh") ) != -1){
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

		case 'h':
		default:
			fprintf(stderr, "Usage: %s \n"
				"\t[-c <frameCount> = number of frames to record] \n"
				"\t[-n <frameNameFormat> = name format to use for recorded frames (default: %s)\n",
				argv[0], opts.frameNameFormat);
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


	int captureCount = opts.frameCount > 0 ? opts.frameCount : 0;
	Mat frame;
	int frameIndex = 0;

	while(!captureCount || frameIndex < captureCount){
		cap >> frame;
		if(frame.empty()){
			cout << "Error reading image frame" << endl;
			return -1;
		}
		
		frameIndex++;
		
		char outname[256];
		snprintf(outname, sizeof(outname), opts.frameNameFormat, frameIndex);
		imwrite(outname, frame);
		
		if(!opts.noDisplay){
			imshow("Live", frame);
			if(waitKey() == 27)
				break;
		}
	}


	cap.release();
    return 0;
}




