#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace cv;
using namespace std;

//Define names of colors so we can print out which color we'll be saving
static const char *chan[] = {"Red", "Green", "Blue"};


//Main function
int main(int argc, char** argv){
	
	//Verify CLI arguments are correct
	if(argc != 3){
		cout << "Usage: " << argv[0] << " <image filename format (ie. light_%04d.ppm)> R|G|B" << endl;
		return 1;
	}
	
	//Determine which color channel we'll be using.
	int ch;
	if(argv[2][0] == 'G' || argv[2][0] == 'g')
		ch = 1;
	else if(argv[2][0] == 'B' || argv[2][0] == 'b')
		ch = 2;
	else if(argv[2][0] == 'R' || argv[2][0] == 'r')
		ch = 0;
	else{
		cout << "Invalid color channel. must be 'b'/'B' or 'g'/'G' or 'b'/'B'" << endl;
		return 1;
	}

	cout << "***** Color to Component Grayscale - Justin Denning *****" << endl;
	cout << "Converting input frames from RGB to " << chan[ch] << " only grayscale" << endl;

	Mat frame;
	Mat channels[3];
	
	//ACCEPT BUFFER SHORTAGE ERROR POSSIBILITY
	char image[256];
	int imageIndex = 0;
	
	//Determine the folder that the source image frames are in. We'll be using the same location for the output frames.
	char outputFolder[256];
	strncpy(outputFolder, argv[1], sizeof(outputFolder));
	char *ptr = strrchr(outputFolder, '/');
	if(ptr)
		*ptr = '\0';
	else
		strcpy(outputFolder, ".");
	
	while(1){
		//ACCEPT SECURITY HOLE BY LETTING THE USER PROVIDE THE FORMAT STRING!!!!
		//Set the path/name of the next image frame we'll be reading in
		snprintf(image, sizeof(image), argv[1], ++imageIndex);
		
		//Read it in and ensure it actually loaded. Otherwise, we're done processing frames
		frame = imread(image);
		if(frame.empty())
			break;
		
		//Split the frame into component channels
		split(frame, channels);

		//Write out the selected color channel as a gray image (in the same folder as the source images)
		snprintf(image, sizeof(image), "%s/gray_%04d.pgm", outputFolder, imageIndex);
		imwrite(image, channels[ch]);
	}
	
	cout << "Complete, " << imageIndex << " total images written" << endl;
	return 0;
}
	