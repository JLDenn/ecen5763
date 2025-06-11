/**
	@author	Justin Denning
	@date	11 June 2025
	
	@Description
		This code simply opens an image provided via cmd line argument, applies Sobel edge detection, and displays both 
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
#define WINDOW_NAME_SOBEL		"Sobel"


using namespace cv;
using namespace std;


/**
	Perform sobel operation on a frame in both dimensions and combine the results
	
	@param frame - source frame to perform sobel operation on. Should be already blurred if required.
	@param ksize - odd value between 1 and 31
	@param scale - scale value for the results (> 1.0 brightens the results, think contrast)
	@param delta - offset value to add to result values (think brightness)
	@prarm borderType - see https://docs.opencv.org/4.1.1/d2/de8/group__core__array.html#ga209f2f4869e304c82d07739337eae7c5
	
	@return - the resulting Mat
*/
static Mat simpleSobel(Mat frame, int ksize = 3, double scale = 1.0, double delta = 0, int borderType = BORDER_DEFAULT){
	
	cout << "Performing Sobel edge detection" << endl;
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
	namedWindow(WINDOW_NAME_SOBEL);
	Mat sobel = simpleSobel(frame);
	imshow(WINDOW_NAME_SOBEL, sobel);
	
	//Wait for a keypress so we have time to see the images. 
	waitKey(0);

	cout << "Destroying windows" << endl;
	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME_ORIG);
	destroyWindow(WINDOW_NAME_SOBEL);
	
	//Return success
	return 0;
}