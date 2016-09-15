#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <iostream>

#include "congestion.cpp"
#include "clientBuffer.cpp"

using namespace std;

// get IP address from hostname
string showIP(string hostname) {
	struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP

    int status = 0;
    if ((status = getaddrinfo(hostname.c_str(), "80", &hints, &res)) != 0) {
    	std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
    }

  //  std::cout << "IP addresses for " << hostname.c_str() << ": ";
    string iptemp;
    
    for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    	// convert address to IPv4 address
    	struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
    	// convert the IP to a string and print it:
    	char ipstr[INET_ADDRSTRLEN] = {'\0'};
    	inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
   // 	std::cout << "  " << ipstr << std::endl;
    	iptemp = ipstr;
	}
    freeaddrinfo(res); // free the linked list

    return  iptemp;
}

// get server address structure from hostname or IP
struct sockaddr_in serverAddressStructure(string hostname, string portNum){
	struct sockaddr_in serverAddr; // server address structure
	memset((char*)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(portNum.c_str()));

	// get IP address from hostname
    serverAddr.sin_addr.s_addr = inet_addr(showIP(hostname).c_str());
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));    
	
    return serverAddr;
}

int checkPacket(Packet* TCP_packet){ // error: 0  data: 1  SYN-ACK: 2   FIN-ACK: 3  
    int packetType = 0;
    
    if (TCP_packet->is_synflag_set() && TCP_packet->is_ackflag_set() && !TCP_packet->is_finflag_set()) packetType = 2;
    if (TCP_packet->is_finflag_set() && !TCP_packet->is_ackflag_set() && !TCP_packet->is_synflag_set()) packetType = 3;
    if (!TCP_packet->is_finflag_set() && !TCP_packet->is_ackflag_set() && !TCP_packet->is_synflag_set()) packetType = 1;

    return packetType;
}

void sendSYN(int sock_fd, struct sockaddr_in serverAddr){
    
    Packet TCP_packet; // initiate seq, ack = 0
    TCP_packet.set_synflag_on();
    socklen_t serverAddrLen = sizeof(serverAddr);

    sendto(sock_fd, TCP_packet.output(), 1032, 0, (struct sockaddr *) &serverAddr, serverAddrLen);
    cout << "Send SYN packet" << endl;
}

void sendFINACK(int sock_fd, struct sockaddr_in serverAddr){
    
    Packet TCP_packet; // initiate seq, ack = 0
    TCP_packet.set_finflag_on();
    TCP_packet.set_ackflag_on();
    socklen_t serverAddrLen = sizeof(serverAddr);

    sendto(sock_fd, TCP_packet.output(), 1032, 0, (struct sockaddr *) &serverAddr, serverAddrLen);
    cout << "Send Fin-ACK packet" << endl;
}

void sendFirstACK(int sock_fd, struct sockaddr_in serverAddr, int acknum){
    
    Packet TCP_packet; // initiate seq, ack = 0
    TCP_packet.set_ackflag_on();
    socklen_t serverAddrLen = sizeof(serverAddr);
    TCP_packet.set_ack((uint16_t)acknum);

    sendto(sock_fd, TCP_packet.output(), 1032, 0, (struct sockaddr *) &serverAddr, serverAddrLen);
    cout << "Send ACK packet " << acknum << endl;

}

