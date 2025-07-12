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



/** @function main */
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
	snprintf(rel, sizeof(rel), "%s/../../train/chick.xml", exPath.c_str());
	
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
