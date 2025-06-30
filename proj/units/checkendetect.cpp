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

/** Function Headers */
void detectAndDisplay( Mat frame );

/** Global variables */
CascadeClassifier chickenCascade;


/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
                             "{@image||Image to process}");

    parser.about( "\nThis program demonstrates using the cv::CascadeClassifier class to detect chickens in an image.\n" );
    
	if(!parser.check() || argc < 2){
		parser.printMessage();
		return 1;
	}

    //-- 1. Load the cascades
    if( !chickenCascade.load( "../training/cascade/cascade.xml" ) )
    {
        cout << "--(!)Error loading chicken cascade\n";
        return -1;
    };


	Mat img = imread(parser.get<String>("@image"));
    if(img.empty()){
		cout << "Error loading image: " << parser.get<String>("@image") << endl;
		return 1;
	}

	//-- 3. Apply the classifier to the frame
	detectAndDisplay( img );
	waitKey();
    return 0;
}

/** @function detectAndDisplay */
void detectAndDisplay( Mat frame )
{
    Mat frame_gray = frame;
    //cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    //equalizeHist( frame_gray, frame_gray );


	imshow( "Capture - gray image", frame_gray );
	waitKey(10);

    //-- Detect chickens
    std::vector<Rect> chickens;
    chickenCascade.detectMultiScale( frame_gray, chickens );

    for ( size_t i = 0; i < chickens.size(); i++ )
    {
        Point center( chickens[i].x + chickens[i].width/2, chickens[i].y + chickens[i].height/2 );
        ellipse( frame, center, Size( chickens[i].width/2, chickens[i].height/2 ), 0, 0, 360, Scalar( 255, 0, 255 ), 4 );

    }

    //-- Show what you got
    imshow( "Capture - Chicken detection", frame );
}