int main(int argc, char *argv[])
{

    fd_set readFds;
    fd_set watchFds;
    FD_ZERO(&readFds);
    FD_ZERO(&watchFds);

    string hostname;
    string portNum;

    if (argc != 3){
        cerr << "Input parameter should be: SERVER-HOST-OR-IP Port_Number" << endl;
        return 1;
    }
    else {
        hostname = argv[1];
        portNum = argv[2];
    }

    string ipAddress = showIP(hostname);
    
    // create send and receive socket
    int sock_fd;
    // check if socket is created successfully
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        cerr << "Cannot create a receive socket" << endl;
        return 2;
    }
    
    int maxSockfd = sock_fd;
    FD_SET(sock_fd, &watchFds);

    // create client address structure
    struct sockaddr_in clientAddr;
    memset((char*)&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET; // IPV4
    clientAddr.sin_port = htons(5000); // port number 5000
    clientAddr.sin_addr.s_addr = inet_addr("10.0.0.2");
    memset(clientAddr.sin_zero, '\0', sizeof(clientAddr.sin_zero));

    // bind recvSock_fd to 10.0.0.2:5000
    if (bind(sock_fd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == -1){
        perror("bind");
        return 3;
    }

    // get server address structure
    struct sockaddr_in serverAddr;
    serverAddr = serverAddressStructure(hostname,portNum);
    socklen_t serverAddrLen = sizeof(serverAddr);



//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------

    //begin to do the TCP process
    int time_s = 0;
    int time_ms = 500000;

    struct timeval tv;
    tv.tv_sec = time_s;
    tv.tv_usec = time_ms;

    //state=1 means client is waiting for SYNACK
    //state=2 means client is waiting for data packet.
    //state=3 means client is alreay send FINACK, and checking whether there is retransmission FIN packet.
    
    int state = 0;

    int expectedSeq = 0;
    ClientBuffer* cbuffer = new ClientBuffer("Received_Data.txt");
    
    sendSYN(sock_fd, serverAddr);
    state = 1;

    char* buf = new char[1032];
    Packet* recvPacket = NULL;
    Packet* sendPacket = NULL;

    while(true) {

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
                cout << "Time out, ";
                sendSYN(sock_fd, serverAddr);
            } else if(state == 3) {
                cout << "In 5s, there is no retransmission of FIN packet. The server should already close. We also close the cilent." << endl;
                return 0;
            } else if(state == 2) {
                cout << "Doesn't recieve any data packet in 500ms" << endl;
            } else {
                cout << "Error: unknown state" << endl;
            }
        }
        else {
            tv.tv_sec = time_s;
            tv.tv_usec = time_ms;

            if (FD_ISSET(sock_fd, &readFds)){
                
                if(recvfrom(sock_fd, buf, 1032, 0, (struct sockaddr *) &serverAddr, &serverAddrLen) < 0) {
                    perror("recvfrom");
                    return 5;
                }

                recvPacket = new Packet(buf);
                int packetType = checkPacket(recvPacket);
                if(packetType == 3) {
                    cout << "Receive FIN packet" << endl;
                    if(state != 3) cbuffer->closeFile();
                    sendFINACK(sock_fd, serverAddr);
                    state = 3;
                    tv.tv_sec = 5;
                    tv.tv_usec = 0;


                } else if(packetType == 2) {
                    state = 2;
                    cout << "Receive SYNACK packet" << endl;
                    expectedSeq = (int)recvPacket->get_seq() + 1;
                    cbuffer->setBeginPosition(expectedSeq);
                    sendFirstACK(sock_fd, serverAddr, expectedSeq);

                } else if(packetType == 1) {
                    sendPacket = cbuffer->receivePacketAndSendAck(recvPacket);
                    sendto(sock_fd, sendPacket->output(), 1032, 0, (struct sockaddr *) &serverAddr, serverAddrLen);
                    delete sendPacket;
                    sendPacket = NULL;
                    cbuffer->sendDataToApp();

                } else {
                    cout << "Error: unknow packetType" << endl;
                    exit(0);
                }                
            }

            delete recvPacket;
            recvPacket = NULL;
        }
        
        if(sendPacket != NULL) {
            delete sendPacket;
            sendPacket = NULL;
        }
        if(recvPacket != NULL) {
            delete sendPacket;
            sendPacket = NULL;
        }
    }
    
    delete[] buf;
    buf = NULL;
    delete cbuffer;
    cbuffer = NULL;

    cout << "The program finishes" << endl;
    return 0;
}
