#include "congestion.h"

using namespace std;

CongestionWindow::CongestionWindow(){
	windowSize = 1024;
	threshold = 30720;
	//congestionState = 0; // 0: slow start
}

void CongestionWindow::setWindowSize(int size){
	windowSize = size;
}

int CongestionWindow::getWindowSize(){
	return windowSize;
}

void CongestionWindow::setThreshold(int num){
	threshold = num;
}

int CongestionWindow::getThreshold(){
	return threshold;
}

void CongestionWindow::adjustParameter(int num){ // 0: ack	1: timeout	2: 3 dup acks
	// adjust windowSize and threshold 
	if (num == 0){ // ack
		if (windowSize < threshold) { windowSize = windowSize + 1024;} // slow start
		else { windowSize = windowSize + 1024 * 1024 / windowSize;}  // congestion avoidance
	}
	if (num == 1){ // timeout
		threshold = windowSize / 2;
		windowSize = 1024;	
	}
	if (num == 2){ // 3 dup acks
		windowSize = windowSize / 2;
		threshold = windowSize / 2;
	}

}



