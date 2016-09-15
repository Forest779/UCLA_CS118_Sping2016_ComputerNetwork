#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <string.h>

 using namespace std;

 class HttpRequest {
 private:
 	string method;
 	string host;
 	string version;
 	string path;
 	string output;
 	unsigned short port; 

 public:
 	HttpRequest();

 	void setHost(string newHost);
 	string getHost();

 	void setPath(string newPath);
 	string getPath();

 	void setPort(unsigned newPort);
 	unsigned getPort();

 	void setMethod(string newMethod);
 	string getMethod();

 	void setVersion(string newVersion);
 	string getVersion();

 	void encode();

 	void decode(string wire);

 	string send();

 	void print();
 };

 #endif