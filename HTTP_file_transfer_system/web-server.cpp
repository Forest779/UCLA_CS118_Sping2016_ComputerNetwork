#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <vector>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include "http_request.cpp"
#include "http_response.cpp"
#include "url_encode.cpp"

using namespace std;

string folder;

void handleRequest(struct sockaddr_in clientAddr, int clientSockfd);
//void test(int sockfd);

int main(int argc, char *argv[]) {
  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  unsigned short portnum;
  string hostname;

  if(argc!=4) {
    portnum = 4000;
    hostname = "localhost";
    folder = ".";
  } else {
    hostname = argv[1];
    portnum = stoul(argv[2]);
    folder = argv[3];
    if(folder != ".") folder = folder.erase(0,1);
  }

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(portnum);     // short, network byte order
  addr.sin_addr.s_addr = inet_addr(Url::showIP(hostname).c_str());
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }


  while(true) {
    // set socket to listen status
    if (listen(sockfd, 1) == -1) {
      perror("listen");
      return 3;
    }

    // accept a new connection
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);

    if (clientSockfd == -1) {
      perror("accept");
      return 4;
    }

    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    cout << "Accept a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;

    //create a thread for the request
    thread(handleRequest, clientAddr, clientSockfd).detach();

    //std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  return 0;
}


void handleRequest(struct sockaddr_in clientAddr, int clientSockfd) {

  // read/write data from/into the connection
  bool isEnd = false;
  char buf[21] = {0};
  string wire = "";
  stringstream ss;
  string last = "";
  int time = 0;

  fcntl(clientSockfd, F_SETFL, O_NONBLOCK);

  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));

    while (recv(clientSockfd, buf, 20, 0) < 0) {
      cout<< "wait the data for 1s" << endl;
      time++;
      sleep(1);
      if(time>9) {
        cout<< "Already wait 10s for the data. Finish the this thread" << endl;
        time=0;
        return;
      }
    }

    ss << buf << endl;
    //cout << buf;
    int length = ss.str().size();

    string check = last + ss.str().substr(0,length-1);
    wire = wire+ ss.str().substr(0,length-1);

    if(check.find("\r\n\r\n") != string::npos) break;
    last = ss.str().substr(0,length-1);
    ss.str("");
  }

  //decode the message
  HttpRequest request;
  request.decode(wire);


  // generate http response message
  HttpResponse response;
  response.setVersion("HTTP/1.0");


  // 400 Bad Request
  if ((request.getMethod() != "GET") || (request.getPath().size() == 0)) {
    cout << "Cannot understand the request" << endl;
    response.setStatus("400 Bad Request");
    response.setConLength(0);
    response.encode();
    //response.print();
  }


  //search requested file in the local server system
   ifstream inStream;
   string filename = request.getPath();
   filename.erase(0,filename.find("/")+1);

   if (folder != ".") {
    filename = folder + "/" + filename;
    cout << "filename: " << filename << endl;
   }

   inStream.open(filename,ios::binary);

  
  // 404 file not found
  if (inStream.fail())
  {
    cout << "Requested File Not Found" << endl;
    response.setStatus("404 NOT FOUND");
    response.setConLength(0);
    response.encode();
  }
  else
  {
  // file successfully found
  response.setStatus("200 OK");
  vector<unsigned char> vec; 
  unsigned char i;

  while(1) {
    inStream.read((char *)&i, sizeof(i));
    if(inStream.eof()) break;
    vec.push_back(i);
  }


  response.setContent(vec);  
  response.setConLength(vec.size());

  response.encode();
  //response.print();
  }
  
  inStream.close();
  vector<char> input = response.send();

  //send response to client
   if (send(clientSockfd, &input[0], input.size(), 0) == -1) {
     perror("send");
     return;
   }

  close(clientSockfd);

  while(true);

  cout << "Thread finishes" << endl;
}
