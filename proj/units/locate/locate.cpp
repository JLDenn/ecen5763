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

#define DECK_HEIGHT			167
#define DECK_WIDTH			290
#define PIXELS_TO_INCH		(600.0/DECK_HEIGHT)
#define OBJ_BORDER_SIZE		50

#define IMG_WIDTH			1280
#define IMG_HEIGHT			720

vector<Point2f> clicks;
bool processClicks = false;

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
/**
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
**/

static const char *prompts[] = {
	"First click on the top left (NW) corner of the deck",
	"Now click on the first angled corner to the right of the NW corner",
	"Click on the 2nd angled corner (the concave one) to the right of that",
	"Click on the NE corner of the deck",
	"Click on the SE corner of the deck (next to the house wall)",
	"Click on the SW corner of the deck (next to the wall again)",
	};


void cornerClick(int action, int x, int y, int, void *frame){
	if(action == EVENT_LBUTTONDOWN){
		if(!processClicks){
			Mat *scene = (Mat*)frame;
			circle(*scene, Point(x, y), 5, Scalar(0,0,0), 8); 
			imshow("raw", *scene);
			
			clicks.push_back(Point2f(x,y));
			if(clicks.size() < 6){
				cout << prompts[clicks.size()] << endl;
				return;
			}
			

			processClicks = true;
		}
	}
}


/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
							 "{@m||Set file to store new M in, and enable click to warp}"
							 "{i||Image to apply perspective correction to. Use camera if not provided}"
							 "{c||Enable creation of new M file}");

    parser.about( "\nThis program displays image correction, and allows click-on-corners to generate M (-p option)\n" );
    
	if(!parser.check() || argc < 2){
		parser.printMessage();
		return -1;
	}

	Mat scene;
	if(parser.has("i")){
		scene = imread(parser.get<string>("i"));
	}
	else{
		//Capture a single frame from the camera
		VideoCapture cap(0);
		if(!cap.isOpened()){
			cout << "Error openning camera to capture image" << endl;
			return -1;
		}

		cap.set(CAP_PROP_FRAME_WIDTH, IMG_WIDTH);
		cap.set(CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT);
		
		cap >> scene;
		cap.release();
	}
		
		
		if(scene.empty()){
			cout << "Error opening image at " << parser.get<string>(0) << endl;
			return -1;
		}
	

	
	double M_raw[3][3] = {{1.0,0.0,0.0}, {0.0,1.0,0.0}, {0.0,0.0,1.0}};
	
	//We'll be creating the M output file
	if(!parser.has("c")){
		FILE *f = fopen(parser.get<string>(0).c_str(), "r");
		if(!f){
			cout << "Unable to open M file for reading (" << parser.get<string>(1) << "). To create a new M file, use -c option" << endl;
			return -1;
		}
		else{
			//Read in the file contents to M
			for(int i=0;i<3;i++){
				if(3 != fscanf(f, "%lf, %lf, %lf", &M_raw[i][0],&M_raw[i][1],&M_raw[i][2])){
					cout << "Error reading M file (" << parser.get<string>(1) << ")" << endl;
					return -1;
				}
			}
			fclose(f);
		}
	}
		
		
	Mat M = Mat(3,3,CV_64F,M_raw);
	Mat fit;

	//Are we expected to create a new M file?
	if(parser.has("c")){
		namedWindow("raw");
		setMouseCallback("raw", cornerClick, &scene);	

		cout << prompts[0] << endl;
		imshow("raw", scene);	
		
		while(!processClicks){
			if(waitKey(10) == 27){
				cout << "Aborting... (M file not modified)" << endl;
				return 0;
			}
		}
		
		setMouseCallback("raw", NULL);
		
		//We should have all 6 click locations now, So we'll calculate the new M
		vector<Point2f> mappedCoords(4), srcCoords(4);
		srcCoords[0] = clicks[0];
		srcCoords[1] = clicks[3];
		srcCoords[2] = clicks[4];
		srcCoords[3] = clicks[5];
		
		mappedCoords[0] = Point2f(OBJ_BORDER_SIZE,OBJ_BORDER_SIZE);		//NW
		mappedCoords[1] = Point2f(OBJ_BORDER_SIZE + DECK_WIDTH * PIXELS_TO_INCH, OBJ_BORDER_SIZE + 24 * PIXELS_TO_INCH);	//NE
		mappedCoords[2] = Point2f(OBJ_BORDER_SIZE + DECK_WIDTH * PIXELS_TO_INCH, OBJ_BORDER_SIZE + DECK_HEIGHT * PIXELS_TO_INCH);	//SE 
		mappedCoords[3] = Point2f(OBJ_BORDER_SIZE, OBJ_BORDER_SIZE + DECK_HEIGHT * PIXELS_TO_INCH);		//SW	

		M = getPerspectiveTransform(srcCoords, mappedCoords);		
		
		warpPerspective(scene, fit, M, Size(OBJ_BORDER_SIZE*2 + DECK_WIDTH*PIXELS_TO_INCH, OBJ_BORDER_SIZE*2 + DECK_HEIGHT*PIXELS_TO_INCH));
		
		for(int i=0;i<4;i++){
			Point2f pt;
			mapPoint(srcCoords[i], &pt, M);

			circle(fit, Point((int) pt.x, (int) pt.y), 3, Scalar(255,0,0), 2);
		}

		FILE *f = fopen(parser.get<string>(1).c_str(), "w");
		if(!f){
			cout << "Error openning M file for writing (" << parser.get<string>(1) << endl;
			return -1;
		}
		
		for(int i=0;i<3;i++)
			fprintf(f, "%lf, %lf, %lf\n", 
					M.at<double>(i,0), M.at<double>(i,1),M.at<double>(i,2));
		
		fclose(f);
		cout << "New M file saved: " << endl;
		cout << M << endl;
		
		cout << "To verify point conversion works, there should be blue dots directly on top of the black circles in the fit image" << endl;
	}
	else {	//Demo the read in correction
		imshow("raw", scene);
		cout << "Warping with: " << endl;
		cout << M << endl;
		warpPerspective(scene, fit, M, Size(OBJ_BORDER_SIZE*2 + DECK_WIDTH*PIXELS_TO_INCH, OBJ_BORDER_SIZE*2 + DECK_HEIGHT*PIXELS_TO_INCH));
	}
	
	imshow("fit", fit);
	
	waitKey();
    return 0;
}
