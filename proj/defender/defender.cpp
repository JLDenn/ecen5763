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
#define IMG_HEIGHT	720
#define IMG_WIDTH 	1280

//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME		"Live Video"

//Define the application quit key (esc)
#define ESCAPE 		27


using namespace cv;
using namespace std;

int main(int argc, char *argv[]){
	
/**
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
	
	Mat frame;
	
	while(1){
		//Read frame and verify it is valid
		cap >> frame;
		if(frame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		

		//Display the image
		imshow(WINDOW_NAME, frame);
		
		//Give the system time to actually render the image to the screen, and check for an escape key press
		//	which will cause us to quit. 
		if(waitKey(10) == ESCAPE)
			break;
	}

	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME);
*/


	PanTilt pt;
	if(!pt.openPort()){
		cout << "Error opening default serial port" << endl;
		return 1;
	}
	
	int pan, tilt, periph;
	if(!pt.getPos(&pan, &tilt, &periph)){
		cout << "Error getting position information" << endl;
		return 1;
	}

	cout << "Current position " << pan << ", " << tilt << ", " << (periph ? "ON" : "OFF") << endl;

	if(!pt.home()){
		cout << "Error homing" << endl;
		return 1;
	}
	
	cout << "Waiting 10s..." << endl;
	usleep(10000000);
	
	cout << "Moving to 0, 100" << endl;
	if(!pt.moveTo(0, 100)){
		cout << "Error moving to 0,100" << endl;
		return 1;
	}
	
	cout << "Waiting 4s..." << endl;
	usleep(4000000);
	
	cout << "Moving 100, -200" << endl;
	if(!pt.move(100, -200)){
		cout << "Error performing delta move" << endl;
		return 1;
	}
	
	cout << "Waiting 2s..." << endl;
	usleep(2000000);
	
	cout << "Setting active" << endl;
	if(!pt.active(1)){
		cout << "Error setting perminant active" << endl;
		return 1;
	}
	
	cout << "Waiting 2s..." << endl;
	usleep(2000000);
	
	cout << "Setting inactive" << endl;
	if(!pt.active(0)){
		cout << "Error setting perminant inactive" << endl;
		return 1;
	}
	
	cout << "Waiting 2s..." << endl;
	usleep(2000000);

	cout << "Setting active for 2 seconds" << endl;
	if(!pt.active(1, 2000)){
		cout << "Error setting 2s active" << endl;
		return 1;
	}
	
	cout << "Waiting 3s..." << endl;
	usleep(3000000);
	
	cout << "trying consecutive moves" << endl;
	if(!pt.moveTo(200, 200) || !pt.moveTo(-20, -50)){
		cout << "Error setting one of the two moveTo commands" << endl;
		return 1;
	}
	
	cout << "All tests complete" << endl;
	pt.closePort();
	

	
	//Return success
	return 0;
}