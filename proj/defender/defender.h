
#include "pthread.h"

//Global variable that holds the "shoot here" location. When either location is negative, no shoot directive is given
typedef struct {
	Point2f targetLocation;		//App can update when engage is false (cleared in targeting thread)

	volatile bool engage;				//Assumed to be atomic
								//Targetting thread will clear engage when fire is complete

	
	volatile bool running;
}target_t;

