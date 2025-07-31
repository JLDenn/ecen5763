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

static const char *prompts[] = {
	"First click on the top left (NW) corner of the deck",
	"Now click on the first angled corner to the right of the NW corner",
	"Click on the 2nd angled corner (the concave one) to the right of that",
	"Click on the NE corner of the deck",
	"Click on the SE corner of the deck (next to the house wall)",
	"Click on the SW corner of the deck (next to the wall again)",
	};

//-----------------------------------------------------------------------------------------------------------
// Process mouse clicks on the corners of the deck so we can calculate the correction matrix (once we have all 6 clicks)
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

//-----------------------------------------------------------------------------------------------------------
/** @function main */
int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv,
                             "{help h||}"
							 "{@m||Set file to store new M in, and enable click to warp}"
							 "{i||Image to apply perspective correction to. Use camera if not provided}"
							 "{c||Enable creation of new M file}");

    parser.about( "\nThis program displays image correction, and allows click-on-corners to generate M (-c option)\n" );
    
	if(!parser.check() || argc < 2 || parser.has("help")){
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
		cout << "Error getting image" << endl;
		return -1;
	}
	
	string mfile = parser.get<string>(0);
	
	double M_raw[3][3] = {{1.0,0.0,0.0}, {0.0,1.0,0.0}, {0.0,0.0,1.0}};
	Mat M = Mat(3,3,CV_64F,M_raw);
	Mat fit;
	
	if(!parser.has("c")){
		//We'll be reading the M file in
		
		FILE *f = fopen(mfile.c_str(), "r");
		if(!f){
			cout << "Unable to open M file for reading (" << mfile << "). To create a new M file, use -c option" << endl;
			return -1;
		}
		else{
			//Read in the file contents to M
			for(int i=0;i<3;i++){
				if(3 != fscanf(f, "%lf, %lf, %lf", &M_raw[i][0],&M_raw[i][1],&M_raw[i][2])){
					cout << "Error reading M file (" << mfile << ")" << endl;
					return -1;
				}
			}
			fclose(f);
		}
		
		//Demo the read in M file by displaying the raw and newly fit scenes
		imshow("raw", scene);
		cout << "Warping with: " << endl;
		cout << M << endl;
		warpPerspective(scene, fit, M, Size(OBJ_BORDER_SIZE*2 + DECK_WIDTH*PIXELS_TO_INCH, OBJ_BORDER_SIZE*2 + DECK_HEIGHT*PIXELS_TO_INCH));
	}
	else{
		//We'll be creating the M file
		
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

		FILE *f = fopen(mfile.c_str(), "w");
		if(!f){
			cout << "Error openning M file for writing (" << mfile << endl;
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

	imshow("fit", fit);
	
	waitKey();
    return 0;
}
