#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#define MAXBUFSIZE 100

/* You will have to modify the program below */

int main (int argc, char * argv[])
{
	int sock;                           //This will be our socket
	struct addrinfo hints, *res;		// hints gives info to getAddrinfo(), res holds info for local addrinfo
	ssize_t nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	int isBound;
	size_t len = MAXBUFSIZE;
	socklen_t fromlen;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM; // UDP
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	getaddrinfo(NULL, argv[1], &hints, &res);
	fromlen = sizeof res;

	//Causes the system to create a generic socket of type UDP (datagram)
	//if ((sock = **** CALL SOCKET() HERE TO CREATE A UDP SOCKET ****) < 0)
	if ((sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		printf("unable to create socket");
	}

	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	char command[] = "apple";	
	//nbytes = **** CALL SENDTO() HERE ****;
	nbytes = (sendto(sock,command,sizeof command,0,&res,&fromlen));
	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));
	//nbytes = **** CALL RECVFROM() HERE ****;  
	//nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,remote.sin_addr.s_addr,sizeof(remote.sin_addr.s_addr));
	printf("Server says %s\n", buffer);

	close(sock);

}

