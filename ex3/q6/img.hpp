
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "track.hpp"
#include <vector>
 
#define MAX_LINE_LEN	128
#define MAX_IMG_DIMENSION	3000

using namespace std;

class Img {
	
private:
	int _width;
	int _height;
	uint8_t *data;


	//Private method to read in a line from a file until a \n is found, or we run out of space 
	char *readLine(int fdin, char *line){
	
		int len = 0;
		while(len < MAX_LINE_LEN){
			if(read(fdin, (void *)&line[len], 1) < 1){
				printf("Error reading header data\n");
				return NULL;
			}
			
			//If we found a \n, replace it with a NULL and return
			if(line[len] == '\n'){
				line[len] = '\0';
				return line;
			}
			
			//Advance to the next character
			len++;
		}
		
		return NULL;		//Too long
	}



	//Returns the next x pixel location to check (skips over this none-object), or 0 if object was found.
	int resolveObj(int x, int y, int thresh, int minSize, obj_t *obj){
		
		//We'll now crawl around the border of the object keeping track of min/max x/y values.
		static const int dir[][2] = { {0,1}, {1,1}, {1,0},  {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1} };
		
		int minx=x, miny=y;
		int maxx=x, maxy=y;
		int curx=x, cury=y;
		int stopx=x;
		
//		cout << "NEW OBJ" << " at " << x << "," << y << endl;
		
		int stuckEscape = 0;	//Make sure we don't have some wierd shape that will make us loop forever. 
		int lastDir = 0;		//Holds the modified last direction we moved (so we can start looking in almost the same direction)
		do{
			int d = lastDir;
			do{
				int newx = curx + dir[d][0];
				int newy = cury + dir[d][1];
				
//				cout << "trying dir " << dir[d][0] << "," << dir[d][1] << endl;
				
				//Check the next location's pixel values agains the threshold. getPixel checks for out of bounds
				//	x,y values and returns 0 if out of bounds (which will definitely be below the thresh, so we don't
				//	need to check bounds here. 
				if(getPixel(newx, newy) >= thresh){
					
					//We're now moving to the new pixel since we know it is >= thresh
					curx = newx;
					cury = newy;
					
					//Update the min/max values
					if(curx > maxx)
						maxx = curx;
					if(curx < minx)
						minx = curx;
					if(cury > maxy){
						maxy = cury;
						stopx = curx;	//We keep track of an X coordinate when we hit a new maxy so we can trigger a "search complete"
					}
					if(cury < miny)
						miny = cury;
					
					//We want to attempt to check the previous direction in the next locations (if we moved right, we'll first check down-right)
					//	as a starting point at this new position. However, if we moved down to get here, we'll just check down next.
					lastDir = d ? d-1 : d;
					
//					cout << "moving " << dir[d][0] << "," << dir[d][1] << " to " << newx << "," << newy << " | stopx=" << stopx << ", maxy=" << maxy << endl;
					
					//We moved, so we'll exit the direction search loop
					break;
				}
				
				d = (d+1) % 8;
			}while(d != lastDir);
			
		}while(stuckEscape++ < 5000 && (curx != stopx || cury != maxy));
		
		//Check our object width/height agains the minSize provided. We're measuring the size in number of pixels, not distance,
		//	so we'll add 1 to the h & w.
		int h = maxy-miny+1;
		int w = maxx-minx+1;
		if(h >= minSize && w >= minSize){
			//Object meets the criteria, so update the obj provided and return 0 (object found)
			obj->x = (maxx - minx)/2 + minx;
			obj->y = (maxy - miny)/2 + miny;
			obj->w = w;
			obj->h = w;
			
//			cout << "object found: w,h: " << obj->w << "," << obj->h << " @ x,y: " << obj->x << "," << obj->y << endl;
			return 0;
		}
		
		//Return the extents of the object so we'll skip over the rest of the object and not call resolveObj again in this row
		return maxx+1;
	}





public:
	////////////////////////////////////////////////////////////////////////////
	//Constructor, load provided file
	Img(string filename = ""){
		_width = _height = 0;
		data = NULL;
		
		if(!filename.size())
			return;
		else 
			load(filename);
	}
	
	////////////////////////////////////////////////////////////////////////////
	//Copy constructor 
	Img(const Img &src){
		_width = _height = 0;
		data = NULL;
		copy(src);
	}
	
	////////////////////////////////////////////////////////////////////////////
	//Destructor to ensure we don't have a memory leak
	~Img(){
		if(data)
			free(data);
	}
	
