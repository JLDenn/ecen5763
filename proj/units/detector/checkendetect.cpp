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

/** Global variables */
CascadeClassifier bird_cascade;


/** @function detectAndDisplay */
Mat detect( Mat frame )
{
    Mat frame_gray;
    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );

    //-- Detect birds
    std::vector<Rect> birds;
    bird_cascade.detectMultiScale( frame_gray, birds, 1.4, 5, CASCADE_SCALE_IMAGE);
	cout << "found " << birds.size() << endl;

    for ( size_t i = 0; i < birds.size(); i++ ){
        Point center( birds[i].x + birds[i].width/2, birds[i].y + birds[i].height/2 );
        ellipse( frame, center, Size( birds[i].width/2, birds[i].height/2 ), 0, 0, 360, Scalar( 255, 0, 255 ), 4 );
    }

	return frame;
}


/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                            "{help h||}"
                            "{@image||Image to process}"
							"{o||output results to this file}");

    parser.about( "\nThis program demonstrates using the cv::CascadeClassifier class to detect objects (chickens) in an image.\n"
                  "You can use Haar or LBP features.\n\n" );
    
	if(!parser.check() || argc < 2){
		parser.printMessage();
		return -1;
	}

    //-- 1. Load the cascades
    if(!bird_cascade.load( "../../train/cascade_out/cascade.xml" ) ){
        cout << "--(!)Error loading cascade\n";
        return -1;
    };


	Mat img_raw = imread(parser.get<String>("@image"));
    if(img_raw.empty()){
		cout << "Error loading image: " << parser.get<string>("@image") << endl;
		return 1;
	}
	Mat img;
	resize(img_raw, img, Size(1280,720));

	//-- 3. Apply the classifier to the frame
	cout << "Detecting..." << endl;
	Mat out = detect( img );
	
	if(parser.has("o")){
		cout << "Writing to " << parser.get<string>("o") << endl;
		imwrite(parser.get<string>("o"), out);
	}
	else{
		imshow( "Capture - bird detection", out );
		waitKey();
	}
    return 0;
}






/**  HOG detection test **/
/** @function main **/
/*
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
                             "{@image||Image to process}");

    parser.about( "\nThis program demonstrates using HOG to detect chickens in an image.\n" );
    
	if(!parser.check() || argc < 2){
		parser.printMessage();
		return 1;
	}

	Mat img = imread(parser.get<string>(0));
	if(img.empty()){
		cout << "Error opening image at " << parser.get<string>(0) << endl;
		return 1;
	}


	Mat imgRaw;
	img.copyTo(imgRaw);
	int nLevels = 13;

	
	//Get the executable path 
	string exPath = ".";
	size_t p = string(argv[0]).rfind("/");
	if(p != string::npos)
		exPath = string(argv[0]).substr(0, p);
	
	char rel[128];
	snprintf(rel, sizeof(rel), "%s/../../train/chick_on_deck.xml", exPath.c_str());
	
	HOGDescriptor hog;
	hog.load(rel);
	hog.nlevels = nLevels;
	
	vector<Rect> found;
	vector<double> quality;
	hog.detectMultiScale(img, found, quality); 


	for(int i=0;i<found.size();i++){
		cout << "found at " << found[i].x << "," << found[i].y << endl;
		rectangle(imgRaw, found[i], Scalar(0, quality[i] * 255, 0), 3);
	}

	imshow("Detections", imgRaw);
	waitKey();
    return 0;
}
*/