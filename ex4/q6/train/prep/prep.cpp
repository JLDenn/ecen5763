/**
	This code takes the dataset downloaded from https://www.kaggle.com/datasets/hanxunyu828/inriaperson and 
	uses the annotation files to crop and save the actual people from the images. 
	
	Usage: ./train -o=croppedImages -i=INRIAPerson/Train/annotations -r=INRIAPerson
	
	-o - is the output folder to store the output cropped images to
	-i - is the directory where the annotation files are located (annotation file example provieded below
	-r - optional relative path to prepend (without trailing /) to the path located in the annotation file
	
	
	The two (types of) lines used are the lines starting with:
		Image filename : 
		Bounding box for object #### "PASperson" (Xmin, Ymin) - (Xmax, Ymax) : 
	
	********************* ANNOTATION FILE EXAMPLE **********************
# PASCAL Annotation Version 1.00

Image filename : "Train/pos/crop001011.png"
Image size (X x Y x C) : 1124 x 826 x 3
Database : "The INRIA Rhône-Alpes Annotated Person Database"
Objects with ground truth : 2 { "PASperson" "PASperson" }

# Note that there might be other objects in the image
# for which ground truth data has not been provided.

# Top left pixel co-ordinates : (0, 0)

# Details for object 1 ("PASperson")
# Center point -- not available in other PASCAL databases -- refers
# to person head center
Original label for object 1 "PASperson" : "UprightPerson"
Center point on object 1 "PASperson" (X, Y) : (344, 235)
Bounding box for object 1 "PASperson" (Xmin, Ymin) - (Xmax, Ymax) : (164, 176) - (433, 784)

# Details for object 2 ("PASperson")
# Center point -- not available in other PASCAL databases -- refers
# to person head center
Original label for object 2 "PASperson" : "UprightPerson"
Center point on object 2 "PASperson" (X, Y) : (642, 229)
Bounding box for object 2 "PASperson" (Xmin, Ymin) - (Xmax, Ymax) : (530, 167) - (760, 773)
	
	********************* ANNOTATION FILE EXAMPLE **********************
	
	
**/


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <dirent.h>
#include <fstream>

#include <iostream>
#include <time.h>

using namespace cv;
using namespace std;

#define BORDER			20

int main(int argc, char **argv){
	
	const char* keys ={
        "{help h|     | show help message}"
        "{o outf|     | output folder for cropped images}"
		"{i imgf|     | input folder for the annoation files}"
		"{r rel |     | relative path adjustment for the annotation file paths}"
    };
	
    CommandLineParser parser( argc, argv, keys );
    if ( parser.has( "help" ) ){
        parser.printMessage();
		cout << "Example:" << endl;
		cout << argv[0] << " -o=croppedImages -i=INRIAPerson/Train/annotations -r=INRIAPerson" << endl;
        exit( 0 );
    }
	
	String outDir = parser.get< String >( "o" );
	cout << "Output directory: " << outDir << endl;
	String imgAnnDir = parser.get< String >("i");
	cout << "Image annotation directory: " << imgAnnDir << endl;
	String relPath = parser.get< String >("r");
	cout << "Relative path pre-pended to annotation paths: " << relPath << endl;
	
	if(!outDir.size()){
		cout << "Output directory required" << endl;
		return 1;
	}
	
	int frameIdx = 1;
	struct dirent *en;
	DIR *dr = opendir(imgAnnDir.c_str()); //open all or present directory
	if (dr) {
		while ((en = readdir(dr)) != NULL) {
			//cout << en->d_name << endl; //print all directory name
			String filename = imgAnnDir;
			filename.append("/").append(en->d_name);
			
			ifstream file(filename);
			if(!file.is_open()){
				cout << "Can't open " << filename << endl;
				continue;
			}

			string imgFilename = "";
			vector<Vec4i> bounds;

			string line;
			while(getline(file, line)){
				size_t found = line.find("Image filename : ");
				if(found != string::npos)
					imgFilename = line.substr(18, line.size()-19);
				
				found = line.find("Bounding box for object ");
				if(found != string::npos){					
					found = line.find(": ");
					if(found != string::npos){
						Vec4i v;
						sscanf(line.substr(found+2).c_str(), "(%d, %d) - (%d, %d)", &v[0], &v[1], &v[2], &v[3]);
						bounds.push_back(v);
					}
				}
			}
			
			
			file.close();		 

			if(imgFilename.size()){
				if(relPath.size()){
					string p = relPath;
					p.append("/").append(imgFilename);
					imgFilename = p;
				}
				
				cout << "reading image: " << imgFilename << endl;
				
				//Read and crop the image
				Mat raw = imread(imgFilename);
				if(!raw.empty()){
					
					for(int i=0;i<bounds.size();i++){
						Vec4i v = bounds.at(i);
						v[0] -= BORDER;
						if(v[0] < 0) v[0] = 0;
						v[1] -= BORDER;
						if(v[1] < 0) v[1] = 0;
						
						Size s = raw.size();
						v[2] += BORDER;
						if(v[2] >= s.width) v[2] = s.width-1;
						v[3] += BORDER;
						if(v[3] >= s.height) v[3] = s.height-1;
						
						Mat crop = raw(Rect(v[0], v[1], v[2]-v[0], v[3]-v[1]));
						Mat sized;
						resize(crop, sized, Size(64,128));
						
						string outFile = outDir;
						outFile.append("/").append(to_string(frameIdx++)).append(imgFilename.substr(imgFilename.size()-4));
						
						//cout << "Writing cropped file: " << outFile << endl;
						imwrite(outFile, sized);
					}
				}
			}
		

		}
		closedir(dr); //close all directory
	}
	else
		cout << "Error opening directory" << endl;
	return(0);

	
	
	
	
	
}