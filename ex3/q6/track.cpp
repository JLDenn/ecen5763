
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "track.hpp"
#include "img.hpp"

#define GHOST_DELAY				10
#define OBJECT_RED_THRESH		200		//Determined by trial and error, as well as examaning the histogram of several image frames.
#define OBJECT_MIN_SIZE			2		//Minimum number of pixels in x AND y before considered an object

#define SEARCH_FIELD_SIZE		100		//Size of the area around the previous detection that we'll use in the first pass on a new frame.
										// This (rightly) assumes the object will more likely be near where it was in the last frame.

using namespace std;


////////////////////////////////////////////////////////////////////////////////////
//Main function
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv){
	
	cout << "***** Laser Tracking - Justin Denning *****" << endl;
	
	//Verify CLI arguments are correct
	if(argc != 2){
		cout << "Usage: " << argv[0] << " <image filename format (ie. gray_%04d.pgm)>" << endl;
		return 1;
	}
	

	//ACCEPT BUFFER SHORTAGE AS AN ERROR POSSIBILITY (won't overflow though)
	char image[256];
	char outputFolder[256];
	int imageIndex = 0;
	
	//Determine the folder that the source image frames are in. We'll be using the same location for the output frames.
	strncpy(outputFolder, argv[1], sizeof(outputFolder));
	char *ptr = strrchr(outputFolder, '/');
	if(ptr)
		*ptr = '\0';
	else
		strcpy(outputFolder, "."); 
	
	Img prev;			//Image class to hold the previous image frame (just before the current one)
	int foundCount = 0;
	int found = 0;		//This will be used to "time out" the hold of previous frame detections to "cover" the gap frames where
						// we didn't detect the laser (a simple form of something like a Kalman filter)
						//	It uses the GHOST_DELAY to hold a previously found spot on for that number of future frames.
	obj_t obj;			//Declare the last object found outside the while loop so it can have some persistance for the frames we can't find the object. 
	
	obj_t targetSrchField = {.x=0,.y=0,.w = SEARCH_FIELD_SIZE, .h = SEARCH_FIELD_SIZE};
	while(1){
		
		//ACCEPT SECURITY HOLE BY LETTING THE USER PROVIDE THE FORMAT STRING!!!!
		//Set the path/name of the next image frame we'll be reading in
		snprintf(image, sizeof(image), argv[1], ++imageIndex);
		
		Img img(image);
		if(!img.loaded())
			break;

		Img orig(img);	//Hold on to the original image since the process of doing the diff/sobel transform modifies the img.
		
		//Now we'll take the differnce between this frame and the previous one, then we'll do a sobel transform on the diff image
		int count = 0;
		if(imageIndex > 1){
			
			//Perform the diff, the result ends up in img.
			img.diff(prev);
			
			//Save the unmodified (before we add the cross and box) for use as a diff in the next loop
			prev.copy(orig);
			
			//Peform a ~sobel transform to bring out the laser dot. 
			Img x;
			Img sobel;
			
			int sobelx[] = {-128, 0, 128, -256, 0, 256, -128, 0, 128};	//Scaled by Img::convolve by 128, so 128=1, 256=2, etc.
			img.convolve(x, sobelx);
			
			int sobely[] = {128, 256, 128, 0, 0, 0, -128, -256, -128};
			img.convolve(sobel, sobely);
			
			sobel.mul(x);	
			
			//We now have an approximate Sobel transform completed (after the initial frame diffs).		
			
			//Look for objects that are above the threshold of OBJECT_RED_THRESH
			//	However, if no object is found, we'll try 3 more times with lower threshold levels (10 lower each loop)
			for(int t=0;t<4;t++){
				vector<obj_t> vObj;
				count = 0;
				
				//For the first try (of 4), if the previous frame had a detection, we'll try searching in a reduce field of view
				//	for a first try since the object will most likely be in that area again. 
				if(t == 0 && found == GHOST_DELAY-1)
					count = sobel.objects(&vObj, OBJECT_RED_THRESH - t*10, OBJECT_MIN_SIZE, &targetSrchField);
				if(!count)
					count = sobel.objects(&vObj, OBJECT_RED_THRESH - t*10, OBJECT_MIN_SIZE);
				
				if(count){
					//Draw cross to indicate where the dot is. 
					obj = vObj.at(0);
					found = GHOST_DELAY;
					foundCount++;
					
					//Get ready for the next frame "quick" search. We'll update the first pass search field for the next frame
					targetSrchField.x = obj.x;
					targetSrchField.y = obj.y;
					break;
				}
			}
			
			if(found){
				found--;
				orig.mark(&obj);
			}
			
			
			//Assemble the name of the output frame, and save the marked frame.
			char name[32];
			snprintf(name, sizeof(name), "%s/marked_%04d.pgm", outputFolder, imageIndex);
			orig.save(name);
		}
		else	//First frame actions
			prev.copy(orig);	//Save the unmodified (without cross and box) for use as a diff in the next loop
		
		//Print progress details, '.' for every frame or frame number on 100s. Use X instead of . for any frame that had an object found in it. 
		if(imageIndex % 100 == 0)
			cout << imageIndex;
		else if(count)
			cout << "X";
		else
			cout << ".";
		cout.flush();
	}
	
	cout << "Complete, " << imageIndex << " total images written (found laser object in " << foundCount << " frames)" << endl;
	return 0;
}
	