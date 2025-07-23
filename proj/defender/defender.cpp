/**
	@author	Justin Denning
	@date	12 July 2025
	
	@Description

	
**/

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <chrono>

#include "pantilt.hpp"
#include "object.hpp"
#include "defender.h"

#define MAX_TRACKED_OBJS	3

//Define the image size
#define IMG_WIDTH 	1280
#define IMG_HEIGHT	720


//Define the window name since we'll be referencing it multiple times.
#define WINDOW_NAME		"Live Video"

//Define the application quit key (esc)
#define ESCAPE 		27

struct {
	bool test;
	char *video;		//NULL (no -v argument) indicates use camera 
	int minArea;
	int motionExpansion;	
	char* demo;			//Save displayed frames to this location, if !NULL
}opts = {};

using namespace cv;
using namespace std;

//Targetting thread communication
static target_t targetMem;


#define DECK_HEIGHT			167						//Inches S-N
#define DECK_WIDTH			305						//Inches W-E
#define PIXEL_TO_INCH		(600.0/DECK_HEIGHT)		//AFTER point has been warped to correct for perspective

static Mat generateWarp(){
	vector<Point2f> mappedCoords(4), srcCoords(4);
	
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

	
	return getPerspectiveTransform(srcCoords, mappedCoords);
}

//-------------------------------------------------------------------
//Convert a point in raw pixel location to deck coordinates (inches from NW corner)
static Point2f pointWarp(Point2f p, Mat M){
	double d[] = {p.x, p.y, 1.0};
	Mat pm(3, 1, CV_64F, d);
	Mat pd = M * pm;		

	Point2f pt;
	pt.x = pd.at<double>(0) / pd.at<double>(2);
	pt.y = pd.at<double>(1) / pd.at<double>(2);	
	
	pt.x -= 50;
	pt.x /= PIXEL_TO_INCH;
	pt.y -= 50;
	pt.y /= PIXEL_TO_INCH;
	
	return pt;
}

//--------------------------------------------------------------------
//Check if point is actually on the deck (provide in inches from NW corner) 
static bool validDeckPoint(Point2f p){
	if(p.x < 0 || p.x > DECK_WIDTH - 50)		//Can't shoot right most 50 inches of the deck.
		return false;
	
	if(p.y < 0 || p.y > DECK_HEIGHT)
		return false;
	
	return true;
}


//--------------------------------------------------------------------
//Convert an x,y location from NW corner of the deck to Pan & Tilt angles
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





//---------------------------------------------------------------------
//Thread that controls the firing of the squirt gun and interface with the P&T
void * targetingThread(void *arg){
	
	return NULL;

	target_t *target = (target_t*) arg;
	PanTilt pt;
	
	if(!pt.openPort())
		exit(-1);

	pt.home();
	
	
	while(target->running){
		
		if(target->engage){
			
			Vec2f v;
			targetPoint(target->targetLocation, &v);
			pt.moveTo((int)(v[0]*10), (int)(v[1]*10));
			pt.active(true, 1000);
			
			target->engage = false;
		}
		
		usleep(1000);
	}
	
	return NULL;
}




//---------------------------------------------------------------------
static int findMotion(Mat frame, Mat *avg, vector<vector<Vec2i>> *contours){
	
	Mat blurred;
	Mat temp;
	static int framesStored = 0;

	
	if(avg->empty()){
		GaussianBlur(frame, blurred, Size(21, 21), 0);
		blurred.convertTo(*avg, CV_32F, 1.0/255.0);
		return 0;
	}

	avg->convertTo(temp, CV_8UC1);
	GaussianBlur(frame, blurred, Size(21, 21), 0);
		
	Mat diff, threshFrame;
	absdiff(blurred, temp, diff);
	accumulateWeighted(blurred, *avg, 0.2);
	
	//Don't check for motion in the first 10 frames stored (since we don't have our average built yet)
	if(++framesStored < 10)
		return 0;
	
	threshold( diff, threshFrame, 40, 255, THRESH_BINARY);
	
	Mat dilateFrame;
	dilate(threshFrame, dilateFrame, getStructuringElement(MORPH_RECT, Size(3,3)), Point(-1,-1), 20);
	
	vector<vector<Vec2i>> rawContours;
	findContours(dilateFrame, rawContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	
	for(int i=0;i<rawContours.size();i++){
		if(contourArea(rawContours[i]) >= opts.minArea)
			contours->push_back(rawContours[i]);
	}

	return contours->size();
}

//---------------------------------------------------------------------
static uint64_t curTime(){
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec * 1000ull) + ts.tv_nsec/1000000;
}


