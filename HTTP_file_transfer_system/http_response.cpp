#include <string>
#include <string.h>

#include <iostream>
#include <map>
#include <vector>

#include "http_response.hpp"

 using namespace std;

//-----------------------------------------------------
 HttpResponse::HttpResponse() {
 	version = "HTTP/1.0";
 	status = "200 OK";
 	conLength = 0;
 	//content = new vector<unsigned char>;
 	//output = new vector<unsigned char>;
 }

//-----------------------------------------------------

 void HttpResponse::setVersion(string newVersion) {
 	version = newVersion;
 }

 string HttpResponse::getVersion() {
 	return version;
 }

 void HttpResponse::setStatus(string newStatus) {
 	status = newStatus;
 }

 string HttpResponse::getStatus() {
 	return status;
 }

 void HttpResponse::setConLength(int newConLength) {
	 conLength = newConLength;
 }

 int HttpResponse::getConLength() {
 	return conLength;
 }

 void HttpResponse::setContent(vector<unsigned char> newContent) {
 	for (unsigned int k=0;k<newContent.size();k++) {
 		content.push_back(newContent[k]);
 	}
 }

 vector<char> HttpResponse::getContent() {
 	return content;
 }

//-----------------------------------------------------
 
 void HttpResponse::encode() {
 	output.clear();
 	string tmp = "";
 	tmp = tmp + version + " " + status + "\r\n";
 	tmp = tmp + "Content-Length: " + to_string(conLength) + "\r\n";
 	tmp = tmp + "\r\n";
 	for (unsigned int k=0; k<tmp.size(); k++) {
 		output.push_back(tmp[k]);
 	}
 	for (unsigned int k=0; k<content.size(); k++) {
 		output.push_back(content[k]);
 	}
 }

 void HttpResponse::print() {
 	cout << version << " " << status << endl;
 	cout << "Content-Length: " << conLength << endl;
 	return;
 }

 vector<char> HttpResponse::send() {
 	return output;
 }

 HttpResponse HttpResponse::decodeHeader(string wire) {
 	HttpResponse header;
 	int index;
 	string temp;
 	// find version
 	index = wire.find(" ");
 	temp = wire.substr(0,index);
 	header.setVersion(temp);
 	wire.erase(0,index+1);

 	//find status
 	index = wire.find("\r\n");
 	temp = wire.substr(0,index);
 	header.setStatus(temp);
 	wire.erase(0,index+2);

 	//find other headers
 	map<string, string> headers;//to save all the possible headers

 	while(true) {
 		string key = "";
 		string value = "";

 		if(wire.find("\r\n") == 0) {//meet \r\n\r\n
 			break;
 		} else {
 			index = wire.find(":");
 			key = wire.substr(0,index);
 			wire.erase(0,index+2);
 			index = wire.find("\r\n");
 			value = wire.substr(0,index);
 			wire.erase(0,index+2);
 			headers.insert(pair<string, string>(key, value));
 		}
 	}
 	map<string, string>::iterator it;
 	it = headers.find("Content-Length");
 	if(it==headers.end()) {
 		header.setConLength(-1);
 	} else {
 		string ltmp = headers["Content-Length"];
 		header.setConLength(stoi(ltmp));
 	}
 	return header;
 }