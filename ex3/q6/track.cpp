
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "track.hpp"
#include "img.hpp"

using namespace std;


////////////////////////////////////////////////////////////////////////////////////
//Main function
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv){
	
	//Verify CLI arguments are correct
	if(argc != 2){
		cout << "Usage: " << argv[0] << " <image filename format (ie. gray_%04d.pgm)>" << endl;
		return 1;
	}
	

	cout << "***** Laser Tracking - Justin Denning *****" << endl;
	
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
	
	Img prev;
	
	int frame = 0;
	while(1){
		frame++;
		
		//ACCEPT SECURITY HOLE BY LETTING THE USER PROVIDE THE FORMAT STRING!!!!
		//Set the path/name of the next image frame we'll be reading in
		snprintf(image, sizeof(image), argv[1], ++imageIndex);
		
		Img img(image);
		if(!img.loaded())
			break;

		Img orig(img);
		//orig.save("raw.pgm");
		
		
		obj_t obj = {0};
		int count = 0;

		//Now we'll take the differnce between this frame and the previous one, then we'll do a sobel transform on the diff image
		if(prev.loaded()){
			//Perform the diff, the result ends up in prev.
			prev.absDiff(img);
			
			//Peform a ~sobel transform to bring out the laser dot. 
			Img x;
			Img sobel;
			
			int sobelx[] = {-128, 0, 128, -256, 0, 256, -128, 0, 128};
			prev.convolve(x, sobelx);
			
			int sobely[] = {128, 256, 128, 0, 0, 0, -128, -256, -128};
			prev.convolve(sobel, sobely);
			
			sobel.mul(x);			
			
			//look for objects that are above the threshold of 200 (determined by trial and error, as well as examaning the histogram of several images. 
			vector<obj_t> vObj;
			count = sobel.objects(&vObj, 200, 3);
			if(count){
				//Draw cross to indicate where the dot is. 
				//for(int i=0;i<count;i++){
					obj = vObj.at(0);
					orig.mark(&obj);
				//}
			}
			else if(obj.x || obj.y)
				orig.mark(&obj);
			
			//Assemble the name of the output frame, and save the marked frame.
			char name[32];
			snprintf(name, sizeof(name), "%s/marked_%04d.pgm", outputFolder, frame);
			orig.save(name);
		}
		
		//Print progress details, '.' for every frame, frame number on 100s, and X for any frame that had an object found in it. 
		if(frame % 100 == 0)
			cout << frame;
		else if(count)
			cout << "X";
		else
			cout << ".";
		cout.flush();
	
		prev.copy(img);
	}
	
	cout << "Complete, " << imageIndex << " total images written" << endl;
	return 0;
}
	