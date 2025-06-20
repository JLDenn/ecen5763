#ifndef TRACK_HPP
#define TRACK_HPP

#define ASSERT(z)	{if(!(z)) {cout << "assertion failed @ " << __FILE__ << ":" << __LINE__ << endl; exit(1);}}

typedef struct {
	int height;
	int width;
	uint8_t *buf;
}img_t;

typedef struct {
	int x;
	int y;
	int w;
	int h;
}obj_t;


#endif