	////////////////////////////////////////////////////////////////////////////
	//	Load an image at the provided filename
	bool load(string filename){
		
		int temp;
		if(data)
			free(data);
		data = NULL;
		
		int fdin;
		if((fdin = open(filename.c_str(), O_RDONLY, 0644)) < 0){
			printf("Error opening %s\n", filename.c_str());
			return false;
		}
		
		//Read the first ascii line and verify it is a P5 file. 
		char buf[MAX_LINE_LEN];
		char *line = readLine(fdin, buf);
		if(!line){
			cout << "Error reading source file " << filename << endl;
			goto cleanup;
		}
		if(strcmp(buf, "P5")){
			cout << "Incorrect image file format. Expected pgm binary grayscale (P5) file" << endl;
			goto cleanup;
		}
		
		//Read and ignore the comment line
		line = readLine(fdin, buf);
		
		//Read the dimensions
		line = readLine(fdin,buf);
		if(!line){
			cout << "Error reading source file " << filename << endl;
			goto cleanup;
		}	
		if(sscanf(line, "%i %i", &_width, &_height) != 2){
			cout << "Error reading image width/height values" << endl;
			goto cleanup;
		}
		if(_width <= 0 || _width > MAX_IMG_DIMENSION || _height <= 0 or _height > MAX_IMG_DIMENSION){
			cout << "Invalid values for image width and height" << endl;
			goto cleanup;
		}
		
		//Read the depth
		line = readLine(fdin, buf);
		if(!line){
			cout << "Error reading source file " << filename << endl;
			goto cleanup;
		}	
		if(sscanf(line, "%i", &temp) != 1 || temp != 255){
			cout << "Error reading image depth value" << endl;
			goto cleanup;
		}
		
		//Allocate and read in the image data
		temp = _width * _height;
		data = (uint8_t*)malloc(temp);
		if(!data){
			cout << "Error allocating memory for the image data" << endl;
			goto cleanup;
		}
		if(read(fdin, data, temp) != temp){
			cout << "Error reading full image frame data" << endl;
			free(data);
			data = NULL;
			goto cleanup;
		}

		close(fdin);
		return true;

cleanup:
		//Close the file before returning
		close(fdin);
		return false;
	}
	
	//////////////////////////////////////////////////////////////////////
	//	Copy the provided img to this (data copy)
	void copy(const Img &img){
		if(data)
			free(data);
		
		_width = img._width;
		_height = img._height;
		data = (uint8_t*) malloc(_width * _height);
		if(data)
			memcpy(data, img.data, _width*_height);
	}
	
	//////////////////////////////////////////////////////////////////////
	//	Save this image to the provided filename
	bool save (string filename) const {
		if(!loaded()){
			cout << "Attemped to write with no image loaded" << endl;
			return false;
		}
		
		int len;
		int fdout = open(filename.c_str(), O_WRONLY | O_CREAT, 0644);
		if(fdout < 0){
			cout << "Error opening file for write: " << filename << endl;
			return false;
		}
		
		char hdr[] = "P5\n# Justin Denning custom image\n";
		if(write(fdout, hdr, sizeof(hdr)-1) != sizeof(hdr)-1){
			cout << "Error writing to file" << endl;
			goto cleanup;
		}
		
		len = snprintf(hdr, sizeof(hdr), "%i %i\n255\n", _width, _height);
		if(write(fdout, hdr, len) != len){
			cout << "Error writing to file" << endl;
			goto cleanup;
		}

		if(write(fdout, data, _width*_height) != _width*_height){
			cout << "Error writing to file" << endl;
			goto cleanup;
		}	
		
		close(fdout);
		return true;
		
cleanup:
		close(fdout);
		return false;
	}

	
	int width()const { return _width; }
	int height() const { return _height; }
	int size() const { return _width * _height; }
	bool loaded() const { return data ? true : false; }
	
	uint8_t getPixel(int x, int y) const { 
		if(x >= 0 && y >= 0 && x < _width && y < _height)
			return data[y * _width + x];
		return 0;
	}
	void setPixel(int x, int y, uint8_t val){
		if(x >= 0 && y >= 0 && x < _width && y < _height)
			data[y * _width + x] = val;
	}
	
	
	////////////////////////////////////////////////////////////////////////
	//	allocate memory to fit w and h, and fill with zeros.
	bool zero(int w, int h){
		if(data)
			free(data);
		
		data = (uint8_t*)malloc(w*h);
		if(!data)
			return false;
		_width = w;
		_height = h;
		
		for(int i=0;i<size();i++)
			data[i] = 0;
	}
	
	
	//////////////////////////////////////////////////////////////////////////
	//get histogram
	int histogram(uint8_t *levels){
		if(!loaded())
			return -1;
		
		for(int i=0;i<256;i++)
			levels[i] = 0;
		
		for(int i=0;i<size();i++)
			levels[data[i]]++;
		
		return 0;
	}
	
	
	/////////////////////////////////////////////////////////////////////////
	//Threshold
	//	Values below min will be 0, 
	//	Values above max will be 255
	//	Values between will be unchanged
	//	min can = max
	int threshold(uint8_t min, uint8_t max){
		if(!loaded())
			return -1;
		
		for(int i=0;i<size();i++)
			data[i] = data[i] < min ? 0 : (data[i] > max ? 255 : data[i]);
		return 0;
	}
	
