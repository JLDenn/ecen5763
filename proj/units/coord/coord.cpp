/**
	The base for this code came from code that was initially used for Exercise 1 (face detection)
*/

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace cv;


static void on_track(int thresh, void* raw){
	
	Mat img;
	Mat *imgRaw = (Mat*) raw;
	Canny(*imgRaw, img, 233, 233*3, 3);
	
	vector<Vec4i> lines; // will hold the results of the detection
	HoughLinesP(img, lines, 1, CV_PI/60, thresh, 50, 10); // runs the actual detection
	
	Mat c;
	cvtColor(img, c, COLOR_GRAY2BGR);
	
	//Draw the lines
	for( size_t i = 0; i < lines.size(); i++ ){
		Vec4i l = lines[i];
		line( c, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
	}
	
	imshow("Edge", c);
}


/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
                             "{@image||Image to process}");

    parser.about( "\nThis program demonstrates using line detection to find the edges of the deck (for coordinate translation).\n" );
    
	if(!parser.check() || argc < 2){
		parser.printMessage();
		return 1;
	}

	Mat img = imread(parser.get<string>(0));
	if(img.empty()){
		cout << "Error opening image at " << parser.get<string>(0) << endl;
		return 1;
	}
	
	Mat r;
	double scale = 1024.0/img.cols;
	resize(img, r, Size(1024,(int)(img.rows * scale)));
	cvtColor(r, img, COLOR_BGR2GRAY);


	int threshLine = 180;
	namedWindow("Edge");
	createTrackbar("threshold", "Edge", &threshLine, 250, on_track, (void*)&img);

	on_track(threshLine, &img);

	waitKey();
    return 0;
}
