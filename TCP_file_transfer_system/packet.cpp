#include <math.h>

#include "packet.h"

using namespace std;


Packet::Packet()
{
	seq = 0;
	ack = 0;
	window = 30720;
	flags = 0x00;
	memset(payload, '\0', sizeof(payload));

}

Packet::Packet(char* packet){
	uint16_t high = (uint16_t) packet[0];
	high = high & 0xFF;
	uint16_t low = (uint16_t) packet[1];
	low = low & 0xFF;
	seq = low | high << 8;
	//cout << "uint_16t packet[0] is " << (uint16_t) packet[0]<< endl;
	//cout << "uint_16t packet[1] is " << (uint16_t) packet[1] << endl;
	//seq = (uint16_t) (packet[1] | (packet[0]) << 8);
	
	high = (uint16_t) packet[2];
	high = high & 0xFF;
	low = (uint16_t) packet[3];
	low = low & 0xFF;
	ack = low | high << 8;

	window = (uint16_t) 30720;
	flags = (uint16_t) packet[7] | (uint16_t)(packet[6]) << 8;
	for(int i=0;i<1024;i++){
		payload[i] = packet[i+8];
	}

}

void Packet::packet_header(){
	cout << "seq number is: " << seq << endl;
	cout << "ack number is: " << ack << endl;
	cout << "window size is: " << window << endl;
	cout << "SYN is: " << this->is_synflag_set() << endl;
	cout << "ACK is: " << this->is_ackflag_set() << endl;
	cout << "FIN is: " << this->is_finflag_set() << endl;
}

char* Packet::output(){
		
	char* packet;
	packet = new char[1032];

	packet[0] = seq >> 8;  // high byte
	packet[1] = seq & 0xFF; // low byte
 //	cout << "seq number is " << (int)(unsigned char)packet[0] << " " << (int)(unsigned char)packet[1] << endl;
//	cout << "seq number is " << (uint16_t)packet[0] << " " <<(uint16_t)packet[1] << endl; 
// 	cout << "seq num is: " << seq << endl;
	packet[2] = ack >> 8;
	packet[3] = ack & 0xFF;
//	cout << "ack number is " << (int)packet[2] << " " << (int)packet[3] << endl;
//	cout << "ack number is " << (uint16_t)packet[2] << " " << (uint16_t)packet[3] << endl;
	packet[4] = window >> 8;
	packet[5] = window & 0xFF;
	packet[6] = flags >> 8;
	//cout << "flag first 8 bit is: " << packet[6] << endl;
	packet[7] = flags & 0xFF;
	//cout << "flag last 8 bit is: " << packet[7] << endl;

	for (int i = 0; i < 1024; ++i)
	{
		packet[i+8] = payload[i];
	//	cout << "packet" << i+8 << " element is: " << packet[i+8] << endl;
	}
	//cout << "packet is: " << packet << endl;
	//cout << "payload is: " << payload << endl;

	return packet;
}

int Packet::payload_length(){
	return min((int)strlen(payload), 1024);
}

void Packet::set_payload(char* data){
	//cout << "size of data is: " << sizeof(data) << endl;
	memcpy(payload, data, sizeof(payload));
	//payload[sizeof(payload)-1] = '\0';
//	cout << sizeof(payload) << "sizeof" << endl;
//	cout << strlen(payload) << "strlen" << endl;
}


char* Packet::get_payload(){
	return payload;
}







