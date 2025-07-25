#include <opencv2/core/types.hpp>
#include <time.h>
#include <string>
#include <iostream>
using namespace cv;
using namespace std;

#define MAX_MOTION_STEP_DISTANCE		20.0	//Maximum inches moved between detections
#define FIRE_SOLN_MIN_INST				10
#define FIRE_SOLN_TIME_SPAN				4000	//milliseconds
#define FIRE_SOLN_POINTS_EVAL			5		//Use a maximum of this number of previous points to extrapolate position
	

typedef struct {
	Point2f	loc;
	float quality;
	uint64_t time;				//Stores current time in ms
}inst_t;

class Object {
private:
	vector<inst_t> hist;		//x, y, quality (0-1). Distances in inches from NW deck corner
	int minSolnCount;

	//Get the current time in ms
	uint64_t curTime(){
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ((uint64_t)ts.tv_sec * 1000ull) + ts.tv_nsec/1000000;
	}
	
	//Calculate the distance between two points
	double distance(Point2f p1, Point2f p2){
		return sqrt(pow(p1.x-p2.x, 2) + pow(p1.y-p2.y, 2));
	}

public:
	Object(int minSolnCount_setting = FIRE_SOLN_MIN_INST){
		minSolnCount = minSolnCount_setting;
	}
	
	~Object(){ }
	
	//------------------------------------------------------------------------
	//Allow the caller to check if the detected location is likely this object
	bool checkLoc(Point2f loc){
		if(!hist.size())
			return false;
		
		Point2f lastLoc = hist[hist.size()-1].loc;
		return distance(lastLoc, loc) < MAX_MOTION_STEP_DISTANCE;
	}
	
	//------------------------------------------------------------------------
	//Add a new detection instance to our object history
	void newDetect(Point2f loc, uint64_t time = 0, float quality = 1.0){
		
		if(!time)
			time = curTime();
		
		inst_t i;
		i.loc = loc;
		i.quality = quality;
		i.time = time;
		hist.push_back(i);
	}
	
