/**
	@author	Justin Denning
	@date	13 June 2025
	
	@Description
		This code makes use of opencv to open a video capture device (camera) and performs a configurable edge detection
	
	@note
		This code in its entirety was written personally by Justin Denning for 
		Exercise #2 in ECEN5763 Summer 2025
**/


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <chrono>

//Define the image size
#define IMG_HEIGHT	480
#define IMG_WIDTH 	640


//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME			"Live Video"
#define WINDOW_NAME_EDGE	"Edges"

//Define the application quit key (esc)
#define ESCAPE 		27

#define FPS_FRAME_AVG	10		//Number of frames to average to determine frames per second.

static struct {	
	char edgeMode;				//'c' = canny, 's' = sobel, 'n' = none. Window is created/destroyed when changed (as needed)
	int minThresh;				//minimum threshold for canny edge detection
	double calcFramerate;		//Calculated framerate (updated every STATE_PRINT_INTERVAL (1000ms))
} state = {.edgeMode = 'n', .minThresh = 50};


using namespace cv;
using namespace std;



/**
	(COPIED FROM ex2/q2/sobel.cpp)
	Perform sobel operation on a frame in both dimensions and combine the results
	
	@param frame - source frame to perform sobel operation on. Should be already blurred if required.
	@param ksize - odd value between 1 and 31
	@param scale - scale value for the results (> 1.0 brightens the results, think contrast)
	@param delta - offset value to add to result values (think brightness)
	@prarm borderType - see https://docs.opencv.org/4.1.1/d2/de8/group__core__array.html#ga209f2f4869e304c82d07739337eae7c5
	
	@return - the resulting Mat
*/
static Mat simpleSobel(Mat frame, int ksize = 3, double scale = 1.0, double delta = 0, int borderType = BORDER_DEFAULT){
	
	//cout << "Performing Sobel edge detection" << endl;
	Mat sobel_x, sobel_y;
	Sobel(frame, sobel_x, CV_32F, 1, 0, ksize, scale, delta, borderType);
	Sobel(frame, sobel_y, CV_32F, 0, 1, ksize, scale, delta, borderType);
	
	convertScaleAbs(sobel_x, sobel_x);
	convertScaleAbs(sobel_y, sobel_y);
	Mat sobel;
	addWeighted(sobel_x, 0.5, sobel_y, 0.5, 0, sobel);
	
	return sobel;
}


/**
	(COPIED FROM ex2/q3/canny.cpp)
	Perform canny operation on a frame and returns the results
	
	@param frame - source frame to perform sobel operation on. Should be already blurred if required.

	
	@return - the resulting Mat
*/
static Mat simpleCanny(Mat frame, int minThresh, int ratio = 3, int kernel_size = 3){
	
	//cout << "Performing Canny edge detection" << endl;

	Mat canny;
	Canny(frame, canny, minThresh,  minThresh * ratio, kernel_size);
	
	Mat dst;
	dst = Scalar::all(0);
	frame.copyTo(dst, canny);
	return dst;
}



static void onTrackbar(int val, void* arg){

}



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
	

	
	//Store the last frame capture time
	chrono::time_point<chrono::high_resolution_clock> last;
	chrono::time_point<chrono::high_resolution_clock>  first;
	unsigned long frameCount = 0;


	Mat frame;
	bool running = true;
	while(running){
		
		//Read frame and verify it is valid
		cap >> frame;
		if(frame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		//Perform edge detection and display if enabled
		if(state.edgeMode == 'c'){
			Mat gray;
			cvtColor(frame, gray, COLOR_BGR2GRAY);
			GaussianBlur(gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT);
			Mat canny = simpleCanny(gray, state.minThresh);
			imshow(WINDOW_NAME_EDGE, canny);
		}
		else if(state.edgeMode == 's'){
			Mat gray;
			cvtColor(frame, gray, COLOR_BGR2GRAY);
			GaussianBlur(gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT);			
			Mat sobel = simpleSobel(gray);
			imshow(WINDOW_NAME_EDGE, sobel);
		}
		else{	//None
			
		}

		//Overlay framerate on image frame
		string framerate = std::to_string(state.calcFramerate);
		framerate.append(" FPS");
		putText(frame, framerate, Point(10,30), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(255,255,255), 1);
		imshow(WINDOW_NAME, frame);
		

		//Get the current time, and save time if this frame starts a new set of frames we'll be calculating FPS over.
		last = chrono::high_resolution_clock::now();
		if(!frameCount)
			first = last;
		
		//If it is time to print the frame processing time stats, we'll save the new framerate to overlay on the next frames
		if(frameCount >= FPS_FRAME_AVG){			
			std::chrono::duration<double> diff = last - first;
			state.calcFramerate = frameCount / diff.count();
			frameCount = 0;
		}
		else
			frameCount++;
		
		
		//Give the system time to actually render the image to the screen, and check for a key press
		switch(waitKey(1)){
		case ESCAPE:
			running = false;
			break;
		case 'n':
			//Disable all edge detections. This will destroy the edge window if it was previously displayed. 
			if(state.edgeMode != 'n')
				destroyWindow(WINDOW_NAME_EDGE);
			
			state.edgeMode = 'n';
			cout << "(n) Disable edge detection" << endl;
			break;
		case 'c':
			//Enable Canny edge detection. Ensure the window is created and has a trackbar. 
			if(state.edgeMode != 'c'){
				namedWindow(WINDOW_NAME_EDGE);
				createTrackbar("Threshold", WINDOW_NAME_EDGE, &state.minThresh, 100, onTrackbar);
			}
			state.edgeMode = 'c';
			cout << "(c) Enable Canny edge detection" << endl;
			break;
		case 's':
			//Enable Sobel edge detection and display. Destroy and re-create the window (to ensure we don't have a trackbar).
			if(state.edgeMode != 's'){
				if(state.edgeMode != 'n')
					destroyWindow(WINDOW_NAME_EDGE);
				namedWindow(WINDOW_NAME_EDGE);
			}
			state.edgeMode = 's';
			cout << "(s) Enable Sobel edge detection" << endl;
			break;
			
		case -1:
			break;
		default:
			cout << "Invalid key. Expected: c, s, n, or ESC" << endl;
			break;
		}
			
	}

	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME);
	if(state.edgeMode != 'n')
		destroyWindow(WINDOW_NAME_EDGE);
	
	//Return success
	return 0;
}