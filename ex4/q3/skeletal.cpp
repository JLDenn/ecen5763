#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>

using namespace cv;
using namespace std;

#define COORD_OUT(a)	"(" << a[0] << "," << a[1] << ") - (" << a[2] << "," << a[3] << ")"
#define MIN_FINGER_THRESH		6 	//Value out of 10 that will be counted as a finger gap
#define MIN_FINGER_PRESENT		15000 //min threshold for counting a finger as found
#define FRAME_COUNT				300

#define ESCAPE	27



int main(int argc, char** argv){
	const char* filename = argc >= 2 ? argv[1] : "pic1.jpg";

	VideoCapture cap(0);
	if(!cap.isOpened()){
		cout << "Error opening camera" << endl;
		return 1;
	}
	
	
	int bthresh = 90;
	int skthresh = 40;
	
	Mat gray, binary, mfblur;
	Mat src;
	Mat base, diff;
	int frameNumber = 1;

	//Loop through number of frames required
	while(1){
		int fingerCount = 0;
		
		//Read image frame to process and resize it so we're not working with overly large images.
		cap >> gray;
		resize(gray, src, Size(640,360));
		
		//Convert it to grayscale
		cvtColor(src, gray, COLOR_BGR2GRAY);
		
		
		//Remove the background based on the first frame collected (assumes no hand in the initial frame)
		if(base.empty())
			gray.copyTo(base);	//Create our background elemination frame

		//Subtract out our stored background so we'll only see the hand in the foreground
		subtract(gray, base, diff);

		//Blur the image to aid in smoothing edges
		GaussianBlur(diff, mfblur, Size(3,3), 3);

		//Apply a threshold to get a cleaner object to skeletonize and find contours of.
		threshold(mfblur, binary, bthresh, 255, THRESH_BINARY);
		
		//We need a copy since binary will be destroyed lower down, and we'll need it to be in color for the final hconcat call
		//that combines the 4 images for saving. 
		Mat binColor;
		cvtColor(binary, binColor, COLOR_GRAY2BGR);

		//Create a blank canvas for use to draw contours and contourDefects on
		Mat drawing = Mat::zeros( binary.size(), CV_8UC3 );	
		
		//Find the contours in the current frame
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(binary, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

		if(contours.size()){
			//Find the largest contour (assume it is the hand)
			int lgIdx = 0;
			double area = 0.0;
			for(int i=0;i<contours.size();i++){
				double a = contourArea(contours[i]);
				if(a > area){
					lgIdx = i;
					area = a;
				}
			}

			//Find the convex Hull (the outline extents of the hand contour)
			vector<Vec4i> convDefects;
			vector<Point> lgContour = contours[lgIdx];
			vector<vector<Point>> hull(1);
			convexHull( Mat(lgContour), hull[0], false);
		 
			//Draw the contors and hull so we can see what is being used for finger recognition
			Scalar color = Scalar( 0,0,255 );
			drawContours( drawing, contours, lgIdx, color );
			drawContours( drawing, hull, 0, color, 2);

			if(hull[0].size() > 2){				
				vector<int> hullIndexes;
				convexHull(Mat(lgContour), hullIndexes, true);

				//Find the hull defects (gaps in the extents). These will be the gaps between the fingers. 
				convexityDefects(Mat(lgContour), hullIndexes, convDefects);
				
				//Draw the defects found
				for(int i=0;i<convDefects.size();i++){
					Point p1 = lgContour[convDefects[i][0]];
					Point p2 = lgContour[convDefects[i][1]];
					Point p3 = lgContour[convDefects[i][2]];
					
					line(drawing, p1, p3, Scalar(0,255,0), 1); 
					line(drawing, p3, p2, Scalar(0,255,0), 1);
				}
			}
			
			
			//Find the max defect distance (we'll only count it if it is above a threshold defined above)
			int max = 0;
			for(int i=0;i<convDefects.size();i++){
				if(convDefects[i][3] > max)
					max = convDefects[i][3];
			}
			
			//Start with 1 finger (since we have enough contours to know there is a hand)
			fingerCount = 1;
			
			for(int i=0;i<convDefects.size();i++){
				if(max > MIN_FINGER_PRESENT && convDefects[i][3] > MIN_FINGER_THRESH * max / 10)
					fingerCount++;
			}				

		}
		
		
		// This section of code was adapted from the following post, which was
		// based in turn on the Wikipedia description of a morphological skeleton
		//
		// http://felix.abecassis.me/2011/09/opencv-morphological-skeleton/
		//
		Mat skel(binary.size(), CV_8UC1, Scalar(0));
		Mat temp;
		Mat eroded;
		Mat element = getStructuringElement(MORPH_CROSS, Size(3, 3));
		bool done;
		int iterations=0;

		do{
			erode(binary, eroded, element);
			dilate(eroded, temp, element);
			subtract(binary, temp, temp);
			bitwise_or(skel, temp, skel);
			eroded.copyTo(binary);

			done = countNonZero(binary);
			iterations++;
		} while (done<50 && (iterations < 100));


		//Overlay the number of fingers detected on the raw (original ) grayscale image.
		cvtColor(gray, temp, COLOR_GRAY2BGR);
		putText(temp, string("Fingers: ")+to_string(fingerCount), Point(50,50), FONT_HERSHEY_COMPLEX, 1.0, Scalar(0,0,255));

		//Combine the 4 frames of interest into a single image and save out the individual .jpg frames that we'll use to create the mpeg from. 
		Mat skelColor, top, full;
		if(frameNumber <= FRAME_COUNT){
			cvtColor(skel, skelColor, COLOR_GRAY2BGR);
			hconcat(temp, skelColor, top);
			
			hconcat(binColor, drawing, temp);
			vconcat(top, temp, full);
			
			imshow("raw", full);
			
			char name[64];
			snprintf(name, sizeof(name), "output/frame%04d.jpg", frameNumber++);
			imwrite(name, full);
		}
		
		//On ESC key press, or if we've reached 300 frames, we'll exit. 
		if(waitKey(10) == ESCAPE || frameNumber >= FRAME_COUNT)
			break;
	}

	return 0;
}