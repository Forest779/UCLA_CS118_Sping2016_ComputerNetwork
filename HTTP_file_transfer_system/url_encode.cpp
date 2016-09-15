#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>
#include <iostream>

#include "url_encode.hpp"

using namespace std;

string Url::getUrlHost(string url) {
	url.erase(0,7);
	if (url.find(":") != string::npos) {
		int index = url.find(":");
		return url.substr(0,index);
	} else {
		int index = url.find("/");
		return url.substr(0,index);
	}
	
}

unsigned Url::getUrlPort(string url) {
	url.erase(0,7);
	if (url.find(":") != string::npos) {
		int index = url.find(":");
		url.erase(0,index+1);
		index = url.find("/");
		return stoul(url.substr(0,index));
	} else {
		return 80;
	}
}

string Url::getUrlPath(string url) {
	url.erase(0,7);
	int index = url.find("/");
	url.erase(0,index);
	return url;
}

string Url::getUrlFileName(string url) {
	int index = url.rfind("/");
	url.erase(0,index+1);
	return url;
}

string Url::showIP(string hostname) {
	struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    int status = 0;
    if ((status = getaddrinfo(hostname.c_str(), "80", &hints, &res)) != 0) {
    	std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
    }

    std::cout << "IP addresses for " << hostname.c_str() << ": ";
    string iptemp;
    for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    	// convert address to IPv4 address
    	struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

    	// convert the IP to a string and print it:
    	char ipstr[INET_ADDRSTRLEN] = {'\0'};
    	inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    	std::cout << "  " << ipstr << std::endl;
    	iptemp = ipstr;
	}
    freeaddrinfo(res); // free the linked list

    return  iptemp;
}