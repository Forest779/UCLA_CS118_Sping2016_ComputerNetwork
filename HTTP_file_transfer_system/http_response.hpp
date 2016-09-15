#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <string.h>
#include <vector>

 using namespace std;

 class HttpResponse {
 private:
 	string version;
 	string status;
 	int conLength;
 	vector<char> content;
 	vector<char> output;

 public:
 	HttpResponse();

 	void setVersion(string newVersion);
 	string getVersion();

 	void setStatus(string newStatus);
 	string getStatus();

 	void setConLength(int newConLength);
 	int getConLength();

 	void setContent(vector<unsigned char>);
 	vector<char> getContent();

 	void encode();

 	//void decode(string wire);

 	vector<char> send();

 	void print();

 	static HttpResponse decodeHeader(string wire);
 };

 #endif