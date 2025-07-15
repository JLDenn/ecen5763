#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctime>
#include <string>
#include <iostream>
using namespace std;

#define INVALID_HANDLE 		-1
#define DEFAULT_RATE		1000		//100.0 deg/sec


class PanTilt {
private:
	int serial;
	std::string rxBuf;
	bool blocking;
	
	//-----------------------------------------------------
	//	Write command to the serial port. 
	//	This function will append a '\r' character, so the command should not include it.
	bool sendCmd(const char *cmd){
		if(serial == INVALID_HANDLE){
			cout << "Couldn't send command: port not open" << endl;
			return false;
		}
		
		int len = strlen(cmd);
		int written = 0;
		while(written != len)
			written += write(serial, cmd, len - written);

		while(!write(serial, "\r", 1))
			;
		
		return true;
	}
	
	//-----------------------------------------------------
	// Read any bytes in the read buffer into the local rxBuf vector
	// this function simply reads whatever is found into the local buffer, but will stop reading at '\r' if found
	bool readResp(){
		if(serial == INVALID_HANDLE)
			return false;
		
		char c;
		int r = read(serial, &c, 1);
		while(r > 0){
			rxBuf += c;
			if(c == '\r')
				return true;
			r = read(serial, &c, 1);
		}
		
		if(r < 0){
			if(errno == EAGAIN || EWOULDBLOCK)
				return true;
			return false;
		}
		
		return true;
	}
	

public:
	//-----------------------------------------------------
	PanTilt(){
		serial = INVALID_HANDLE;
		blocking = false; 
	}
	
	//-----------------------------------------------------
	~PanTilt(){
		closePort();
	}
	
	//-----------------------------------------------------
	bool openPort(const char *port = "/dev/ttyACM0"){
		serial = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
		if(serial < 0){
			cout << "Error opening serial port (" << port << "): " << errno << endl;
			return false;
		}
		return true;
	}
	
	//-----------------------------------------------------
	bool closePort(){
		if(serial != INVALID_HANDLE){
			close(serial);
			serial = INVALID_HANDLE;
			return true;
		}
		return false;
	}
		
	//-----------------------------------------------------
	bool home(){
		return sendCmd("h");
	}
	
	//-----------------------------------------------------
	bool moveTo(int pan, int tilt, int rate = DEFAULT_RATE){
		char cmd[64];
		snprintf(cmd, sizeof(cmd), "x%dy%dr%d", pan, tilt, rate);
		return sendCmd(cmd);
	}
	
	//-----------------------------------------------------
	bool move(int pan, int tilt = 0, int rate = DEFAULT_RATE){
		char cmd[64];
		snprintf(cmd, sizeof(cmd), "dx%dy%dr%d", pan, tilt, rate);
		return sendCmd(cmd);
	}
	
	//-----------------------------------------------------
	bool active(bool on, int time = 0){
		char cmd[32];
		snprintf(cmd, sizeof(cmd), "a%dt%d", on ? 1 : 0, time);
		return sendCmd(cmd);
	}
	
	//-----------------------------------------------------
	bool getPos(int *pan, int *tilt, int *periph = NULL){
		if(!sendCmd("g"))
			return false;
		
		rxBuf.clear();
		//Wait for response
		clock_t clk = clock() + CLOCKS_PER_SEC;
		while(clock() < clk && (!rxBuf.size() || rxBuf.at(rxBuf.size()-1) != '\r'))
			readResp();
		
		if(rxBuf.size() && rxBuf.at(rxBuf.size()-1) == '\r'){
			int p;
			sscanf(rxBuf.c_str(), "%d,%d,%d", pan, tilt, &p);
			if(periph)
				*periph = p;
			return true;
		}
		
		cout << "Full response not received: " << rxBuf << endl;
		return false;
	}
	
	//-----------------------------------------------------
	//	Perform a full test of the basic functions of the pan & tilt
	//	Returns 0 on success, 1 on failure
	int test(){
	
		if(!openPort()){
			cout << "Error opening default serial port" << endl;
			return 1;
		}
		
		int pan, tilt, periph;
		if(!getPos(&pan, &tilt, &periph)){
			cout << "Error getting position information" << endl;
			return 1;
		}

		cout << "Current position " << pan << ", " << tilt << ", " << (periph ? "ON" : "OFF") << endl;

		if(!home()){
			cout << "Error homing" << endl;
			return 1;
		}
		
		cout << "Waiting 10s..." << endl;
		usleep(10000000);
		
		cout << "Moving to 0, 100" << endl;
		if(!moveTo(0, 100)){
			cout << "Error moving to 0,100" << endl;
			return 1;
		}
		
		cout << "Waiting 4s..." << endl;
		usleep(4000000);
		
		cout << "Moving 100, -200" << endl;
		if(!move(100, -200)){
			cout << "Error performing delta move" << endl;
			return 1;
		}
		
		cout << "Waiting 2s..." << endl;
		usleep(2000000);
		
		cout << "Setting active" << endl;
		if(!active(1)){
			cout << "Error setting perminant active" << endl;
			return 1;
		}
		
		cout << "Waiting 2s..." << endl;
		usleep(2000000);
		
		cout << "Setting inactive" << endl;
		if(!active(0)){
			cout << "Error setting perminant inactive" << endl;
			return 1;
		}
		
		cout << "Waiting 2s..." << endl;
		usleep(2000000);

		cout << "Setting active for 2 seconds" << endl;
		if(!active(1, 2000)){
			cout << "Error setting 2s active" << endl;
			return 1;
		}
		
		cout << "Waiting 3s..." << endl;
		usleep(3000000);
		
		cout << "trying consecutive moves" << endl;
		if(!moveTo(200, 200) || !moveTo(-20, -50)){
			cout << "Error setting one of the two moveTo commands" << endl;
			return 1;
		}
		
		cout << "All tests complete" << endl;
		closePort();
		return 0;
	}

	
};