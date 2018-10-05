#include <sys/types.h>
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
#include <string.h>
/* You will have to modify the program below */
// starter code supplemented with stuff from beej's

#define MAXBUFSIZE 100

int main (int argc, char * argv[] )
{

	char exitCommand[]="exit\0";
	int sock;                           //This will be our socket
	struct addrinfo hints, *serverInfo;		// hints gives info to getAddrinfo(), res holds info for server addrinfo
	struct sockaddr client_addr; // client's address information
	ssize_t nbytes;                     //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];            //a buffer to store our received message
	int isBound;
	size_t len = MAXBUFSIZE;
	socklen_t fromlen;
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}
	// fetch info about server address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM; // UDP
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	getaddrinfo(NULL, argv[1], &hints, &serverInfo);
	
	
	
	//Causes the system to create a generic socket of type UDP (datagram)
	//if ((sock = **** CALL SOCKET() HERE TO CREATE UDP SOCKET ****) < 0)
	sock = socket(serverInfo->ai_family,serverInfo->ai_socktype,serverInfo->ai_protocol);
	if(sock<0)
	{
		printf("unable to create socket");
	}
	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	isBound = bind(sock, serverInfo->ai_addr, serverInfo->ai_addrlen);
	if(isBound<0)
	{
		printf("unable to bind socket\n");
	}
	
	freeaddrinfo(serverInfo);
	//waits for an incoming message
	fromlen = sizeof client_addr;
	nbytes = recvfrom(sock, buffer, sizeof buffer, 0, &client_addr, &fromlen);
	
	if(strcmp(buffer,"greetings") == 0) // received greeting from client
	{
		nbytes = sendto(sock, buffer, 1, 0, (struct sockaddr*)&client_addr, sizeof(client_addr)); // respond w/ '1' to tell client to display menu
	}
	int exitSignal;
	exitSignal=1;

	buffer[nbytes]='\0';
	while(exitSignal != 0)
	{
		
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, &client_addr, &fromlen);
		buffer[nbytes]='\0';
		printf("%s %zd \n",buffer,nbytes);
		
		exitSignal = strcmp(buffer,exitCommand);

			

	}

	close(sock);
	return 999;
}

























