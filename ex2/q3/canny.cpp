/**
	@author	Justin Denning
	@date	11 June 2025
	
	@Description
		This code simply opens an image provided via cmd line argument, applies Canny edge detection, and displays both 
		the original and the edge image. 
	
	@note
		This code in its entirety was written personally by Justin Denning for 
		Exercise #1 in ECEN5763 Summer 2025
**/


#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>

#define ESCAPE_KEY 	27

//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME_ORIG		"Original"
#define WINDOW_NAME_CANNY		"Canny"


using namespace cv;
using namespace std;


/**
	Perform canny operation on a frame and returns the results
	
	@param frame - source frame to perform sobel operation on. Should be already blurred if required.

	
	@return - the resulting Mat
*/
static Mat simpleCanny(Mat frame, int minThresh, int ratio = 3, int kernel_size = 3){
	
	cout << "Performing Canny edge detection" << endl;

	Mat canny;
	Canny(frame, canny, minThresh,  minThresh * ratio, kernel_size);
	
	Mat dst;
	dst = Scalar::all(0);
	frame.copyTo(dst, canny);
	return dst;
}

static void onTrackbar(int val, void* arg){
	Mat *frame = (Mat *)arg;
	imshow(WINDOW_NAME_CANNY, simpleCanny(*frame, val));
}


/**
	Main entry point. 
*/
int main(int argc, char *argv[]){
	
	if(argc != 2){
		cout << "Usage: %s <image path>" << endl;
		return 1;
	}
	
	//Read in the image as a greyscale image
	Mat orig = imread(argv[1], IMREAD_COLOR);
	if(orig.empty()){
		cout << "Error reading image at " << argv[1] << endl;
		return 1;
	}

	
	//Convert image to grayscale and blur
	Mat gray, frame;
	cvtColor(orig, gray, COLOR_BGR2GRAY);
	GaussianBlur(gray, frame, Size(3,3), 0, 0, BORDER_DEFAULT);
	
	
	//Create window and display the original image
	namedWindow(WINDOW_NAME_ORIG);
	imshow(WINDOW_NAME_ORIG, orig);
	
	//Create window, perform sobel edge detection, and display the resulting image
	namedWindow(WINDOW_NAME_CANNY);
	int minThresh = 50;
	createTrackbar("Threshold", WINDOW_NAME_CANNY, &minThresh, 100, onTrackbar, &frame);
	
	Mat canny = simpleCanny(frame, minThresh);
	imshow(WINDOW_NAME_CANNY, canny);
	
	//Wait for a keypress so we have time to see the images. 
	waitKey(0);

	cout << "Destroying windows" << endl;
	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME_ORIG);
	destroyWindow(WINDOW_NAME_CANNY);
	
	//Return success
	return 0;
}