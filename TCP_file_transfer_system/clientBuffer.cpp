#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "clientBuffer.h"

using namespace std;

ClientBuffer::ClientBuffer(string filename) {
	memset(buffer, '\0', sizeof(buffer));
	for(int i=0; i<(int)sizeof(effectiveData); i++) {
		effectiveData[i] = false;
	}
	begin = 0;
	end = 0;
	outStream.open(filename);
	if(outStream.fail()) {
		cout << "Error: Cannot find the file!" << endl;
		exit(0);
	}
	cout << "File open!" << endl;
}

void ClientBuffer::setBeginPosition(int seq) {
	begin = seq;
	end = seq;
}

Packet* ClientBuffer::receivePacketAndSendAck(Packet* packet) {
	int seq = (int)packet->get_seq();
	int halfBufferSize = (int)sizeof(buffer)/2;
    Packet* Ack;

	if(begin == seq) {//receive new in order data
		cout << "Receive new data (in order) packet " << seq << endl;

		//Send ack back
		Ack = new Packet();
		int length = packet->payload_length();
		int acknum = (begin + length) % (int)sizeof(buffer);
		Ack->set_ack((uint16_t)acknum);
		Ack->set_ackflag_on();
		//cout << "beign is " << begin << " length is " << length << endl;
		cout << "Send Ack packet " << acknum << endl;

		//write data to buffer
		char* payload = packet->get_payload();
		if((seq+length) <= (int)sizeof(buffer)) {// Don't need to go from tail to head
			int j=0;
			for (int i=seq; i<(seq+length); i++) {
				if(effectiveData[i] == true) {
					cout << "Error: Data position confiction, when writing new in order data to buffer" << endl;
					exit(0);
				}
				if(buffer[i] != '\0') {
					cout << "Error: Find old data not removed, when writing new in order data to buffer " << endl;
					exit(0);
				}
				effectiveData[i] = true;
				buffer[i] = payload[j];
				j++;
			}
			cout << "At " << seq << " write " << length << " bytes data to buffer" <<endl;
		} else {// need to go from tail to head
			int j=0;
			for(int i=seq; i<(int)sizeof(buffer); i++) {//from seq to tail
				if(effectiveData[i] == true) {
					cout << "Error: Data position confiction, when writing new in order data to buffer" << endl;
					exit(0);
				}
				if(buffer[i] != '\0') {
					cout << "Error: Find old data not removed, when writing new in order data to buffer " << endl;
					exit(0);
				}
				effectiveData[i] = true;
				buffer[i] = payload[j];
				j++;
			}
			for(int i=0; i<(seq+length-(int)sizeof(buffer)); i++) {//form head to finish the wristing
				if(effectiveData[i] == true) {
					cout << "Error: Data position confiction, when writing new in order data to buffer" << endl;
					exit(0);
				}
				if(buffer[i] != '\0') {
					cout << "Error: Find old data not removed, when writing new in order data to buffer " << endl;
					exit(0);
				}
				effectiveData[i] = true;
				buffer[i] = payload[j];
				j++;
			}
			cout << "At " << seq << " write " << length << " bytes data to buffer" << endl;
			cout << "And go from buffer tail to the head" << endl;
		}

	} else if(effectiveData[seq] == true) {//receive data which is already in buffer, but not in file
		cout << "Already receive this data packet (not write to file yet) before" << seq <<endl;
		Ack = new Packet();
		Ack->set_ack((uint16_t)begin);
		Ack->set_ackflag_on();
		cout << "Send Ack packet " << begin << " Retransmission" << endl;
	} else if( ((begin>=halfBufferSize) && (seq<begin) && (seq>=(begin-halfBufferSize))) //receive data which is already in file
		       ||
		       ((begin<halfBufferSize) && (seq<begin))
		       ||
		       ((begin<halfBufferSize) && (seq>=begin+halfBufferSize)) )
	{
		cout << "Already receive this data packet (already write to file) before" << seq << endl;
		Ack = new Packet();
		Ack->set_ack((uint16_t)begin);
		Ack->set_ackflag_on();
		cout << "Send Ack packet " << begin << " Retransmission" << endl;
	} else if(effectiveData[seq] == false) {// receive new out of order data
		//send dup ack back
		cout << "Recieve new data (out of order) packet " << seq << endl;
		Ack = new Packet();
		Ack->set_ack((uint16_t)begin);
		Ack->set_ackflag_on();
		cout << "Send Ack packet " << begin << " Retransmission" << endl;

		//write the data to buffer
		int length = packet->payload_length();
		char* payload = packet->get_payload();
		if((seq+length) <= (int)sizeof(buffer)) {// Don't need to go from tail to head
			int j=0;
			for (int i=seq; i<(seq+length); i++) {
				if(effectiveData[i] == true) {
					cout << "Error: Data position confiction, when writing new out of order data to buffer" << endl;
					exit(0);
				}
				if(buffer[i] != '\0') {
					cout << "Error: Find old data not removed, when writing new out of order data to buffer " << endl;
					exit(0);
				}
				effectiveData[i] = true;
				buffer[i] = payload[j];
				j++;
			}
			cout << "At " << seq << " write " << length << " bytes data to buffer" <<endl;
		} else {// need to go from tail to head
			int j=0;
			for(int i=seq; i<(int)sizeof(buffer); i++) {//from seq to tail
				if(effectiveData[i] == true) {
					cout << "Error: Data position confiction, when writing new out of order data to buffer" << endl;
					exit(0);
				}
				if(buffer[i] != '\0') {
					cout << "Error: Find old data not removed, when writing new out of order data to buffer " << endl;
					exit(0);
				}
				effectiveData[i] = true;
				buffer[i] = payload[j];
				j++;
			}
			for(int i=0; i<(seq+length-(int)sizeof(buffer)); i++) {//form head to finish the wristing
				if(effectiveData[i] == true) {
					cout << "Error: Data position confiction, when writing new out of order data to buffer" << endl;
					exit(0);
				}
				if(buffer[i] != '\0') {
					cout << "Error: Find old data not removed, when writing new out of order data to buffer " << endl;
					exit(0);
				}
				effectiveData[i] = true;
				buffer[i] = payload[j];
				j++;
			}
			cout << "At " << seq << " write " << length << " bytes data to buffer" << endl;
			cout << "And go from buffer tail to the head" << endl;
		}
	}
	return Ack;
}

void ClientBuffer::sendDataToApp() {
	int count = 0;
	//char tmp1;
	while(effectiveData[begin] == 1) {
		char tmp;
		tmp = buffer[begin];
		outStream << noskipws << tmp;
		effectiveData[begin] = 0;
		buffer[begin] = '\0';
		begin = (begin+1) % (int)sizeof(buffer);
		count++;
		//tmp1 = tmp;
	}
	//cout << "The last letter is: " << tmp1 << endl;
	cout << "Send " << count << " bytes data to app" <<endl;
}

void ClientBuffer::closeFile() {
	outStream.close();
	cout<< "File close!" << endl;
}