int main(int argc, char *argv[]){
	
	//------------------------------------------------------------------
	//	Process CLI arguments
	int opt;
	//Defaults
	opts.minArea = 1000;
	opts.motionExpansion = 10;
	opts.demo = NULL;
	int frameIndex = 1;
	
	while( (opt = getopt(argc, argv, "tv:a:e:d:") ) != -1){
		switch(opt){
		case 't':
			opts.test = true;
			break;
		case 'v':
			opts.video = optarg;
			cout << "Using " << opts.video << endl;
			break;
			
		case 'a':
			opts.minArea = atoi(optarg);
			break;
			
		case 'e':
			opts.motionExpansion = atoi(optarg);
			break;
			
		case 'd':
			opts.demo = optarg;
			break;
		
		default:
			fprintf(stderr, "Usage: %s \n\t[-t = test run] \n\t[-v video/path = use video file] \n\t[-a minArea = motion threshold] \n\t[-e expansion = distance to expand motion rect]\n", argv[0]);
			exit(-1);
		}
	}
	
	
	
	
	//Instantiate the pan & tilt object (and run tests is directed)
	vector<Object> objs(MAX_TRACKED_OBJS);
	if(opts.test){
		Object obj;
		if(obj.test())
			exit(-1);
		
		PanTilt pt;
		if(pt.test())
			exit(-1);
	}
	

	//Create the capture instance which will open the camera
	VideoCapture cap;
	if(opts.video)
		cap.open(opts.video);
	else
		cap.open(0);
	
	if(!cap.isOpened()){
		cout << "Unable to open video/camera for input" << endl;
		exit(-1);
	}
	
	//Configure the camera frame dimensions
	if(!opts.video){
		cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
		cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
	}
	
	
	//Create thread that will interface with the squirtgun/pan&tilt
	pthread_t targetThreadID;
	targetMem.engage = false;
	targetMem.running = true;
	pthread_create(&targetThreadID, NULL, targetingThread, (void*) &targetMem);


	Mat M = generateWarp();
	
	//Create our display window
	namedWindow(WINDOW_NAME);
	
	
	Mat avgBackgroundFrame;
	Mat clrFrame;
	
	while(1){
		//Read frame and verify it is valid
		cap >> clrFrame;
		if(clrFrame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}
		
		
		
		Mat grayFrame;
		cvtColor(clrFrame, grayFrame, COLOR_BGR2GRAY);
		
		vector<vector<Vec2i>> contours;
		int count = findMotion(grayFrame, &avgBackgroundFrame, &contours);
		
		// if(count)
			// cout << "Found " << count << " contours" << endl;
		
		Mat annotatedFrame;
		clrFrame.copyTo(annotatedFrame);
		
		for(int i=0;i<count;i++){
			Rect r = boundingRect(contours.at(i));
			rectangle(annotatedFrame, r, Scalar(0,255,0), 2);
			
			Point2f p(r.x + r.width/2.0, r.y + r.height/2.0);
			
			//Motion detected in rectangle r, check for chicken
			r.x -= opts.motionExpansion;
			if(r.x < 0) r.x = 0;
			r.y -= opts.motionExpansion;
			if(r.y < 0) r.y = 0;
			r.width += opts.motionExpansion*2;
			if(r.x + r.width > annotatedFrame.cols) r.width = annotatedFrame.cols-r.x;
			r.height += opts.motionExpansion*2;
			if(r.y + r.height > annotatedFrame.rows) r.height = annotatedFrame.rows-r.y;
			
			//Mat motionSubFrame(grayFrame, r);
			//imshow("ROI", motionSubFrame);

			Point2f p_warp = pointWarp(p, M);
			if(!validDeckPoint(p_warp))
				continue;

			int j=0;
			int avail = -1;
			for(;j<MAX_TRACKED_OBJS;j++){
				cout << "loop " << j << endl;
				
				if(objs[i].checkLoc(p_warp)){
					cout << "checked" << endl;
					objs[i].newDetect(p_warp);
					cout << "added" << endl;
					if(!targetMem.engage && objs[i].fireSolution()){
						cout << "idle and fireSoln ready" << endl;
						
						targetMem.targetLocation = objs[i].getShotLoc(curTime() + 1000);	//1 seconds in the future
						cout << "Shooting at " << targetMem.targetLocation.x << "," << targetMem.targetLocation.y << endl;
						
						targetMem.engage = true;
						objs[i].flush();
					}
					break;
				}
				else if(!objs[i].active())
					avail = i;
				else{
					if(objs[i].flush(4000))	//Flush any objects where the last motion is 4 seconds old
						avail = i; 
				}
			}
			
			if(j == MAX_TRACKED_OBJS && avail >= 0){
				cout << "added to new object: " << avail << " -- " << p_warp.x << "," << p_warp.y << endl;
				objs[avail].newDetect(p_warp);
			}

		}
	
		
		
		//Display the image
		imshow(WINDOW_NAME, annotatedFrame);
		if(opts.demo){
			char demofname[128];
			snprintf(demofname, sizeof(demofname), opts.demo, frameIndex++);	// !!!accepting the security hole of allowing user to provide format string!!!!
			imwrite(demofname, annotatedFrame);
		}
		
		
		//Give the system time to actually render the image to the screen, and check for an escape key press
		//	which will cause us to quit. 
		if(waitKey(1) == ESCAPE)
			break;
	}


	targetMem.running = false;
	pthread_join(targetThreadID, NULL);
	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME);


	
	//Return success
	return 0;
}