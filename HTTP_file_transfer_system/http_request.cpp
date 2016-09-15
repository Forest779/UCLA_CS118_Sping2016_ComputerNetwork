
#include <string>
#include <string.h>

#include <iostream>
#include <sstream>

#include "http_request.hpp"

 using namespace std;

//----------------------------------------------------------------

 HttpRequest::HttpRequest() {
 	method = "GET";
 	version = "HTTP/1.0";
 	output = "";
 }

//----------------------------------------------------------------

 void HttpRequest::setHost(string newHost) {
 	host = newHost;
 }


 string HttpRequest::getHost() {
 	return host;
 }

 void HttpRequest::setPath(string newPath) {
 	path = newPath;
 }

 string HttpRequest::getPath() {
 	return path;
 }

 void HttpRequest::setPort(unsigned newPort) {
 	port = newPort;
 }

 unsigned HttpRequest::getPort() {
 	return port;
 }

 void HttpRequest::setMethod(string newMethod) {
 	method = newMethod;
 }

 string HttpRequest::getMethod() {
 	return method;
 }

 void HttpRequest::setVersion(string newVersion) {
 	version = newVersion;
 }

 string HttpRequest::getVersion() {
 	return version;
 }

//----------------------------------------------------------------

void HttpRequest::print() {
	cout << output;
}

void HttpRequest::encode() {
	output = "";
	output = output + method + " " + path + " " + version + "\r\n";
	output = output + "Host: " + host + ":" + to_string(port) + "\r\n";
	output = output + "\r\n";
}

void HttpRequest::decode(string wire) {
	int index;
	string temp;

	//find method
	index = wire.find(" ");
	temp = wire.substr(0,index);
	wire.erase(0,index+1);
	method = temp;

	//find path
	index = wire.find(" ");
	temp = wire.substr(0,index);
	wire.erase(0,index+1);
	path = temp;

	//find version
	index = wire.find("\r\n");
	temp = wire.substr(0,index);
	wire.erase(0,index+2);
	version = temp;
}

string HttpRequest::send() {
	return output;
}