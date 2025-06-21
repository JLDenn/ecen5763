#ifndef TRACK_HPP
#define TRACK_HPP

//Define an assertion check we can use that will print the failure and exit the application
#define ASSERT(z)	{if(!(z)) {cout << "assertion failed @ " << __FILE__ << ":" << __LINE__ << endl; exit(1);}}

//Define an object detection structure to store each of the objects found
typedef struct {
	int x;
	int y;
	int w;
	int h;
}obj_t;

#endif