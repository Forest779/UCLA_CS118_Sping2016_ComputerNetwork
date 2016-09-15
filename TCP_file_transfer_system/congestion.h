#ifndef CONGESTIONWINDOW_H
#define CONGESTIONWINDOW_H

#include <string>
#include <string.h>
#include <thread>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

using namespace std;


class CongestionWindow{

private:
	int windowSize;
	int threshold;
	//int congestionState; // 0: slow start	1: congestion avoidance

public:
	CongestionWindow();

	void setWindowSize(int size);
	int getWindowSize();

	void setThreshold(int num);
	int getThreshold();

	void adjustParameter(int num); // 0: acks    1: timeout	    2: 3 dup acks
};
#endif