	//------------------------------------------------------------------------
	//Check if this object should be shot at
	bool fireSolution(){
		
		//Check if we have the minimum number of instances required (ignoring the timestamps for now)
		if(hist.size() < minSolnCount)
			return false;
	
	
		//Check if the full detection span of time is less than the required motion time before we trigger
		if(hist[hist.size()-1].time - hist[0].time < FIRE_SOLN_TIME_SPAN)
			return false;
		
		
		//Now we'll run through the list starting with the most recent entries, and we'll count the number of entries found
		//	before we run over the time we need the most recent MIN_INST entries to be within. If time gets too large, we'll exit (false)
		//	If we exit the while (likely due to enough instances found (c >= minSolnCount), we can return true. 
		//	We know we won't run out of loops (i) because we know we have enough entries due to the above check.
		int i = hist.size()-1;
		uint64_t t = hist[i--].time;
		int c = 1;
		while(i>=0 && c < minSolnCount){
			if(t - hist[i--].time > FIRE_SOLN_TIME_SPAN){
				cout << "Only " << c << "entries, no soln" << endl;
				return false;
			}
			c++;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------
	//Called after fireSolution has found a valid solution, but we'll return a shot location whether or not that has passed
	//This uses a linear approximation from the last (up to) 5 samples to estimate where the target will be at the provided time
	//(which will be added to the current time)
	Point2f getShotLoc(uint64_t time = 0){
		if(!hist.size())
			return Point2f(-1,-1);
		
		if(hist.size() == 1)
			return hist[0].loc;
		
		//use current time if time wasn't provided
		if(!time)
			time = curTime();
		
		//Times before the first entry are not valid (since we're using baseTime)
		if(time < hist[0].time)
			return Point2f(-1,-1);


		//We'll reduce the size of time values by subtracting out the first timestamp value from all the times. 
		uint64_t baseTime = hist[0].time;
		time -= baseTime;
		
		//Motion estimation range... the number of most recent instances we'll use to determine the shot location
		int c = FIRE_SOLN_POINTS_EVAL;
		if(c > hist.size())
			c = hist.size();
		
//		cout << "getShotLoc: calculating x,y,t bars: size()=" << hist.size() << ", c=" << c << endl;
		
		//Calculate x_bar, y_bar and t_bar (averages to use in the line fit below)
		//Keep in mind, we're calculating line fits for x(t) and y(t), so we need all three bars.
		float xb = 0.0;
		float yb = 0.0;
		float tb = 0.0;
		for(int i=hist.size()-1; i>=(int)(hist.size()-c); i--){
			xb += hist[i].loc.x;
			yb += hist[i].loc.y;
			tb += hist[i].time - baseTime;
		}
		xb /= c;
		yb /= c;
		tb /= c;
		
//		cout << "getShotLoc: calculating mx,my" << endl;
		
		//USING TIME AS LINE FIT EQUATION INDEPENDENT VARIABLE, we'll calculate two slopes, one for Y direction, and one for X direction
		float mx = 0.0;	//numerator sum
		float my = 0.0;	//numerator sum
		
		float dx = 0.0;	//partial denominator
		float dy = 0.0;	//partial denominator
		for(int i=hist.size()-c; i<hist.size(); i++){
			Point2f loc = hist[i].loc;
			uint64_t t = hist[i].time - baseTime;
			float x_diff = loc.x - xb;
			float y_diff = loc.y - yb;
			float t_diff = t - tb;
			
			mx += t_diff * x_diff;
			dx += t_diff * t_diff;
			
			my += t_diff * y_diff;
			dy += t_diff * t_diff;
		}
		
		mx /= dx;
		my /= dy;
		
		
		
//		cout << "getShotLoc: calculating bx,by" << endl;
		//Now we need to intercepts
		float bx = xb - mx*tb;
		float by = yb - my*tb;
		
//		cout << "xeq= " << mx << "t + " << bx << endl;
//		cout << "yeq= " << my << "t + " << by << endl;
		
//		cout << "Outputting extrapolated shot value" << endl;
		Point2f shotLoc(mx * time + bx, my * time + by);
		return shotLoc;
	}
	
	//------------------------------------------------------------------------
	//Reset object (clear all history)
	bool flush(uint64_t age = 0){
		if(hist.size() && hist[hist.size()-1].time <= curTime() - age)
			hist.clear();
		
		//Return true if flushed (or already was)
		return hist.size() ? false : true;
	}
	
	//------------------------------------------------------------------------
	//Check if object is active (has a current location history)
	bool active(){
		return hist.size() ? true : false;
	}
	
	//------------------------------------------------------------------------
	//Test all the critical functions in the class (distructive)
	int test(){		
		hist.clear();
		
		Point2f p1(10,10);
		Point2f p2(20,10);
		Point2f p3(20,20);
		Point2f p4((float)(p3.x+MAX_MOTION_STEP_DISTANCE),(float)(p3.y+MAX_MOTION_STEP_DISTANCE));
		
		//While history is empty
		if(checkLoc(p1)){ cout << "checkLoc() did not return false when history is empty" << endl; return -1;}
		if(fireSolution()){ cout << "fireSolution() did not return false when history is empty" << endl; return -1; }
		if(getShotLoc() != Point2f(-1,-1)){ cout << "getShotLoc() did not return -1,-1 when history is empty" << endl; return -1;}
		
		uint64_t t1 = 200000;
		newDetect(p1, t1);
		if(fireSolution()){ cout << "fireSolution() did not return false when history is < " << minSolnCount << endl; return -1;}
		if(getShotLoc() != p1){cout << "getShotLoc() did not return only point loaded" << endl; return -1;}
		if(!checkLoc(p2)){ cout << "checkLoc() did not return true when valid point was checked" << endl; return -1;}
		if(checkLoc(p4)){ cout << "checkLoc() returned true when new point was too far away" << endl; return -1;}
		
		uint64_t t2 = t1+1000;
		newDetect(p2, t2);
		if(fireSolution()){ cout << "fireSolution() did not return false when history is < " << minSolnCount << endl; return -1;}
		Point2f p = getShotLoc(t2+1000);
		if(p != Point2f(30,10)){cout << "getShotLoc() returned incorrect results with two entries (10,10)@200000, (20,10)@201000. Should be (30,10)@202000, but result was: " << p.x << "," << p.y << endl; return -1;}
		
		hist.clear();
		newDetect(p1,t1);
		newDetect(p3,t2);
		p = getShotLoc(t2+1000);
		if(p != Point2f(30,30)){cout << "getShotLoc() returned incorrect results with two entries (10,10)@200000, (20,20)@201000. Should be (30,30)@202000, but result was: " << p.x << "," << p.y << endl; return -1;}
		
		hist.clear();
		for(int i=0;i<minSolnCount+1;i++)
			newDetect(p1,t1+i*80);
		if(!fireSolution()){cout << "fireSolution() returned false when there are enough history elements to call valid" << endl; return -1;}
		p = getShotLoc(t1+minSolnCount*80);
		if(p != p1){ cout << "getShotLoc() did not return the point value that was stored in all positions (10,10). returned instead: " << p.x << "," << p.y << endl; return -1;}
		
		return 0;
	}
	
	
};



