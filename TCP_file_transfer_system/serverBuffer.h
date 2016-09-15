#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "packet.cpp"

using namespace std;

class ServerBuffer {
private:
	char buffer[30720];
	bool packetBound[30720];
	int begin;
	int end;
	int lastSend;
	int cWindow;
	int ssthresh;
	int rWindow;
	int seqLimit;
	int maxWindow;
	ifstream inStream;

public:
	ServerBuffer(string filename, int rWindow);	

	void setBeginPosition(int seq);

	Packet** sendPacket(int &packetnum);

	Packet* retransmit();

	void changeCongestionWindow(int cWindow, int ssthresh);

	void receiveAck(int ack);

	int receiveDataFromApp();

	bool isBufferEmpty();

	void closeFile();
};