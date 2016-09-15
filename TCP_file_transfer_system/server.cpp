#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <sys/select.h>

#include "serverBuffer.cpp"
#include "congestion.cpp"

using namespace std;

int checkPacket(Packet* TCP_packet){ // error: 0  ACK: 4   SYN: 5  FINACK: 6
    int packetType = 0;
    
    if (!TCP_packet->is_synflag_set() && TCP_packet->is_ackflag_set() && !TCP_packet->is_finflag_set()) packetType = 4;
    if (TCP_packet->is_synflag_set() && !TCP_packet->is_finflag_set() && !TCP_packet->is_ackflag_set()) packetType = 5;
    if (TCP_packet->is_finflag_set() && TCP_packet->is_ackflag_set() && !TCP_packet->is_synflag_set()) packetType = 6;

    return packetType;
}

void sendSYNACK(int sock_fd, struct sockaddr_in clientAddr, int initseq){  // initial seq number
    
    Packet TCP_packet; // initiate seq, ack = 0
    TCP_packet.set_synflag_on();
    TCP_packet.set_ackflag_on();
    TCP_packet.set_seq(initseq);
    socklen_t clientAddrLen = sizeof(clientAddr);

    sendto(sock_fd, TCP_packet.output(), 1032, 0, (struct sockaddr *) &clientAddr, clientAddrLen);
    cout << "Send SYNACK packet with initial sequnce number " << initseq << endl;

}


void sendFIN(int sock_fd, struct sockaddr_in clientAddr){
    
    Packet TCP_packet; // initiate seq, ack = 0
    TCP_packet.set_finflag_on();
    socklen_t clientAddrLen = sizeof(clientAddr);

    sendto(sock_fd, TCP_packet.output(), 1032, 0, (struct sockaddr *) &clientAddr, clientAddrLen);

    cout << "Send Fin packet" << endl;

}

int main(int argc, char *argv[])
{
  fd_set readFds;
  fd_set watchFds;
  FD_ZERO(&readFds);
  FD_ZERO(&watchFds);

  string filename;
  string portNum;

  if (argc != 3){
        cerr << "Input parameter should be: Port_Number File_Name" << endl;
        return 1;
    }else {
        portNum = argv[1];
        filename = argv[2];
    }

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0) {
  	perror("socket");
  	return 1;
  }
  
  int maxSockfd = sockfd;
  FD_SET(sockfd, &watchFds);

  struct sockaddr_in addr;
  memset((char*)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(portNum.c_str()));     // short, network byte order
  addr.sin_addr.s_addr = inet_addr("10.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
     perror("bind");
     return 2;
  }

  //create client address
  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  //memset((char*)&clientAddr, 0, sizeof(clientAddr));
  // clientAddr.sin_family = AF_INET;
  // clientAddr.sin_port = htons(5000);     // short, network byte order
  // clientAddr.sin_addr.s_addr = inet_addr("10.0.0.2");
  // memset(clientAddr.sin_zero, '\0', sizeof(clientAddr.sin_zero));
  

  //receive the SYN from client
  char* buf = new char[1032];
  Packet* recvPacket = NULL;
  Packet** sendPacket = NULL;
  Packet* resendPacket = NULL;

  //state=1 means server already received SYN packet, and send a SYNACK back.
  //state=2 means server is waiting for ack for a data packet.
  //state-3 means server is waiting for FINACK packet.
  int state = 0;


  while(true) {
    if(recvfrom(sockfd, buf, 1032, 0, (struct sockaddr *) &clientAddr, &clientAddrLen) < 0) {
      perror("recv");
      return 4;
    }
    recvPacket = new Packet(buf);

    if(checkPacket(recvPacket) == 5) {
      cout << "Receive a request from client" << endl;
      state = 1;
      break;
    }

    delete recvPacket;
  }

  //store the client ip and port information
  struct sockaddr_in clientAddrAccepted;
  memset((char*)&clientAddrAccepted, 0, sizeof(clientAddrAccepted));
  clientAddrAccepted.sin_port = clientAddr.sin_port;
  clientAddrAccepted.sin_addr.s_addr = clientAddr.sin_addr.s_addr;

  //set initial sequnce number
  int initseq = 1000;

  //send SYNACK back
  sendSYNACK(sockfd, clientAddr, initseq);

