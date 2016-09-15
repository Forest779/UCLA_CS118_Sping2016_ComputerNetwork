#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "serverBuffer.h"

using namespace std;

ServerBuffer::ServerBuffer(string filename, int rWindow) {
	memset(buffer, '\0', sizeof(buffer));
	for(int i=0; i<(int)sizeof(packetBound); i++) {
		packetBound[i] = false;
	}
	begin = 0;
	end = 0;
	lastSend = 0;
	cWindow = 1024;
	ssthresh = 30720;
	this->rWindow = rWindow;
	seqLimit = 15000;
	maxWindow = min(min(this->rWindow, seqLimit), cWindow);
	inStream.open(filename);
	if(inStream.fail()) {
		cout << "Cannot find the file!" << endl;
		exit(0);
	}
	cout << "File open!" << endl;
}

void ServerBuffer::setBeginPosition(int seq) {
	begin = seq;
	end = seq;
	lastSend = seq;
}

void ServerBuffer::receiveAck(int ack) {
    cout << "Receiving new ACK packet " << ack << endl;

	if(begin < ack) {
		//cout << "begin < ack" << endl;
		for(int i=begin; i<ack; i++) {
			buffer[i] = '\0';
			packetBound[i] = false;
			packetBound[i+1] = false;
		}
		begin = ack;
	}

	if(begin > ack) {
		//cout << "begin > ack" << endl;
		for(int i=begin; i<(int)sizeof(buffer); i++) {
			buffer[i] = '\0';
			packetBound[i] = false;
		}
		for(int i=0; i<ack; i++) {
			buffer[i] = '\0';
			packetBound[i] = false;
			packetBound[i+1] = false;
		}
		begin = ack;
	}

	//cout << "end is: " << end << endl;
	//cout << "begin is: " << begin << endl;

	// if(packetBound[begin] == false) {
	// 	cout << "All data acked" << endl;
	// } else {
	// 	cout << "Some data still wait to be acked" << endl;
	// }
}

Packet** ServerBuffer::sendPacket(int& packetnumToOutside) {
	//cout << "Enter sendPacket function" << endl;

	int dataLength = 0;
	int packetnum = 0;
	int j = 0;
	int seq = lastSend;

	char tmpBuffer[30720];
	memset(tmpBuffer, '\0', sizeof(tmpBuffer));

	//cout << "end is " << end << endl;


	if(lastSend == end) {
		return NULL;
	}

	if(lastSend < end) {
		dataLength = end-lastSend;
		if ((dataLength%1024) != 0) {
			packetnum = (dataLength/1024)+1;
		} else {
			packetnum = (dataLength/1024);
		}
		j=0;
		for(int i=lastSend; i<end; i++) {
			tmpBuffer[j] = buffer[i];
			j++;
		}
		//lastSend = end;
	}

	if(lastSend > end) {
		dataLength = sizeof(buffer)-lastSend+end;
		if ((dataLength%1024) != 0) {
			packetnum = (dataLength/1024)+1;
		} else {
			packetnum = (dataLength/1024);
		}
		
		j=0;
		for(int i=lastSend; i<(int)sizeof(buffer); i++) {
			tmpBuffer[j] = buffer[i];
			j++;
		}
		for(int i=0; i<end; i++) {
			tmpBuffer[j] = buffer[i];
			j++;
		}
		//lastSend = end;
	}
	//cout << "Already calculate dataLength and packetnum" << endl;

	for (int i=0; i<packetnum-1; i++) {
		lastSend = (lastSend+1024) % (int)sizeof(buffer);
		packetBound[lastSend] = true;
		//cout << "Add ture to packetBound " << lastSend << endl;
	}
	//cout << "lastSend1 " << lastSend <<endl;

	if ((dataLength%1024) == 0) {
		lastSend = (lastSend+1024) % (int)sizeof(buffer);
	}else {
		lastSend = (lastSend+(dataLength%1024)) % (int)sizeof(buffer);
	}

	if(lastSend != end) {
		cout << "lastSend != end, something goes wrong, exit!" << endl;
		exit(0);
	}

	packetBound[lastSend] = true;
	//cout << "Add ture to packetBound " << lastSend << endl;

	Packet** output = new Packet*[packetnum];

	for(int i=0; i<packetnum; i++) {
		char payload[1024];
		memset(payload, '\0', sizeof(payload));
		for(int k=0; k<1024; k++) {
			payload[k] = tmpBuffer[i*1024+k];
		}
		output[i] = new Packet();
		output[i]->set_payload(payload);
		output[i]->set_seq(uint16_t(seq));

		cout << "Sending data packet " << seq << " " <<  cWindow <<" " << ssthresh << endl;
		//cout << "payload length is " << output[i]->payload_length() << endl;
		cout << "Effective window size is " << maxWindow << endl;

		seq = (seq+1024) % (int)sizeof(buffer);
	}
	
    //cout << "lastSend is " << lastSend << endl;
	packetnumToOutside = packetnum;
	//cout << "The size of output is " << packetnumToOutside << endl;
	return output;
}

