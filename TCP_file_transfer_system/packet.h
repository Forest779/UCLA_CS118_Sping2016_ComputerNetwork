#ifndef PACKET_H
#define PACKET_H

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

//const int WINDOW_SIZE = 30720;

class Packet {
public:
	uint16_t seq;
	uint16_t ack;
	uint16_t window;
	uint16_t flags;   //0000000000000ASF
	char payload[1024];

	//char payload[1024] = "hello world";  // payload, maximum 1024 bytes
	Packet();
	Packet(char* packet);

	void set_seq(uint16_t num) { seq = num; }
	uint16_t get_seq() { return seq; }

	void set_ack(uint16_t num) { ack = num; }
	uint16_t get_ack() { return ack; }

	void set_finflag_on() { flags |= 1; }
	void set_finflag_off() { flags &= 0; }  // off ??
	bool is_finflag_set() { return (flags & 1); }

	void set_synflag_on() { flags |= (1 << 1); }
	void set_synflag_off() { flags &= ~(1 << 1); }
	bool is_synflag_set() { return ((flags >> 1) & 1); }

	void set_ackflag_on() { flags |= (1 << 2); }
	void set_ackflag_off() { flags &= ~(1 << 2); }
	bool is_ackflag_set() { return ((flags >> 2) & 1); }

	// void get_flags();
	char* output();// return fixed length(1032 bytes) char*
	int payload_length();// get payload length
	void set_payload(char* data);
	char* get_payload();
	void packet_header();
};
#endif