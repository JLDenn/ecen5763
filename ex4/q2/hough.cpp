#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"


#include <iostream>
#include <unistd.h>
#include <pthread.h>

using namespace cv;
using namespace std;

#define WIN_NAME_OUT "Detected Lines (in red) - Probabilistic Line Transform"
#define ESCAPE		27
#define THREAD_COUNT	4

#define FRAME_STATE_FREE		0		//Moved to WORKING by scheduler
#define FRAME_STATE_WORKING		1		//Moved to READY by thread				//THREAD OWNS THE DATA
#define FRAME_STATE_READY		2		//Moved to FREE by scheduler

struct {
	int state;
	int frameNumber;
	Mat src;
	Mat out;
}frame_data[THREAD_COUNT] = {0};

pthread_t threads[THREAD_COUNT];
volatile bool running = true;

int threshLine = 59;
int threshCanny = 28;


void *processFrame(void* arg){
	long threadIdx = (long)arg;
	
	
	while(running){
	
		while(frame_data[threadIdx].state != FRAME_STATE_WORKING && running)
			usleep(10);
		
		if(!running)
			break;
		
		
		Mat frame, dst;
		// Convert frame read to grayscale
		//resize(frame_data[threadIdx].src, frame, Size(640, 360), 0, 0, INTER_CUBIC);
		cvtColor(frame_data[threadIdx].src, frame, COLOR_BGR2GRAY);

		// Edge detection
		Canny(frame, dst, threshCanny, threshCanny*3, 3);
		// Copy edges to the images that will display the results in BGR
		cvtColor(dst, frame_data[threadIdx].out, COLOR_GRAY2BGR);
		
		
		// Probabilistic Line Transform
		vector<Vec4i> linesP; // will hold the results of the detection
		HoughLinesP(dst, linesP, 1, CV_PI/180, threshLine, 50, 10); // runs the actual detection
		
		
		
		// Draw the lines
		for( size_t i = 0; i < linesP.size(); i++ ){
			Vec4i l = linesP[i];
			
			//Filter (remove) lines that are flatter than +/- 20° from horizontal
			//	tan(20°) ~ 0.364 so any ratio of abs(rise/run) < 0.364 will be ignored
			// 	For speed, we'll use run/rise >= 3 will be ignored (after we check for /0 errors)
			int r = l[1] == l[3] ? 10000 : abs( (l[0]-l[2]) / (l[1] - l[3]) );
			if(r <= 3){
				//cout << "("<<l[0]<<","<<l[1]<<") - ("<<l[2]<<","<<l[3]<<") = " << r<<  endl;
				line( frame_data[threadIdx].src, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
			}
		}

		
		frame_data[threadIdx].state = FRAME_STATE_READY;
	}
	return NULL;
}



static int printUsage(const char* app){
	cout << "Usage: " << app << " [video/path] [frames-to-skip]\n\tif video/path is not provided, camera 0 will be used by default" << endl;
	return 1;
}



int main(int argc, char** argv){

	VideoCapture cap;
	
	//Check for arguments. No arguments means we'll use the camera
	if(argc < 1)
		cap.open(0);
	else
		cap.open(argv[1]);
	
	if(!cap.isOpened()){
		cout << "Error opening " << (argc < 1 ? "camera 0" : argv[1]) << endl;
		return 1;
	}
	
	int framesToSkip = 0;
	if(argc >= 1){
		framesToSkip = atoi(argv[2]);
	}
	
	
	for(long i=0;i<THREAD_COUNT;i++){
		frame_data[i].state = FRAME_STATE_FREE;
		pthread_create(&threads[i], NULL, processFrame, (void*) i);
	}
	
	

	
	//Create and move the source window down
	namedWindow(WIN_NAME_OUT);
	//moveWindow("Source", 0, 770);
	createTrackbar("lines", WIN_NAME_OUT, &threshLine, 200);
	createTrackbar("canny" , WIN_NAME_OUT, &threshCanny, 100);

	int thdPush = 0;
	int frameNumberIn = 0;
	int frameNumberOut = 1;
	while(1){
		Mat frame;
		
		//Find waiting thread to send new frame data to
		for(int i=0;i<THREAD_COUNT;i++){
			if(frame_data[i].state == FRAME_STATE_FREE){
				
				//cout << "sending frame to thread " << i << endl;
				cap >> frame_data[i].src;
				if(frame_data[i].src.empty()){
					cout << "No more video frames available" << endl;
					break;
				}
				
				//Skip the first n number of frames (if provided as CLI)
				if(framesToSkip){
					framesToSkip--;
					cout << "Skipping frame (" << framesToSkip << " left)" << endl;
					continue;
				}

				frame_data[i].frameNumber = ++frameNumberIn;
				frame_data[i].state = FRAME_STATE_WORKING;
				break;
			}
		}		
		
		
		bool newFrame = false;
		for(int i=0;i<THREAD_COUNT;i++){
			if(frame_data[i].state == FRAME_STATE_READY && frame_data[i].frameNumber == frameNumberOut){
				
				//imshow("Source", frame_data[i].src);
				imshow(WIN_NAME_OUT, frame_data[i].src);
				newFrame = true;
				
				frameNumberOut++;
				frame_data[i].state = FRAME_STATE_FREE;
				break;
			}
		}

		if(newFrame){
			if(waitKey(5) == ESCAPE)
				break;
		}
	}
	
	
	running = false;
	for(int i=0;i<THREAD_COUNT;i++)
		pthread_join(threads[i], NULL);
	
	
    return 0;
}