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
#define IMG_HEIGHT	480
#define IMG_WIDTH 	640

//Define details about the overlay (border and crosshairs)
#define BORDER_THICKNESS	4	//4		//Setting to 0 disables border processing
#define BORDER_COLOR		0,0,255
#define CROSS_COLOR			0,255,255
#define CROSS_THCKNESS		1	//1		//setting to 0 disables crosshair processing

//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME		"Live Video"

//Define the application quit key (esc)
#define ESCAPE 		27

//Define the interval (in ms) that we wish to print the image processing time to the console 
//	set to 0 to disable
#define STAT_PRINT_INTERVAL		10000






using namespace cv;
using namespace std;

int main(int argc, char *argv[]){
	
	//Create the capture instance which will open the camera
	VideoCapture cap(0, CAP_V4L2);
	if(!cap.isOpened()){
		cout << "Unable to open camera for input" << endl;
		return 1;
	}
	
	//Configure the camera frame dimensions
	cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
	
	//cap.set(CAP_PROP_BUFFERSIZE, 2);
	cap.set(CAP_PROP_FOURCC ,VideoWriter::fourcc('M', 'J', 'P', 'G') );
	cap.set(CAP_PROP_EXPOSURE, 100);
	cap.set(CAP_PROP_FPS, 90);
	
	
	cout << "Settings: fps=" << cap.get(CAP_PROP_FPS) << ", exposure=" << cap.get(CAP_PROP_EXPOSURE) << endl;
	
	//Create our display window
	namedWindow(WINDOW_NAME);
	
#if STAT_PRINT_INTERVAL
	//Hold the next time that we will be printing the frame processing time to the console. 
	//	This will be set following each print so it will always be the time that we wish
	//	to print next. 
	auto nextFrameStat = chrono::high_resolution_clock::now() + chrono::milliseconds(STAT_PRINT_INTERVAL);
	
	//Store the last frame capture time
	chrono::time_point<chrono::high_resolution_clock> last;
	chrono::time_point<chrono::high_resolution_clock>  first;
	unsigned long frameCount = 0;
#endif

	Mat frame;
	bool running = true;
	while(running){
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
		
#if BORDER_THICKNESS
		//Draw 4 pixel border
		rectangle(frame, 
				Point(0,0), 
				Point(IMG_WIDTH-1, IMG_HEIGHT-1), 
				Scalar(BORDER_COLOR), 
				BORDER_THICKNESS);
#endif
		
#if CROSS_THCKNESS
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
		
#endif
		//Display the every 3rd image since our video framerate is only 30hz
		if(frameCount % 3 == 0)
			imshow(WINDOW_NAME, frame);
		
#if STAT_PRINT_INTERVAL
		last = chrono::high_resolution_clock::now();
		if(!frameCount)
			first = last;
		frameCount++;
		

		//If it is time to print the frame processing time stats, we'll print it and upate the nextFrameStat time
		if(start >= nextFrameStat){
			std::chrono::duration<double> diff = last - start;
			cout << "Frame Processing time: " << diff.count() << "s, ";
			nextFrameStat = start + chrono::milliseconds(STAT_PRINT_INTERVAL);
			
			diff = last - first;
			cout << "Frame-rate (avg over runtime): " << frameCount/diff.count() << "fps, ";
			cout << "Total runtime: " << diff.count() << "s" << endl;
		}
#endif
		
		//Give the system time to actually render the image to the screen, and check for an escape key press
		//	which will cause us to quit. 
		switch(waitKey(1)){
		case ESCAPE:
			running = false;
			break;
		case 'e':
			if(cap.get(CAP_PROP_EXPOSURE) > 40)
				cap.set(CAP_PROP_EXPOSURE, cap.get(CAP_PROP_EXPOSURE)-40);
			cout << "Settings: fps=" << cap.get(CAP_PROP_FPS) << ", exposure=" << cap.get(CAP_PROP_EXPOSURE) << endl;
			break;
		case 'E':
			cap.set(CAP_PROP_EXPOSURE, cap.get(CAP_PROP_EXPOSURE)+40);
			cout << "Settings: fps=" << cap.get(CAP_PROP_FPS) << ", exposure=" << cap.get(CAP_PROP_EXPOSURE) << endl;
			break;
		}
			
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