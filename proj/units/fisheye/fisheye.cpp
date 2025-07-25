/**
	@author	Justin Denning
	@date	12 July 2025
	
	@Description

	
**/
#include <unistd.h>
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/calib3d.hpp>
#include <iostream>
#include <chrono>


//Define the image size
#define IMG_WIDTH 	1280
#define IMG_HEIGHT	720


//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME		"Live Video"

//Define the application quit key (esc)
#define ESCAPE 		27


using namespace cv;
using namespace std;



int main(int argc, char *argv[]){
	
	//------------------------------------------------------------------
	//	Process CLI arguments	
	int opt;
	while( (opt = getopt(argc, argv, "t") ) != -1){
		switch(opt){
		case 't':
			cout << "t option enabled" << endl;
			break;
		
		default:
			fprintf(stderr, "Usage: %s \n", argv[0]);
			exit(-1);
		}
	}
	
	

	//Create the capture instance which will open the camera
	VideoCapture cap(0);
	if(!cap.isOpened()){
		cout << "Unable to open camera for input" << endl;
		exit(-1);
	}
	
	//Configure the camera frame dimensions
	cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
	
	
	//Create our display window
	namedWindow(WINDOW_NAME);
	
	
	float kmat_values[3][3] = {{1.51865043e+03, 0.00000000e+00, 9.97723183e+02},
								{0.00000000e+00, 1.52017147e+03, 5.41360776e+02},
								{0.00000000e+00, 0.00000000e+00, 1.00000000e+00}};

	float dmat_values[4][1] = {{-0.4009397},
								{ 0},
								{ 0},
								{ 0}};

	// float balance = 0.0;
	// Size video_size(IMG_WIDTH,IMG_HEIGHT);
	
	// Mat kmat(3, 3, CV_32FC1, kmat_values);
	Mat dmat(4, 1, CV_32FC1, dmat_values);

	// Mat mapx, mapy, camera;


	// Mat eye = Mat::eye(3, 3, CV_32FC1);

	// fisheye::estimateNewCameraMatrixForUndistortRectify(
		// kmat,
		// dmat,
		// video_size,
		// eye,
		// camera,
		// balance
	// );

	// initUndistortRectifyMap(
		// kmat,
		// dmat,
		// eye,
		// camera,
		// video_size,
		// CV_32FC1,
		// mapx,
		// mapy
	// );
	


	
		
	
	
	
	Mat clrFrame;
	while(1){
		//Read frame and verify it is valid
		cap >> clrFrame;
		if(clrFrame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		
		Mat correctedFrame;
		
		float cam[3][3];
		cam[2][2] = 1;
		cam[0][2] = clrFrame.cols/2.0;  // define center x
		cam[1][2] = clrFrame.rows/2.0; // define center y
		cam[0][0] = 10;        	// define focal length x
		cam[1][1] = 10;        	// define focal length y
		
		Mat kmat(3, 3, CV_32FC1, kmat_values);

		
		//remap(clrFrame, correctedFrame, mapx, mapy, INTER_LINEAR, BORDER_CONSTANT);
		undistort(clrFrame, correctedFrame, kmat, dmat);
		
		
		//Display the image
		imshow(WINDOW_NAME, correctedFrame);

		
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