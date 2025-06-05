/**
	@author	Justin Denning
	@date	5 June 2025
	
	@Description
		This code makes use of opencv to open a video capture device (camera), add a border and crosshairs
		to the image and displays it live. It also saves the final frame to a file when closing. 
		
		Console printouts of the image processing time are printed when enabled (Modify STAT_PRINT_INTERVAL 
		macro below if desired)
	
	@note
		This code in its entirety was written personally by Justin Denning for 
		Exercise #1 in ECEN5763 Summer 2025
**/


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <chrono>

//Define the image size
#define IMG_HEIGHT	240
#define IMG_WIDTH 	320

//Define details about the overlay (border and crosshairs)
#define BORDER_THICKNESS	4
#define BORDER_COLOR		0,0,255
#define CROSS_COLOR			0,255,255
#define CROSS_THCKNESS		1

//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME		"Live Video"

//Define the application quit key (esc)
#define ESCAPE 		27

//Define the interval (in ms) that we wish to print the image processing time to the console 
//	set to 0 to disable
#define STAT_PRINT_INTERVAL		1000


using namespace cv;
using namespace std;

int main(int argc, char *argv[]){
	
	//Create the capture instance which will open the camera
	VideoCapture cap(0);
	if(!cap.isOpened()){
		cout << "Unable to open camera for input" << endl;
		return 1;
	}
	
	//Configure the camera frame dimensions
	cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
	
	//Create our display window
	namedWindow(WINDOW_NAME);
	
#if STAT_PRINT_INTERVAL
	//Hold the next time that we will be printing the frame processing time to the console. 
	//	This will be set following each print so it will always be the time that we wish
	//	to print next. 
	auto nextFrameStat = chrono::high_resolution_clock::now() + chrono::milliseconds(STAT_PRINT_INTERVAL);
#endif

	Mat frame;
	
	while(1){
#if STAT_PRINT_INTERVAL
		//Collect the start time so we can calculate the image processing time. 
		auto start = chrono::high_resolution_clock::now();
#endif
		
		//Read frame and verify it is valid
		cap >> frame;
		if(frame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		
		//Draw 4 pixel border
		rectangle(frame, 
				Point(0,0), 
				Point(IMG_WIDTH-1, IMG_HEIGHT-1), 
				Scalar(BORDER_COLOR), 
				BORDER_THICKNESS);
		
		
		//Draw the crosshairs
		line(frame, 
				Point(0 + BORDER_THICKNESS, IMG_HEIGHT/2), 
				Point(IMG_WIDTH-1 - BORDER_THICKNESS, IMG_HEIGHT/2), 
				Scalar(CROSS_COLOR), 
				CROSS_THCKNESS);
				
		line(frame, 
				Point(IMG_WIDTH/2, BORDER_THICKNESS), 
				Point(IMG_WIDTH/2, IMG_HEIGHT-1 - BORDER_THICKNESS),
				Scalar(CROSS_COLOR),
				CROSS_THCKNESS);
		
		//Display the image
		imshow(WINDOW_NAME, frame);
		
#if STAT_PRINT_INTERVAL
		//If it is time to print the frame processing time stats, we'll print it and upate the nextFrameStat time
		if(start >= nextFrameStat){
			const std::chrono::duration<double> diff = chrono::high_resolution_clock::now() - start;
			cout << "Frame processing time: " << diff.count() << "s" << endl;
			nextFrameStat = start + chrono::milliseconds(STAT_PRINT_INTERVAL);
		}
#endif
		
		//Give the system time to actually render the image to the screen, and check for an escape key press
		//	which will cause us to quit. 
		if(waitKey(10) == ESCAPE)
			break;
	}
	
	//If we have a valid frame at quit, we'll write that frame to a file
	if(!frame.empty()){
		vector<int> compression_params;
		compression_params.push_back(IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(9);
		if(!imwrite("320x240Capture.png", frame, compression_params))
			cout << "Error writing final image to file" << endl;
	}
	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME);
	
	//Return success
	return 0;
}