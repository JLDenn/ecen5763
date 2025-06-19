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
	
	int thresh = 100;
	
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
	createTrackbar("Threshold", WINDOW_NAME, &thresh, 255);
	
	
	Mat raw, frame, canny_frame;
	bool running = true;
	while(running){
		
		//Read frame and verify it is valid
		cap >> raw;
		if(raw.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		//Convert the captured frame to grayscale and calculate the canny edges
		cvtColor(raw, frame, COLOR_BGR2GRAY);
		Canny(frame, canny_frame, 50, 200, 3);


		// Hough Line transform on the canny edge frame
		vector<Vec2f> lines; // will hold the results of the detection
		HoughLines(canny_frame, lines, 1, CV_PI/180, thresh, 50, 10 ); // runs the actual detection

		// Overlay lines on frame
		for( size_t i = 0; i < lines.size(); i++ ){
			float rho = lines[i][0], theta = lines[i][1];
			Point pt1, pt2;
			double a = cos(theta), b = sin(theta);
			double x0 = a*rho, y0 = b*rho;
			pt1.x = cvRound(x0 + 1000*(-b));
			pt1.y = cvRound(y0 + 1000*(a));
			pt2.x = cvRound(x0 - 1000*(-b));
			pt2.y = cvRound(y0 - 1000*(a));
			line( raw, pt1, pt2, Scalar(0,0,255), 3, LINE_AA);
		}

		//Display the captured image frame with the lines drawn on it
		imshow(WINDOW_NAME, raw);

		//Wait for a keypress to give the frame a chance to be displayed.
		switch(waitKey(10)){
			case ESCAPE:
				running = false;
				break;
			default:
				break;
		}
		
	}		

	//Cleanup the window before we close
	destroyWindow(WINDOW_NAME);
	return 0;
}
	