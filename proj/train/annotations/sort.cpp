/**
	This code uses an output file from opencv_annotation to sort a collection of images into pos and neg folders. 
	When the object count is 0, the image is put in neg. for each object boxed, this code crops and resizes the selection to 64x128, and saves
	the result to pos directory. 
	
**/


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include <dirent.h>
#include <fstream>

#include <iostream>
#include <time.h>

using namespace cv;
using namespace std;

#define OUTPUT_SIZE		64,64

int main(int argc, char **argv){
	
	const char* keys ={
        "{help h|      | show help message}"
        "{@a	|<none>| the annotation text file with filenames and box coordinates}"
		"{r     | .    | relative path to prepend to paths in annoation file}"
//		"{b     |      | background image to use when subtracting bg features from the pos images. No subtraction if not included}"
    };
	
    CommandLineParser parser( argc, argv, keys );
    if ( argc < 1 || parser.has( "help" ) ){
        parser.printMessage();
		cout << "Example:" << endl;
		cout << argv[0] << " ann.txt" << endl;
        exit( 0 );
    }
	
	String objFilename = parser.get< String >( 0);
	String relPath = parser.get<string>("r");

	ifstream file(objFilename);
	if(!file.is_open()){
		cout << "Can't open " << objFilename << endl;
		return -1;
	}

	//Get the executable path 
	string exPath = ".";
	size_t p = string(argv[0]).rfind("/");
	if(p != string::npos)
		exPath = string(argv[0]).substr(0, p);

	// Mat bg;
	// if(parser.has("b")){		
		// bg = imread(parser.get<string>("b"));
		// if(bg.empty()){
			// cout << "Error reading background image: " << parser.get<string>("b") << endl;
			// return -1;
		// }
	// }
	
	int negIdx = 1;
	int posIdx = 1;
	string line;
	string ext;
	while(getline(file, line)){
		
		size_t pos = line.find(" ");
		string imgFilename = relPath;
		imgFilename.append("/").append(line.substr(0, pos));
		
		cout << imgFilename << endl;
		
		line = line.substr(pos + 1);
		
		pos = imgFilename.rfind(".");
		ext = imgFilename.substr(pos+1); 

		pos = line.find(" ");
		string objCountStr = line.substr(0, pos);
		line = line.substr(pos + 1);
		int objCount = atoi(objCountStr.c_str());
		
		if(!objCount){
			cout << "Saving " << imgFilename << " as negative image" << endl;
			
			//Save the image to neg folder
			char name[64];
			snprintf(name, sizeof(name), "%s/neg/%04i.%s", exPath.c_str(), negIdx++, ext.c_str());
			Mat neg = imread(imgFilename);
			
			if(neg.cols > 1024){
				Mat out;
				double scale = 1024.0/neg.cols;
				resize(neg, out, Size(1024,(int)(neg.rows * scale)));
				imwrite(name, out);
			}
			else
				imwrite(name, neg);
			
			
		}
		else{
			
			vector<int> vals;
			int v;
			while(sscanf(line.c_str(), "%i", &v) == 1){
				vals.push_back(v);
				
				pos = line.find(" ");
				if(pos == string::npos)
					break;
				line = line.substr(pos+1);			
			}
			
			int start = 0;
			Mat raw = imread(imgFilename);
			if(raw.empty()){
				cout << "error opening file: " << imgFilename << endl;
				return -1;
			}
			
			while(start + 4 <= vals.size()){
				
				Mat crop = raw(Rect(vals.at(start), vals[start+1], vals[start+2], vals[start+3]));
				cout << "running" << endl;
				
				//If background image is provided, we'll use it to subtract what we can from positive selection rect
				// if(!bg.empty()){
					
					// cout << "getting bg roi" << endl;
					// Mat bg_roi = bg(Rect(vals.at(start), vals[start+1], vals[start+2], vals[start+3]));
					
					// Mat mask;
					// Mat crop_comp[3];
					// Mat bg_comp[3];
					// split(crop, crop_comp);
					// split(bg_roi, bg_comp);
					
					// cout << "split" << endl;
					// static const char *ch[] = {"0", "1", "2"};
					// for(int c=0;c<3;c++){
						
						// subtract(crop_comp[c], bg_comp[c], mask, noArray(), CV_8UC1);
						// imwrite(string("diff_CH").append(ch[c]).append(".jpg"), mask);
						// threshold(mask, mask, 10, 255, THRESH_BINARY);
						// imwrite(string("bin_CH").append(ch[c]).append(".jpg"), mask);
						// subtract(crop_comp[c], bg_comp[c], crop_comp[c], mask, CV_8UC1);
						// imwrite(string("crop_CH").append(ch[c]).append(".jpg"), crop_comp[c]);
						
						// cout << "comp complete" << endl;
					// }
					
					// merge(crop_comp, 3, crop);
					
					// imshow("crop", crop);
					// waitKey();
					// return 0;
				// }
				
				
				
				
				
				
				Mat sized;
				resize(crop, sized, Size(OUTPUT_SIZE));
				
				//Save the image to pos folder
				char name[64];
				snprintf(name, sizeof(name), "%s/pos/%04i.%s", exPath.c_str(), posIdx++, ext.c_str());
				
				cout << "Saving " << imgFilename << " as positive image (" << name << ")" << endl;
				imwrite(name, sized);
				
				
				start += 4;
			}
			
		}
		

	}


	return(0);
	
}