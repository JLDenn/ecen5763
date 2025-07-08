/**
	This code uses an output file from opencv_annotation to sort a collection of images into pos and neg folders. 
	When the object count is 0, the image is put in neg. for each object boxed, this code crops and resizes the selection to 64x128, and saves
	the result to pos directory. 
	
**/


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <dirent.h>
#include <fstream>

#include <iostream>
#include <time.h>

using namespace cv;
using namespace std;

int main(int argc, char **argv){
	
	const char* keys ={
        "{help h|      | show help message}"
        "{@a	|<none>| the annotation text file with filenames and box coordinates}"
		"{r     | .    | relative path to prepend to paths in annoation file}"
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
		return 1;
	}

	//Get the executable path 
	string exPath = ".";
	size_t p = string(argv[0]).rfind("/");
	if(p != string::npos)
		exPath = string(argv[0]).substr(0, p);

	
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
			while(start + 4 <= vals.size()){
				
				Mat crop = raw(Rect(vals.at(start), vals[start+1], vals[start+2], vals[start+3]));
				Mat sized;
				resize(crop, sized, Size(64,128));
				
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