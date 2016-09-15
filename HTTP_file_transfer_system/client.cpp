#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <stdlib.h>
#include <vector>
#include <fcntl.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "http_request.cpp"
#include "http_response.cpp"
#include "url_encode.cpp"

using namespace std;



int main(int argc, char *argv[]) {

	HttpRequest request;
  string url = argv[1];
	request.setHost(Url::getUrlHost(url));
	request.setPath(Url::getUrlPath(url));
	request.setPort(Url::getUrlPort(url));
	request.encode();
	//request.print();

//------------------------------------------------

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(request.getPort());
	serverAddr.sin_addr.s_addr = inet_addr(Url::showIP(request.getHost()).c_str());
	memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

//build connection with server
	if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    	perror("connect");
    	return 2;
  }

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);

  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
     perror("getsockname");
     return 3;
  }

//send request to server
  string input = request.send();
  cout << input;

  //cout << "client sleep 15s to test timeout" << endl;
  //sleep(15);
  //cout << "client wake up" << endl;

  if (send(sockfd, input.c_str(), input.size(), 0) == -1) {
     perror("send");
     return 4;
  }

//receive response from server
  bool isEnd = false;
  char buf[21] = {0};
  string wire = "";
  string ss = "";
  string last = "";
  string remain = "";
  HttpResponse header;
  //int wLength = 0;
  int time = 0;

  fcntl(sockfd, F_SETFL, O_NONBLOCK);



//read the head of response
 while(!isEnd) {
    memset(buf, '\0', sizeof(buf));

    while (recv(sockfd, buf, 20, 0) < 0) {
      cout<< "wait the data for 1s" << endl;
      cout << endl;
      time++;
      sleep(1);
      if(time>9) {
        cout<< "Already wait 10s for the data. Finish the client" << endl;
        time=0;
        return 0;
      }
    }

    //cout << buf;
    //ss << buf << endl;s

    for (int k=0;k<20;k++) {
      ss = ss + buf[k];
    }

    string check = last + ss;
    //cout << check << endl;
    wire = wire + ss;

    if(check.find("\r\n\r\n") != string::npos) {
      header = HttpResponse::decodeHeader(wire);
      //-------------------------------------------------------------
      //Maybe have problem
      remain = check.substr(check.find("\r\n\r\n")+4, check.size());
      //-------------------------------------------------------------
      //wLength = header.getConLength() - remain.size();
      int index = wire.find("\r\n\r\n");
      cout << wire.substr(0,index);
      cout << endl;
      break;
    } 
    last = ss;
    ss = "";
  }
  //header.print();
  ss = "";

  if(header.getStatus() == "404 NOT FOUND") {
    cout << "Cannot find the file" << endl;
    close(sockfd);
    return 0;
  }

  if(header.getStatus() == "400 Bad Request") {
    cout << "The server cannot understand the http request message" << endl;
    close(sockfd);
    return 0;
  }

  if(header.getStatus() == "200 OK") {
    //---------------------------------------------------------
    //read the content of response
    //int readTime = wLength/20 + 1;
    // save file to local system
    string localPath = Url::getUrlFileName(url);
    ofstream outStream;
    outStream.open(localPath,ios::binary);

    for (unsigned int k=0;k<remain.size();k++) {
      outStream.write((char *)&remain[k], sizeof(unsigned char));
    }
    
    //int count=0;
    while(true) {
      int flag = 0;
      memset(buf, '\0', sizeof(buf));
      flag = recv(sockfd, buf, 20, 0);
      while (flag < 0) {
        cout<< "wait the data for 1s" << endl;
        time++;
        sleep(1);
        if(time>9) {
          cout<< "Already wait 10s for the data. Finish the client" << endl;
          time=0;
          return 0;
        }
        flag = recv(sockfd, buf, 20, 0);
      }
      if(flag > 0) {
        for (int k=0;k<20;k++) {
          if(k < flag) {
            outStream.write((char *)&buf[k], sizeof(unsigned char));
            //count++;
          }
        }
      }
      if(flag==0) break;
    }
    outStream.close();

    cout << endl;
    close(sockfd);

    cout << "close client" << endl;
    return 0;
  } else {
    cout << "Unknown status" << endl;
    close(sockfd);
    return 0;
  }
}




  
 