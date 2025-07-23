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



static bool comp(DMatch a, DMatch b){
	return a.distance < b.distance;
}

//-----------------------------------------------------------------------------------------------------------
static void mapPoint(Point2f p, Point2f *pt, Mat M){
	
	double d[] = {p.x, p.y, 1.0};
	Mat pm(3, 1, CV_64F, d);
	Mat pd = M * pm;		

	pt->x = pd.at<double>(0) / pd.at<double>(2);
	pt->y = pd.at<double>(1) / pd.at<double>(2);	
}

//-----------------------------------------------------------------------------------------------------------
static void targetPoint(Point2f p, Vec2f *v){
	
	Point2f D0(305, 155);		//origin of the deck (where the gun is)
	double theta_offset	= 10.0;	//Offset for the pan direction (due to mounting) indicates aligned with deck, degrees
	double phi_mount = -30.0;		//Mounting angle (in tilt) of the pan & tilt (degrees)
	double hight = 144.0;			//gun mount hight (above the deck, inches)
	
	Point2f shot = D0 - p;
	
	double theta = atan(shot.y / shot.x) * 180.0 / 3.14159 + theta_offset;
	cout << "theta=" << theta << endl;
	
	//Since the pan&tilt is mounted at an angle (not flat), as we move in X(pan) we also change the tilt angle.
	//	This is the component in pan based on the x(pan) position. 
	double x_comp = cos((theta - theta_offset) * 3.14159 / 180.0) * phi_mount;
	cout << "tilt component from pan=" << x_comp << endl;
	
	double range = sqrt(shot.y*shot.y + shot.x*shot.x);
	cout << "range=" << range << endl;
	
	double drop_cor = pow(2.71828182845904, 0.01 * range) - 1; 
	cout << "drop_cor=" << drop_cor << endl;
	
	double phi = -atan(hight / range) * 180.0/3.14159;
	cout << "phi raw=" << phi << endl;

	//Combine the raw phi and the drop and pan-component corrections
	phi -= x_comp;
	phi += drop_cor;
	
	//Return the calculated angles
	(*v)[0] = theta;	//pan
	(*v)[1] = phi;		//tilt
}

//-----------------------------------------------------------------------------------------------------------
void mouse(int action, int xp, int yp, int, void *frame){
	if(action == EVENT_LBUTTONDOWN){
		//Mark click point on the image
		Mat *fit = (Mat*)frame;
		circle(*fit, Point(xp, yp), 5, Scalar(0,0,0), 8); 
		imshow("fit", *fit);
		
		//Calculate distance from top left deck corner in inches
		double x = (xp-50)*0.1285;
		double y = (yp-50)*0.1285;
		cout << endl << "Shooting chicken at " << x << "," << y << " inches from top left" << endl;
		
		Vec2f v;
		targetPoint(Point2f(x,y), &v);
		cout << "pan= " << v[0] << "°, tilt= " << v[1] << "° to hit target at " << x << "," << y << endl;	
	}
	
}

/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
                             //"{@obj||Image to process}"
							 "{@scene||Scene to search for the image in}"
							 "{p||Point to shoot (at click point)}");

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
	
	Mat scene = imread(parser.get<string>(0), IMREAD_GRAYSCALE);
	if(scene.empty()){
		cout << "Error opening image at " << parser.get<string>(0) << endl;
		return 1;
	}
	
	//Mat scene, obj;
	//resize(iobj, obj, Size(0,0), 0.25, 0.25);
	//resize(iscene, scene, Size(0,0), 0.25, 0.25);
	
	
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

	
	vector<Point2f> mappedCoords(4), srcCoords(4);
	// for(int i=0,c=0;i<matches.size() && c<4;i++){
		// if(mask.at<uchar>(i) == 1){
			// usedObjPts[c] = objPts[i];
			// usedScenePts[c] = scenePts[i];
			// c++;
		// }
	// }
	
	// namedWindow("fit");
	
	//Resultant coordinates for the corners of the deck (and BBQ) to provide 1300 pixels / 167 inches (7.784 px/in)
	mappedCoords[0] = Point2f(50,50);		//NW
	mappedCoords[1] = Point2f(2424,237);	//NE
	mappedCoords[2] = Point2f(2307,1000);	//SE (using BBQ NE corner)
	mappedCoords[3] = Point2f(50,1350);		//SW
	
	//Points on the pre-corrected image that line up with the features as indicated
	srcCoords[0] = Point2f(1129,1047);		//NW deck corner
	srcCoords[1] = Point2f(2852,1069);		//NE deck corner
	srcCoords[2] = Point2f(2643,1959);		//NE BBQ corner (absolute corner)
	srcCoords[3] = Point2f(501,1930);		//SW deck corner

	
	Mat M = getPerspectiveTransform(srcCoords, mappedCoords);
	
	Mat fit;
	warpPerspective(scene, fit, M, Size(mappedCoords[1].x+50, mappedCoords[3].y+50));
	
	
	for(int i=0;i<4;i++){
		Point2f pt;
		mapPoint(srcCoords[i], &pt, M);

		circle(fit, Point((int) pt.x, (int) pt.y), 5, Scalar(0,0,0), 4);
	}

	
	
	
	if(parser.has("p")){
		namedWindow("fit");
		setMouseCallback("fit", mouse, &fit);
	}
	
	
	//imshow("raw", scene);
	imshow("fit", fit);
	
	
	// Mat match;
	// vector<DMatch> best(matches.begin(), matches.begin()+5);
	// drawMatches(obj, kptsObj, scene, kptsScene, best, match);
	// imshow("matches", match);
	

	waitKey();
    return 0;
}