//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------

  ServerBuffer* sbuffer = new ServerBuffer(filename, 30720);
  sbuffer->setBeginPosition(initseq+1);
  int lastAcknum = -1;

  //----------------------------------------------
  int cWindow = 1024;
  int ssthresh = 30720;
  CongestionWindow conWindow;
  //----------------------------------------------

  //int count_a = 1;

  int fileEnd = 0;//Judge whether the file is end

  int time_s = 0;
  int time_ms = 500000;

  struct timeval tv;
  tv.tv_sec = time_s;
  tv.tv_usec = time_ms;
  
  while(true) {

    //when read all the file and receive the ack for all data, send FIN to client.
    if(sbuffer->isBufferEmpty() && (fileEnd == 1) && (state != 3)){
      cout << "All data have been sent and been acked" << endl;
      sendFIN(sockfd,clientAddr);
      state = 3;
    }

    //cout << "A new turn " << count_a << endl;
    //count_a++;
    // set up watcher
    int nReadyFds = 0;
    readFds = watchFds;
    
    if ((nReadyFds = select(maxSockfd + 1, &readFds, NULL, NULL, &tv)) == -1) {
      perror("select");
      return 4;
    }
    if (nReadyFds == 0){
        //time out
        tv.tv_sec = time_s;
        tv.tv_usec = time_ms;

        if(state == 1) {
          cout << "Time out, " ;

          sendSYNACK(sockfd, clientAddr, initseq);

        } else if(state == 2) {
          cout << "Time out, ";
          conWindow.adjustParameter(1);
          cWindow = conWindow.getWindowSize();
          ssthresh = conWindow.getThreshold();
          sbuffer->changeCongestionWindow(cWindow, ssthresh);

          resendPacket = sbuffer->retransmit();

          sendto(sockfd, resendPacket->output(), 1032, 0, (struct sockaddr *) &clientAddr, clientAddrLen);

          delete resendPacket;
          resendPacket = NULL;

        } else if(state == 3) {
          cout << "Time out, ";
      
          sendFIN(sockfd,clientAddr);

        } else {
          cout << "Error: unknown state." << endl;
          exit(0);
        }
    }
    else{
      tv.tv_sec = time_s;
      tv.tv_usec = time_ms;

      if (FD_ISSET(sockfd, &readFds)){

        int packetnum = 0;

        if(recvfrom(sockfd, buf, 1032, 0, (struct sockaddr *) &clientAddr, &clientAddrLen) <0) {
          perror("recvform");
          return 5;
        }
        if((clientAddrAccepted.sin_port != clientAddr.sin_port) 
          || 
          (clientAddrAccepted.sin_addr.s_addr != clientAddr.sin_addr.s_addr)) 
        {
          cout << "Receive information from unknown client. Ignore this information." <<endl;
          continue;
        }
        recvPacket = new Packet(buf);
        int packetType = checkPacket(recvPacket);
        if(packetType == 4) {
          if(state == 1) state = 2;
          int acknum = (int)recvPacket->get_ack();
          if(lastAcknum == acknum) {
            cout << "Recieve duplicate Ack" << endl;
            //---------------------
            //do something for fast retransmission
            //---------------------
          } else {
            conWindow.adjustParameter(0);
            cWindow = conWindow.getWindowSize();
            ssthresh = conWindow.getThreshold();
            sbuffer->changeCongestionWindow(cWindow, ssthresh);

            lastAcknum = acknum;
            sbuffer->receiveAck(acknum);
            if (fileEnd == 0) {
              fileEnd = sbuffer->receiveDataFromApp();
            }
            sendPacket = sbuffer->sendPacket(packetnum);
            //cout << sizeof(sendPacket);
            //cout << "The size of sendPacket array is " << packetnum << endl;


            for(int i=0; i<packetnum; i++) {
              //cout << "Actually send " << i << "th sendPacket" << endl;
              sendto(sockfd, sendPacket[i]->output(), 1032, 0, (struct sockaddr *) &clientAddr, clientAddrLen);
            }

            for (int i=0; i<packetnum; i++) {
              delete sendPacket[i];
              sendPacket[i] = NULL;
            }
            delete sendPacket;
            sendPacket = NULL;
          }

        }else if(packetType == 5) {
          cout << "Re";
          sendSYNACK(sockfd, clientAddr, initseq);

        }else if(packetType == 6) {
          cout << "Received FINACK packet, ready to close server." << endl;
          sbuffer->closeFile();
          cout << "Server closed" << endl;
          return 0;

        }else {
          cout << "Error: unexpected packet type!" << endl;
          exit(0);
        }

        //cout << "delete recvPacket" << endl;
        delete recvPacket;
        recvPacket = NULL;
      }
    }  

    if(recvPacket != NULL) {
      delete recvPacket;
      recvPacket = NULL;
    }  
    if(sendPacket != NULL) {
      delete sendPacket;
      sendPacket = NULL;
    }
     if(resendPacket != NULL) {
       delete resendPacket;
       resendPacket = NULL;
     }
  }

  delete[] buf;
  buf =  NULL;
  delete sbuffer;
  sbuffer = NULL;

  cout << "Program finish" << endl;
  return 0;
}