void ServerBuffer:: changeCongestionWindow(int cWindow, int ssthresh) {
	this->cWindow = cWindow;
	this->ssthresh = ssthresh;
}

Packet* ServerBuffer::retransmit() {
	
	char payload[1024];
	memset(payload, '\0', sizeof(payload));

	int i = begin;
	int j = 0;

	while(packetBound[i] == false) {
		payload[j] = buffer[i];
		j++;
		i = (i+1) % (int)sizeof(buffer);
		if (j>1024) {
			cout << "Error: find a packet's payload bigger than 1024 bytes" << endl;
			exit(0);
		}
	}

	Packet* resendPacket = new Packet();
	resendPacket->set_payload(payload);
	resendPacket->set_seq(uint16_t(begin));
	//cout << "resend payload length is " << resendPacket->payload_length() << endl;
   
    //cout << "size of payload is: " << (int)sizeof(payload) << endl;

	cout << "Sending data packet " << begin << " " <<  cWindow <<" " << ssthresh << " Retransmission" << endl;

	return resendPacket;
}

int ServerBuffer::receiveDataFromApp() {
	maxWindow = min(min(rWindow, seqLimit), cWindow);
	int count = 0;

	//if all the data in the file have been read, return fileEndFlag = 1
	//if some data still remain in the file, return fileEndFlag = 0
	int fileEndFlag = 0;

	//when begin and end are in the same position
	if(begin == end) {
		if((begin+maxWindow) <= (int)sizeof(buffer)) {
			for(int i=end; i<(begin+maxWindow); i++) {
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					//cout << "The last letter is: " << tmp <<endl;
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			end = (end+count) % (int)sizeof(buffer);
			cout << "Receive " << count <<" bytes data form app" << endl;
		}

		if((begin+maxWindow) > (int)sizeof(buffer)) {
			for(int i=end; i<(int)sizeof(buffer); i++) {
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			for(int i=0; i<((begin+maxWindow)%(int)sizeof(buffer)); i++) {
				if(fileEndFlag ==1) break;
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			end = (end+count) % (int)sizeof(buffer);
			cout << "Receive " << count <<" bytes data form app" << endl;
		}
	} 



    //when begin is before end
	else if(begin < end) {
		if((begin+maxWindow) <= (int)sizeof(buffer)) {
			for(int i=end; i<(begin+maxWindow); i++) {
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			end = (end+count) % (int)sizeof(buffer);
			cout << "Receive " << count <<" bytes data form app" << endl;
		}

		if((begin+maxWindow) > (int)sizeof(buffer)) {
			for(int i=end; i<(int)sizeof(buffer); i++) {
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			for(int i=0; i<((begin+maxWindow)%(int)sizeof(buffer)); i++) {
				if(fileEndFlag == 1) break;
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			end = (end+count) % (int)sizeof(buffer);
			cout << "Receive " << count <<" bytes data form app" << endl;
		}

	}

    //when begin is after end
	else if(begin > end) {
		if((begin+maxWindow) > (int)sizeof(buffer)) {
			for(int i=end; i<((begin+maxWindow) % (int)sizeof(buffer)); i++) {
				char tmp;
				inStream >> noskipws >> tmp;
				buffer[i] = tmp;
				count++;
				if(inStream.eof()) {
					cout << "All the data in the file have been read into the buffer" << endl;
					fileEndFlag = 1;
					break;
				}
			}
			end = (end+count) % (int)sizeof(buffer);
			cout << "Receive " << count <<" bytes data form app" << endl;
		}
	}
	return fileEndFlag;
}

bool ServerBuffer::isBufferEmpty() {
	return (begin == end);
}

void ServerBuffer::closeFile() {
	inStream.close();
	cout << "File close!" << endl;
}