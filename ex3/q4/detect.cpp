#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace cv;
using namespace std;


#define WINDOW_NAME_PREV			"Previous Frame"
#define WINDOW_NAME_CUR				"Current Frame"
#define WINDOW_NAME_DIFF			"Difference"

//Define the application quit key (esc)
#define ESCAPE 		27


int main(int argc, char** argv){
	
	
	if(argc != 2){
		cout << "Usage: " << argv[0] << " <path to images with %d format for numbers>" << endl;
		return 1;
	}
	
	
	//Create our display window
	namedWindow(WINDOW_NAME_PREV);
	namedWindow(WINDOW_NAME_CUR);
	namedWindow(WINDOW_NAME_DIFF);


	Mat frame, prev;
	Mat channels[3], prevChannels[3], diffChannels[3];
	Mat diff;
	
	//ACCEPT BUFFER SHORTAGE ERROR POSSIBILITY
	char image[256];
	int imageIndex = 0;
	
	//ACCEPT SECURITY HOLE BY LETTING THE USER PROVIDE THE FORMAT STRING!!!!
	while(1){
		snprintf(image, sizeof(image), argv[1], ++imageIndex);
		frame = imread(image);
		if(frame.empty())
			break;
		
		if(!prev.empty()){
			split(frame, channels);
			split(prev, prevChannels);
			
			vector<Mat> chV;
			for(int i=0;i<3;i++){
				absdiff(channels[i], prevChannels[i], diffChannels[i]);
				chV.push_back(diffChannels[i]);
			}
			
			merge(chV, diff);
			
			
			imshow(WINDOW_NAME_PREV, prev);
			imshow(WINDOW_NAME_CUR, frame);
			imshow(WINDOW_NAME_DIFF, diff);
			
			
			if(waitKey(30) == ESCAPE)
				break;
			
		}
		else{
			//First pass we'll move the windows
			moveWindow(WINDOW_NAME_CUR, frame.size().width, 0);
			moveWindow(WINDOW_NAME_DIFF, frame.size().width*2, 0);
			
		}
		
		
		
		prev = frame.clone();
	}



	destroyWindow(WINDOW_NAME_PREV);
	destroyWindow(WINDOW_NAME_CUR);
	destroyWindow(WINDOW_NAME_DIFF);
	return 0;
}
	