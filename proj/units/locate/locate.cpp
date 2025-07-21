#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include<opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>

#include <iostream>
#include <string>

using namespace std;
using namespace cv;



bool comp(DMatch a, DMatch b){
	return a.distance < b.distance;
}


/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
                             //"{@obj||Image to process}"
							 "{@scene||Scene to search for the image in}");

    parser.about( "\nThis program finds the obj image in the scene image\n" );
    
	if(!parser.check() || argc < 2){
		parser.printMessage();
		return 1;
	}

	// Mat iobj = imread(parser.get<string>(0), IMREAD_GRAYSCALE);
	// if(iobj.empty()){
		// cout << "Error opening image at " << parser.get<string>(0) << endl;
		// return 1;
	// }
	
	Mat iscene = imread(parser.get<string>(0), IMREAD_GRAYSCALE);
	if(iscene.empty()){
		cout << "Error opening image at " << parser.get<string>(0) << endl;
		return 1;
	}
	
	Mat scene, obj;
	//resize(iobj, obj, Size(0,0), 0.25, 0.25);
	resize(iscene, scene, Size(0,0), 0.25, 0.25);
	
	
	// Ptr<FeatureDetector> detector = ORB::create(20000);
	// Ptr<DescriptorExtractor> descriptor = ORB::create();

	// Ptr<BFMatcher> matcher  = BFMatcher::create(NORM_HAMMING , true);

	// vector<KeyPoint> kptsObj, kptsScene;
	// detector->detect(obj, kptsObj );
	// detector->detect(scene, kptsScene);
	
	// Mat descrObj, descrScene;
	// descriptor->compute(obj, kptsObj, descrObj);
	// descriptor->compute(scene, kptsScene, descrScene);

	
	// vector<DMatch> matches;
	// matcher->match(descrObj, descrScene, matches);
	
	
	// sort(matches.begin(), matches.end(), comp);
	
	// vector<Point2f> objPts, scenePts;
	// for(int i=0;i<matches.size();i++){
		// Point2f kpo = kptsObj[matches[i].queryIdx].pt;
		// Point2f kps = kptsScene[matches[i].trainIdx].pt;
		
		// objPts.push_back(kpo);
		// scenePts.push_back(kps);
	// }
		
	
	// Mat mask;
	// Mat transform = findHomography(objPts, scenePts, mask);
//	cout << "transform= " << transform << endl;

	
	vector<Point2f> usedObjPts(4), usedScenePts(4);
	// for(int i=0,c=0;i<matches.size() && c<4;i++){
		// if(mask.at<uchar>(i) == 1){
			// usedObjPts[c] = objPts[i];
			// usedScenePts[c] = scenePts[i];
			// c++;
		// }
	// }
	
	namedWindow("fit");
	
	usedObjPts[0] = Point2f(58/4,59/4);
	usedObjPts[1] = Point2f(2628/4,246/4);
	usedObjPts[2] = Point2f(2502/4,983/4);
	usedObjPts[3] = Point2f(64/4,1332/4);
	
	
	usedScenePts[0] = Point2f(1129/4,1047/4);
	usedScenePts[1] = Point2f(2852/4,1069/4);
	usedScenePts[2] = Point2f(2643/4,1959/4);
	usedScenePts[3] = Point2f(501/4,1930/4);

	
	
	Mat M = getPerspectiveTransform(usedScenePts, usedObjPts);
	cout << "M= " << M << endl;
	
	Mat fit;
	warpPerspective(scene, fit, M, scene.size());
	imshow("raw", scene);
	imshow("fit", fit);
	
	
	// Mat match;
	// vector<DMatch> best(matches.begin(), matches.begin()+5);
	// drawMatches(obj, kptsObj, scene, kptsScene, best, match);
	// imshow("matches", match);
	
	

	waitKey();
    return 0;
}
