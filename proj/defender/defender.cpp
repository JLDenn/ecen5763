/**
	@author	Justin Denning
	@date	12 July 2025
	
	@Description

	
**/
#include "opencv2/objdetect.hpp"
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
	const char *video;			//NULL (no -v argument) indicates use camera 
	int minArea;
	int motionExpansion;	
	const char *demo;			//Save displayed frames to this location, if !NULL
	const char *mfile;			//Location of the M file to use for coordinate translations
	const char *cascadefile;	//Location of the cascade.xml file we'll use for object classification
	bool staticTargeting;		//If true, don't use motion extrapolation
	int motionThresh;			//Threshold to use during absdiff to determine what counts as motion
	bool verbose;				//Enables verbose commenting
	bool dry;					//Run the system dry (no pan & tilt), simply indicate where shots would be taken
}opts = {0};

using namespace cv;
using namespace std;

//Targetting thread communication
static target_t targetMem;

static Mat M;


#define DECK_HEIGHT			167						//Inches S-N
#define DECK_WIDTH			290						//Inches W-E (visible)
#define PIXEL_TO_INCH		(600.0/DECK_HEIGHT)		//AFTER point has been warped to correct for perspective
#define DECK_FRAME_BORDER	50

#define PT_LOC_X			305
#define PT_LOC_Y			155

//-------------------------------------------------------------------
//Read the M matrix from the m.mat file (or overridden path)
static Mat readWarp(){

	//Read in the M file that we'll use for coordinate translations
	FILE *f = fopen(opts.mfile, "r");
	if(!f){
		cout << "Unable to open M file to reading read in the coordinate translation matrix (" << opts.mfile << ")" << endl;
		exit(-1);
	}

	//Read in the file contents to M
	static double M_raw[3][3] = {{1.0,0.0,0.0}, {0.0,1.0,0.0}, {0.0,0.0,1.0}};	//Must be static since Mat data must be valid upon return.
	for(int i=0;i<3;i++){
		if(3 != fscanf(f, "%lf, %lf, %lf", &M_raw[i][0],&M_raw[i][1],&M_raw[i][2])){
			cout << "Error reading values from M file (" << opts.mfile << ")" << endl;
			exit(-1);
		}
	}
	fclose(f);

	return Mat(3,3, CV_64F, M_raw);
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
	
	pt.x -= DECK_FRAME_BORDER;
	pt.x /= PIXEL_TO_INCH;
	pt.y -= DECK_FRAME_BORDER;
	pt.y /= PIXEL_TO_INCH;
	
	return pt;
}

//-------------------------------------------------------------------
//Convert a deck coordinate (in inches) to pre-warped image pixel coordinates
static Point2f pointUnWarp(Point2f p, Mat M){
	p.x *= PIXEL_TO_INCH;
	p.x += DECK_FRAME_BORDER;
	p.y *= PIXEL_TO_INCH;
	p.y += DECK_FRAME_BORDER;
	
	double d[] = {p.x, p.y, 1.0};
	Mat pm(3, 1, CV_64F, d);
	Mat pd = M.inv() * pm;		

	Point2f pt;
	pt.x = pd.at<double>(0) / pd.at<double>(2);
	pt.y = pd.at<double>(1) / pd.at<double>(2);		
	return pt;
}

//--------------------------------------------------------------------
//Check if point is actually on the deck (provide in inches from NW corner) 
// p - in inches from NW corner of the deck
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
	
	Point2f D0(PT_LOC_X, PT_LOC_Y);		//origin of the deck (where the gun is)
	double theta_offset	= 0.0;	//Offset for the pan direction (due to mounting) indicates aligned with deck, degrees
	double phi_mount = -30.0;		//Mounting angle (in tilt) of the pan & tilt (degrees)
	double hight = 144.0;			//gun mount hight (above the deck, inches)
	
	Point2f shot = D0 - p;
	
	double theta = atan(shot.y / shot.x) * 180.0 / 3.14159 + theta_offset;
	
	if(opts.verbose)
		cout << "theta=" << theta << endl;
	
	//Since the pan&tilt is mounted at an angle (not flat), as we move in X(pan) we also change the tilt angle.
	//	This is the component in pan based on the x(pan) position. 
	double x_comp = cos((theta - theta_offset) * 3.14159 / 180.0) * phi_mount;
	if(opts.verbose)
		cout << "tilt component from pan=" << x_comp << endl;

	//This is the scale we'll need to apply back to the pan after we know the tile adjustment
	double y_comp = sin((theta - theta_offset) * 3.14159 / 180.0);
	if(opts.verbose)
		cout << "y_comp for back correction to pan=" << y_comp << endl;

	double range = sqrt(shot.y*shot.y + shot.x*shot.x);
	if(opts.verbose)
		cout << "range=" << range << endl;
	
	double drop_cor = pow(2.71828182845904, 0.01 * range) - 1; 
	if(opts.verbose)
		cout << "drop_cor=" << drop_cor << endl;
	
	double phi = -atan(hight / range) * 180.0/3.14159;
	if(opts.verbose)
		cout << "phi raw=" << phi << endl;

	//Combine the raw phi and the drop and pan-component corrections
	phi -= x_comp;
	phi += drop_cor;
	
	theta += phi * y_comp/2;

	//Return the calculated angles
	(*v)[0] = theta;	//pan
	(*v)[1] = phi;		//tilt
}




