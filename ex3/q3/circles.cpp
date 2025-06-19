#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace cv;
using namespace std;


//Define the image size
#define IMG_HEIGHT	480
#define IMG_WIDTH 	640

#define WINDOW_NAME			"Hough"

//Define the application quit key (esc)
#define ESCAPE 		27


int main(int argc, char** argv){
	
	
	//Create the capture instance which will open the camera
	VideoCapture cap(0, CAP_V4L2);
	if(!cap.isOpened()){
		cout << "Unable to open camera for input" << endl;
		return 1;
	}
	
	//Configure the camera frame dimensions and other settings to increase the framerate.
	cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
	
	cap.set(CAP_PROP_FOURCC ,VideoWriter::fourcc('M', 'J', 'P', 'G') );
	cap.set(CAP_PROP_EXPOSURE, 100);
	cap.set(CAP_PROP_FPS, 90);
	
	cout << "Settings: fps=" << cap.get(CAP_PROP_FPS) << ", exposure=" << cap.get(CAP_PROP_EXPOSURE) << endl;
	
	//Create our display window
	namedWindow(WINDOW_NAME);

	
	
	Mat raw, frame, canny_frame;
	bool running = true;
	while(running){
		
		//Read frame and verify it is valid
		cap >> raw;
		if(raw.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		//Convert to grayscale and blur
		cvtColor(raw, frame, COLOR_BGR2GRAY);
		GaussianBlur(frame, frame, Size(9,9), 2, 2);


		// Hough Circle transform to detect circles in the frame
		vector<Vec3f> circles;
		HoughCircles(frame, circles, HOUGH_GRADIENT, 1, frame.rows/8, 100, 50, 0, 0);

        for( size_t i = 0; i < circles.size(); i++ ){
          Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
          int radius = cvRound(circles[i][2]);
		  
          // draw circle center and outline on the image frame
          circle( raw, center, 3, Scalar(0,255,0), -1, 8, 0 );
          circle( raw, center, radius, Scalar(0,0,255), 3, 8, 0 );
        }

		//Show the source frame with the circle detections drawn on it
		imshow(WINDOW_NAME, raw);

		//Wait for keypress (and allow frame to be displayed)
		switch(waitKey(10)){
			case ESCAPE:
				running = false;
				break;
			default:
				break;
		}
		
	}		

	destroyWindow(WINDOW_NAME);
	return 0;
}
	