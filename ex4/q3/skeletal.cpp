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
		
		cap >> gray;
		resize(gray, src, Size(640,360));
		
		cvtColor(src, gray, COLOR_BGR2GRAY);
		
		
		//Remove the background based on the first frame collected (assumes no hand in the initial frame)
		if(base.empty())
			gray.copyTo(base);	//Create our background elemination frame

		subtract(gray, base, diff);

		// To remove median filter, just replace blurr value with 1
		GaussianBlur(diff, mfblur, Size(3,3), 3);

		// Use 70 negative for Moose, 150 positive for hand
		// 
		// To improve, compute a histogram here and set threshold to first peak
		//
		// For now, histogram analysis was done with GIMP
		//
		threshold(mfblur, binary, bthresh, 255, THRESH_BINARY);
		Mat binColor;
		cvtColor(binary, binColor, COLOR_GRAY2BGR);


		Mat drawing = Mat::zeros( binary.size(), CV_8UC3 );	
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(binary, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

		if(contours.size()){

			//Find the largest contour (assuming it is the hand)
			int lgIdx = 0;
			double area = 0.0;
			for(int i=0;i<contours.size();i++){
				double a = contourArea(contours[i]);
				if(a > area){
					lgIdx = i;
					area = a;
				}
			}


			vector<Vec4i> convDefects;
			vector<Point> lgContour = contours[lgIdx];
			vector<vector<Point>> hull(1);
			convexHull( Mat(lgContour), hull[0], false);
		 
				
			Scalar color = Scalar( 0,0,255 );
			drawContours( drawing, contours, lgIdx, color );
			drawContours( drawing, hull, 0, color, 2);

			if(hull[0].size() > 2){				
				vector<int> hullIndexes;
				convexHull(Mat(lgContour), hullIndexes, true);

				convexityDefects(Mat(lgContour), hullIndexes, convDefects);
				
				for(int i=0;i<convDefects.size();i++){
					Point p1 = lgContour[convDefects[i][0]];
					Point p2 = lgContour[convDefects[i][1]];
					Point p3 = lgContour[convDefects[i][2]];
					
					line(drawing, p1, p3, Scalar(0,255,0), 1); 
					line(drawing, p3, p2, Scalar(0,255,0), 1);
				}
			}
			
			
			//Create histogram of defects to determine if, and how many fingers are up.
			int max = 0;
			for(int i=0;i<convDefects.size();i++){
				if(convDefects[i][3] > max)
					max = convDefects[i][3];
			}
			
			//First finger only if we have a large enough max defect
			fingerCount = 1;
			
			for(int i=0;i<convDefects.size();i++){
				if(max > MIN_FINGER_PRESENT && convDefects[i][3] > MIN_FINGER_THRESH * max / 10)
					fingerCount++;
			}				

		}
		
		
		
		// vector<Vec4i> convDefects;
		// convexityDefects(contours, hull, convDefects);
		
		// cout << "Found " << convDefects.size() << "Convexity defects" << endl;
		

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


		cvtColor(gray, temp, COLOR_GRAY2BGR);
		putText(temp, string("Fingers: ")+to_string(fingerCount), Point(50,50), FONT_HERSHEY_COMPLEX, 1.0, Scalar(0,0,255));

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
		
		
		if(waitKey(10) == ESCAPE || frameNumber >= FRAME_COUNT)
			break;
	}

	return 0;
}