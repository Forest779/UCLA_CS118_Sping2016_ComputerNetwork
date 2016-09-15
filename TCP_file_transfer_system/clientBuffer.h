#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "packet.cpp"

using namespace std;

class ClientBuffer {
private:
	char buffer[30720];
	bool effectiveData[30720];
	int begin;
	int end;
	ofstream outStream;

public:
	ClientBuffer(string filename);	

	void setBeginPosition(int seq);

	Packet* receivePacketAndSendAck(Packet* packet);

	void sendDataToApp();

	void closeFile();
};