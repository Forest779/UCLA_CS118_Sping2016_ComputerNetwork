#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <vector>

#include <iostream>
#include <sstream>
#include <fstream>
#include "http_request.cpp"
#include "http_response.cpp"

using namespace std;

int
main()
{
  fd_set readFds;
  fd_set errFds;
  fd_set watchFds;
  fd_set writeFds;
  FD_ZERO(&readFds);
  FD_ZERO(&writeFds);
  FD_ZERO(&errFds);
  FD_ZERO(&watchFds);

  // create a socket using TCP IP
  int listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
  int maxSockfd = listenSockfd;

  // put the socket in the socket set
  FD_SET(listenSockfd, &watchFds);

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(listenSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(40000);     // short, network byte order
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
  if (bind(listenSockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  // set the socket in listen status
  if (listen(listenSockfd, 10) == -1) {
    perror("listen");
    return 3;
  }

  // initialize timer (2s)
  struct timeval tv;
  while (true) {
    // set up watcher
    HttpRequest request;
    HttpResponse response;
    int nReadyFds = 0;
    readFds = watchFds;
    errFds = watchFds;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if ((nReadyFds = select(maxSockfd + 1, &readFds, &writeFds, &errFds, &tv)) == -1) {
      perror("select");
      return 4;
    }

    if (nReadyFds == 0) {
      //handle timeout
    }
    else {
      for(int fd = 0; fd <= maxSockfd; fd++) {
        // get one socket for reading
        if (FD_ISSET(fd, &readFds)) {
          if (fd == listenSockfd) { // this is the listen socket
            struct sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            int clientSockfd = accept(fd, (struct sockaddr*)&clientAddr, &clientAddrSize);

            if (clientSockfd == -1) {
              perror("accept");
              return 5;
            }

            char ipstr[INET_ADDRSTRLEN] = {'\0'};
            inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
            std::cout << "Accept a connection from: " << ipstr << ":" <<
              ntohs(clientAddr.sin_port) << " with Socket ID: " << clientSockfd << " related to listening socket "<<fd<<std::endl;

            // update maxSockfd
            if (maxSockfd < clientSockfd)
              maxSockfd = clientSockfd;

            // add the socket into the socket set
            FD_SET(clientSockfd, &watchFds);
   //         cout << "client socket "<< clientSockfd << " is added into watchFds" <<endl;
   //         cout <<"max socket is "<< maxSockfd <<endl;
          }
          else { // this is the normal socket, http request
            // char buf[20];
            // int recvLen = 0;
            // memset(buf, '\0', sizeof(buf));
            // if ((recvLen = recv(fd, buf, 20, 0)) == -1) {
            //   perror("recv");
            //   return 6;
            // }

            // std::string output(buf);
            // if (output == "50" || recvLen == 0) {
            //   close(fd);
            //   FD_CLR(fd, &watchFds);

            // }
            // else {
            //   std::cout << "Socket " << fd << " receives: " << buf << std::endl;
            // }

              // read data from the connection
              bool isEnd = false;
              char buf[21] = {0};
              string wire = "";
              stringstream ss;
              string last = "";

              while (!isEnd) {
                memset(buf, '\0', sizeof(buf));

                if (recv(fd, buf, 20, 0) == -1) {
                perror("recv");
                return 10;
                }

              ss << buf << endl;
              //cout << buf;
              int length = ss.str().size();

              string check = last + ss.str().substr(0,length-1);
              wire = wire+ ss.str().substr(0,length-1);

              if(check.find("\r\n\r\n") != string::npos) {
                break;
              }
              last = ss.str().substr(0,length-1);
              ss.str("");
              }
              
              FD_CLR(fd, &watchFds);  
              FD_CLR(fd,&readFds);
     //         cout << "reading from socket "<< fd <<endl;
           // update maxSockfd
           
            //decode the message
              
              request.decode(wire);
              request.encode();
     //         request.print();
      //        cout <<"max socket is "<< maxSockfd <<endl;
              FD_SET(fd,&writeFds);
              if (maxSockfd < fd){
                maxSockfd = fd;
              }
              
          }
        }
        if (FD_ISSET(fd, &writeFds)){ // generate http response
          
          response.setVersion("HTTP/1.0");

  // 400 Bad Request
          if ((request.getMethod() != "GET") || (request.getVersion() != "HTTP/1.0")||(request.getPath().size() == 0)) {
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
            inStream.open(filename,ios::binary);


   
  
  // 404 file not found
            if (inStream.fail())
            {
              cout << "Requested File Not Found" << endl;
              response.setStatus("404 NOT FOUND");
              response.setConLength(0);
              response.encode();
              //response.print();  
            }
            else
            {
            // file successfully found
              response.setStatus("200 OK");
              vector<unsigned char> vec; 
              unsigned char i;

              while(1) {
              inStream.read((char *)&i, sizeof(i));
              vec.push_back(i);
              if(inStream.eof()) break;
              }

  // for(unsigned int k=0;k<buffer.size();k++) {
  //   cout<< hex <<(int)buffer[k] << " ";
  // }
  // cout << endl;

              response.setContent(vec);  
              response.setConLength(vec.size());

              response.encode();
  //response.print();
              }
  
              inStream.close();
              vector<char> input = response.send();
  //char *cp = &input[0];


   // for(unsigned int k=0;k<input.size();k++) {
   //   cout<<input[k];
   // }
   // cout << endl;

  //send response to client
              if (send(fd, &input[0], input.size(), 0) == -1) {
                perror("send");
                return 11;
              }

    // for(int k=0;k<1000;k++) {
    //  cout<<input[k];
    // }

  // cout << endl;
              close(fd);
              FD_CLR(fd,&writeFds);
              cout << "response socket " << fd <<" is written and sent"<<endl;
              if (maxSockfd < fd){
                 maxSockfd = fd;
              }
             

        }
      }
    }
  }

  return 0;
}