#define IDLE_WAIT	5000
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//						TARGETTING THREAD
//Thread that controls the firing of the squirt gun and interface with the P&T
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * targetingThread(void *arg){

	target_t *target = (target_t*) arg;
	PanTilt pt;
	
	if(!opts.dry){
		if(!pt.openPort())
			exit(-1);

		pt.home();
	}
	
	long idle = 0;
	while(target->running){
		
		if(target->engage){
			
			Vec2f v;
			targetPoint(target->targetLocation, &v);

			if(opts.verbose){
				Point2f p_unwarp = pointUnWarp(targetMem.targetLocation, M);
				cout << "Target at pixel: " << (p_unwarp.x + DECK_FRAME_BORDER) << "," << (p_unwarp.y + DECK_FRAME_BORDER) << endl;
				cout << "Shooting at: " << target->targetLocation.x << "," << target->targetLocation.y << " (inches from NW corner)" << endl;
				cout << "Fireing solution: " << v[0] << "," << v[1] << " deg" << endl;
			}

			if(opts.dry){
				//Simply delay so the targetting icon can display for ~2 seconds
				usleep(2 * 1000000);
			}
			else{
				pt.moveTo((int)(v[0]*10), (int)(v[1]*10));
				pt.active(true, 1000);
			}
			
			
			target->engage = false;
			idle = IDLE_WAIT;
		}
		else if(--idle == 0){
			if(!opts.dry)
				pt.moveTo(0,0);
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
	
	threshold( diff, threshFrame, opts.motionThresh, 255, THRESH_BINARY);
	
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


//---------------------------------------------------------------------
void onClick(int action, int x, int y, int, void*){
	if(action == EVENT_LBUTTONDOWN){
		if(!targetMem.engage){
			targetMem.targetLocation = pointWarp(Point2f(x,y), M);
			targetMem.engage = true;
		}
	}
}

//------------------------------------------------------------------
//	Process CLI arguments
static void readOpts(int argc, char *argv[]){

	int opt;
	
	//Defaults
	opts.minArea = 1000;
	opts.motionExpansion = 10;
	opts.motionThresh = 40;
	opts.demo = NULL;
	opts.mfile = "m.mat";
	opts.cascadefile = "chicken_cascade.xml";
	
	while( (opt = getopt(argc, argv, "DsVhtv:a:e:d:c:m:M:") ) != -1){
		switch(opt){
		case 't':
			opts.test = true;
			break;
			
		case 'v':
			opts.video = optarg;
			cout << "Using " << opts.video << " as input source" << endl;
			break;
			
		case 'V':
			opts.verbose = true;
			break;
			
		case 'D':
			opts.dry = true;
			cout << "Running dry (no pan & tilt control)" << endl;
			break;
			
		case 'M':
			opts.mfile = optarg;
			cout << "Overriding M file location: " << opts.mfile << endl;
			break;

		case 'c':
			opts.cascadefile = optarg;
			cout << "Overriding cascade file location: " << opts.cascadefile << endl;
			break;
		
		case 'a':
			opts.minArea = atoi(optarg);
			cout << "Overriding minArea: " << opts.minArea << endl;
			break;
			
		case 'e':
			opts.motionExpansion = atoi(optarg);
			cout << "Overriding motionExpansion: " << opts.motionExpansion << endl;
			break;
			
		case 'd':
			opts.demo = optarg;
			cout << "Saving annotated frames in the format: " << opts.demo << endl;
			break;

		case 'm':
			opts.motionThresh = atoi(optarg);
			cout << "Overriding motion threshold (following absdiff()): " << opts.motionThresh << endl;
			break;

		case 's':
			opts.staticTargeting = true;
			cout << "Using static targetting: no motion extrapolation" << endl;
			break;
		
		case 'h':
		default:
			fprintf(stderr, "Usage: %s \n\t[-t = test run] \n"
				"\t[-t = run unit tests and exit]\n"
				"\t[-D = run dry, disable the pan & tilt and simply indicate where shots would be taken]\n"
				"\t[-v <video/path> = use file as video source] \n"
				"\t[-a <minArea> = motion threshold (default: %d)] \n"
				"\t[-e <expansion> = pixels to expand motion rect in all directions (default: %d)]\n"
				"\t[-M <mfile/loc> = override matrix file location (default: %s)]\n"
				"\t[-d <fname%%04d.jpg> = enable frame saving]\n"
				"\t[-c <path/to/cascade.xml> = cascade file to use (default: %s)\n"
				"\t[-m <motionThresh> = threshold used after absdiff() (default: %d)]\n"
				"\t[-s = static targeting. Do not use motion extrapolation, shoot at the latest position]\n"
				"\t[-V = verbose output]\n", 
				argv[0], opts.minArea, opts.motionExpansion, opts.mfile, opts.cascadefile, opts.motionThresh);
			exit(-1);
		}
	}
}


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//														MAIN
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]){
	
	//Read the CLI options/commands
	readOpts(argc, argv);
	
	
	//Instantiate the pan & tilt object (and run tests is directed)
	if(opts.test){
		Object obj;
		if(obj.test())
			exit(-1);
		
		PanTilt pt;
		if(pt.test())
			exit(-1);
		
		cout << "Object and P/T tests complete" << endl;
		return 0;
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

	
	CascadeClassifier chicken_cascade;
	if(!chicken_cascade.load( opts.cascadefile ) ){
        cout << "Error loading cascade file: " << opts.cascadefile << endl;
        exit(-1);
    };
	
	
	//Create thread that will interface with the squirt gun / pan&tilt
	pthread_t targetThreadID;
	targetMem.engage = false;
	targetMem.running = true;
	pthread_create(&targetThreadID, NULL, targetingThread, (void*) &targetMem);

	//Read the perspective correction matrix (m.mat)
	M = readWarp();
	
	//Declare the timespec values we'll use to calculate both actual and process frames per second
	struct timespec ts = {0};		//Start of process loop
	struct timespec ts_e = {0};		//End of process loop
	struct timespec ts_pf = {0};	//process loop after frame acquisition

	//Create our display window and attach a mouse callback so we can click to fire
	namedWindow(WINDOW_NAME);
	setMouseCallback(WINDOW_NAME, onClick);
	
	//Create the object tracking objects. These will track up to 3 objects as they move between frames
	vector<Object> objs;
	for(int i=0;i<MAX_TRACKED_OBJS;i++)
		objs.push_back(Object(0.5));	//0.5 means 50% of the most recent 10 object detections must be determined to be chickens before being shot

	
	//Declare the frames we'll need
	Mat avgBackgroundFrame;				//The background frame that holds the cumulative average
	Mat clrFrame;						//The raw image received from the camera/video file
	int frameIndex = 1;					//Keep track of what frame index we're at so we can properly name the frames with in demo mode (option -d)
	
	while(1){
		//Calculate the full loop time and the loop without the frame read delta times so we can display the FPS values on the frames.
		clock_gettime(CLOCK_MONOTONIC, &ts_e);
		double dt_full_loop = ((double)ts_e.tv_sec + ts_e.tv_nsec/1000000000.0) -  ((double)ts.tv_sec + ts.tv_nsec/1000000000.0);
		double dt_no_cam_loop = ((double)ts_e.tv_sec + ts_e.tv_nsec/1000000000.0) -  ((double)ts_pf.tv_sec + ts_pf.tv_nsec/1000000000.0);

		//Reset ts time to be the start of the loop
		ts = ts_e;
		
		
		//Read frame and verify it is valid
		cap >> clrFrame;
		if(clrFrame.empty()){
			cout << "Error reading frame" << endl;
			break;
		}

		//Get the post frame read time so we can calculate the FPS when we take out the frame read time.
		clock_gettime(CLOCK_MONOTONIC, &ts_pf);
		
		
		//Convert the raw image to a grayscale one so we can process it for motion
		Mat grayFrame;
		cvtColor(clrFrame, grayFrame, COLOR_BGR2GRAY);
		
		//Find motion and return the resulting contours
		vector<vector<Vec2i>> contours;
		int count = findMotion(grayFrame, &avgBackgroundFrame, &contours);
		
		//Output the number of contours found in verbose mode
		if(opts.verbose && count)
			cout << "Found " << count << " contours" << endl;
		
		//Declare and fill the frame we'll be annotating with all the motion boxes, the shot indicator and FPS overlay
		Mat annotatedFrame;
		clrFrame.copyTo(annotatedFrame);
		
		//Loop through all the contours (motion blocks) we found
		for(int i=0;i<count;i++){
			Rect r = boundingRect(contours.at(i));		//r is the raw bounding rectangle of the motion block
			Rect roi(r);								//roi will be expanded to provide a larger area to perform the Haar cascade check on
			
			//Define the centroid point to use as the objects current position
			Point2f p(roi.x + roi.width/2.0, roi.y + roi.height/2.0);
			
			//Expand the roi a bit then attempt to detect chicken in that area
			roi.x -= opts.motionExpansion;
			if(roi.x < 0) roi.x = 0;
			roi.y -= opts.motionExpansion;
			if(roi.y < 0) roi.y = 0;
			roi.width += opts.motionExpansion*2;
			if(roi.x + roi.width > annotatedFrame.cols) roi.width = annotatedFrame.cols-roi.x;
			roi.height += opts.motionExpansion*2;
			if(roi.y + roi.height > annotatedFrame.rows) roi.height = annotatedFrame.rows-roi.y;
			
			//Prepare the subframe around the motion, and run the cascade analysis on it
			Mat motionSubFrame(grayFrame, roi);
			
			std::vector<Rect> birds;
			chicken_cascade.detectMultiScale( motionSubFrame, birds);
			//If any chickens were detected in this small area, we know the object is a chicken, so we'll set the quality to 1.0
			float objQuality = birds.size() ? 1.0 : 0.0;

			//Mark the object using either green (chicken) or red (not chicken)
			rectangle(annotatedFrame, r, objQuality > 0.0 ? Scalar(0,255,0) : Scalar(0,0,255), 1);

			//Ensure the coordinates are within the deck bounds
			Point2f p_warp = pointWarp(p, M);		//p_warp is the perspective corrected coordinates (in inches from the NW deck corner)
			if(!validDeckPoint(p_warp))
				continue;

			//Add this object to one of the current object trackers, or create a new tracker if it doesn't fit an existing track (and we have one available)
			int j;
			int avail = -1;
			for(j=0;j<MAX_TRACKED_OBJS;j++){
				if(objs[j].checkLoc(p_warp)){
					//If we're here, this new point is a valid new entry in this object tracker (objs[j]), so we'll add the new entry
					objs[j].newDetect(p_warp, objQuality);
					
					//Put the object tracker's information over the motion box
					char det[32];
					snprintf(det, sizeof(det), "OBJ%d: %s", j, objs[j].stats()); 
					putText(annotatedFrame, det, Point(r.x, r.y-5), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,255), 1);
					
					//If we're not currently engaging a target (shooting), we'll check if there is enough entries and high enough quality to shoot at
					if(!targetMem.engage && objs[j].fireSolution()){
						//We need to shoot, so store the location we'll be shooting at in the targetMem structure, and set engage to true to send
						//	the target to the shooting thread.
						targetMem.targetLocation = objs[j].getShotLoc(!opts.staticTargeting, curTime() + 1000);	//1 seconds in the future
						targetMem.engage = true;
						
						//Clear the object tracker since we're shooting, and we'll need to redetect before we shoot again. 
						objs[j].flush();
					}
					break;
				}
				else if(!objs[j].active())
					avail = j;		//This tracker is not being used, so mark it as available
				else{
					if(objs[j].flush(4000)){	//Flush any objects where the last motion is 4 seconds old
						if(opts.verbose)
							cout << "Previous obj tracker " << j << " now availble" << endl;
						avail = j; 
					}
				}
			}
			
			if(j == MAX_TRACKED_OBJS && avail >= 0){
				if(opts.verbose)
					cout << "added to new obj tracker: " << avail << " -- " << p_warp.x << "," << p_warp.y << endl;
				objs[avail].newDetect(p_warp, objQuality);
			}
		}
	
		//If we're engaging a target (shooting), we'll draw the crosshairs icon so we can visually see where the shot is going.
		//	.engage is set when a shot requirement is determined, and cleared when the shot is complete
		if(targetMem.engage){
			//Since we're engaging, mark on the live video where we are attempting to shoot. 
			Point2f p_unwarp = pointUnWarp(targetMem.targetLocation, M);
			circle(annotatedFrame, p_unwarp, 20, Scalar(255,0,0), 5);
			line(annotatedFrame, Point2f(p_unwarp.x-25, p_unwarp.y), Point2f(p_unwarp.x+25, p_unwarp.y), Scalar(255,0,0), 3);
			line(annotatedFrame, Point2f(p_unwarp.x, p_unwarp.y-25), Point2f(p_unwarp.x, p_unwarp.y+25), Scalar(255,0,0), 3);
		}
		
		//Add FPS overlay
		char fps[64];
		snprintf(fps, sizeof(fps), "Actual/Process: %4.1lf / %4.1lf FPS", 1.0 / dt_full_loop, 1.0 / dt_no_cam_loop);
		putText(annotatedFrame, fps, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,0,255),2);
		
		//Display the image
		imshow(WINDOW_NAME, annotatedFrame);
		
		//If frame saving mode (demo mode) is enabled, we need to create the new frame file name and write it out.
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

	//We're on the way out, so flag the thread to terminate and wait for it to actually exit. 
	targetMem.running = false;
	pthread_join(targetThreadID, NULL);
	
	//Cleanup the window we created (close it)
	destroyWindow(WINDOW_NAME);

	//Return success
	return 0;
}