	///////////////////////////////////////////////////////////////////////////
	//	Convolve 3x3 matrix with image data
	int convolve(Img &out, int *mat){
		if(!loaded()){
			cout << "No image loaded" << endl;
			return -1;
		}
		
		out.zero(_width, _height);
		
		for(int i=1;i<_width-1;i++){
			for(int j=1;j<_height-1;j++){
				int v;
				v  = getPixel(i-1,j-1)*mat[0];
				v += getPixel(i,  j-1)*mat[1];
				v += getPixel(i+1,j-1)*mat[2];
				v += getPixel(i-1,j  )*mat[3];
				v += getPixel(i,  j  )*mat[4];
				v += getPixel(i+1,j  )*mat[5];
				v += getPixel(i-1,j+1)*mat[6];
				v += getPixel(i,  j+1)*mat[7];
				v += getPixel(i+1,j+1)*mat[8];
				v /= 128;
				if(v > 255) v = 255;
				if(v < 0) v = 0;
				out.setPixel(i,j,v);
			}
		}
		
		return 0;
	}
	
	///////////////////////////////////////////////////////////////////////////
	//	Multiply each pixel of the provided Img to this Img.
	int mul(const Img &in){
		if(!loaded() || !in.loaded()){
			cout << "No image loaded" << endl;
			return -1;
		}
		
		if(in.width() != _width || in.height() != _height){
			cout << "Image dimensions don't match" << endl;
			return -1;
		}
		
		for(int i=0;i<size();i++)
			data[i] = data[i] * in.data[i] / 255;
		
		return 0;
	}
	
	/////////////////////////////////////////////////////////////////////////////
	//	Calculate the difference between this Img and in  (this - in).
	int diff(const Img &in){
		if(!loaded() || !in.loaded()){
			cout << "No image loaded" << endl;
			return -1;
		}
		
		if(in.width() != _width || in.height() != _height){
			cout << "Image dimensions don't match" << endl;
			return -1;
		}		
		
		for(int i=0;i<size();i++){
			int d = data[i] - in.data[i];
			data[i] = d < 0 ? 0 : d;
		}
		return 0;
	}
	
	
	

	
	//////////////////////////////////////////////////////////////////////
	// Find objects that are brighter than thresh, and at least minSize in x and y
	int objects(vector<obj_t> *vObj, int thresh, int minSize){
		
		for(int j=0;j<_height;j++){
			for(int i=0;i<_width;i++){
				
				if(getPixel(i,j) >= thresh){
					obj_t obj;
					
					//Attempt to resolve this object by edge crawling to find size. 
					//	We'll give it a lower threshold to "keep" lower intensity levels as edges
					//	that are lighter than the initial detection level. We'll stay in the object
					//	when the intensity level is >= 2/3 * detection thresh
					int res = resolveObj(i,j, thresh * 2 / 3, minSize, &obj);
					if(!res){
						
						//Since we're scanning through the entire image, we will likely stumble on the same object
						//	multiple times. So, before we add this object into the vector, we'll check to see if this
						//	object is already in there. We'll only add it if it is a fresh object. 
						bool fresh = true;
						for(int idx=0;idx<vObj->size();idx++){
							if(vObj->at(idx).x == obj.x && vObj->at(idx).y == obj.y){
								fresh = false;
								break;
							}
						}
						
						//If this is a previously un-found object, add it to the vector.
						if(fresh)
							vObj->push_back(obj);

					}
					else
						i = res;	//Not an object, so we can skip over any remaining partial object pixels that may trigger us
				}
				
			}
		}
		
		//return the number of unique objects we found in this image. 
		return vObj->size();
	}
	
	////////////////////////////////////////////////////////////////////////
	//	Apply a cross/box mark at the location of the provided obj.
	int mark(obj_t *obj, int size = 20){
		for(int i=-size;i<size;i++){
			setPixel(obj->x+i, obj->y, 255);
			setPixel(obj->x, obj->y+i, 255);
		}
		
		//Place 8x time box around the object
		int boxHalfSize = obj->w*4;			//4 is each direction from x,y. So 4x = a box that is 8 times the size of the object
		for(int i=-boxHalfSize;i<boxHalfSize;i++){
			setPixel(obj->x+i,obj->y+boxHalfSize, 255);
			setPixel(obj->x+i,obj->y-boxHalfSize, 255);
			setPixel(obj->x+boxHalfSize,obj->y+i, 255);
			setPixel(obj->x-boxHalfSize,obj->y+i, 255);
		}
	}
	
	
};