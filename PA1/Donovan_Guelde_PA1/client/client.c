#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define MAXBUFSIZE 500
#define GREETING "greetings"
#define TIMEOUT 1
#define LS "ls"
#define GET "get"
#define DELETE "delete"
#define PUT "put"
#define EXIT "exit"

#define DONE "done"

/* You will have to modify the program below */
//receive file from other system
void downloadFile(int sock, struct sockaddr_in remote, char* buffer,char* filename);

//uploads file to other system
void sendFile(int sock, struct sockaddr_in remote,char* buffer);

//sends exit message to server, waits for reply.  if timeout, assumes server exited, closes client
int exitServer(int sock, struct sockaddr_in remote);

//get local dir of server
void getDir(int sock,struct sockaddr_in remote);

//display main menu every loop
void displayMenu();

//sends a packet (saves a little typing each time...) returns bytes sent
int sendPacket(int socket, char* message, int length,struct sockaddr_in remote);




int main (int argc, char * argv[]){

int nbytes;                             // number of bytes sent/received
int sock;                               //this will be our socket
char buffer[MAXBUFSIZE];				// buffer to send/receive
struct sockaddr_in remote;				//server's address info
int remote_length=sizeof(remote);	


if (argc < 3) //usage check...
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

/******************
  Here we populate a sockaddr_in struct with
  information regarding where we'd like to send our packet 
  i.e the Server.
 ******************/

struct timeval timeOut; //used for client timeout
timeOut.tv_sec = 1; //should timeout after one second, but not consistent, don't know why not...
timeOut.tv_usec = 0; //no microseconds
memset((char *) &remote, 0, sizeof(remote)); //zero out struct
remote.sin_family = AF_INET; //internet protocol
remote.sin_port = htons(atoi(argv[2])); //port specified in CLA by user
if(inet_aton(argv[1],&remote.sin_addr)==0) // put server IP into server's sockad_in struct
{
	printf("inet_aton fail\n");
	return -1;
}

if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) //open socket
{
	printf("unable to open socket\n");
	return -1;
}

if((setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO, (char*)&timeOut, sizeof(struct timeval)))<0) //set timeout option==true
{
	printf("unable to set socket options\n");
	return -1;
} 


//handshake, try 10 times, give up
int timeOutCount=0;
//sends 'greetings', and looks for same in return.  If received, have connected to server
nbytes = sendto(sock, GREETING, strlen(GREETING), 0, (struct sockaddr*)&remote, remote_length); // send greeting
nbytes = recvfrom(sock, buffer, MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length); //receive reply
while(!(nbytes > 0 && nbytes < MAXBUFSIZE)) //if timeout
{
	timeOutCount+=1;
	nbytes = sendto(sock, GREETING, strlen(GREETING), 0, (struct sockaddr*)&remote, remote_length); //resend greeting
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length); //receive reply
	printf("timeout\n");
	if(timeOutCount>10)// give up...
		{
			printf("cannot connect to server, timeout limit exceeded\n");
			return -1;
		}
}

if(strncmp(buffer,GREETING,strlen(GREETING))==0) // connected w/ server and received reply
{
printf("connected to server\n");
printf("server IP:  %s server port:  %d\n",inet_ntoa(remote.sin_addr),ntohs(remote.sin_port));
struct sockaddr_in localAddress;
socklen_t addressLength = sizeof(localAddress);
getsockname(sock, (struct sockaddr*)&localAddress,   \
                &addressLength);
printf("local address: %s  local port:  %d\n", inet_ntoa( localAddress.sin_addr),(int) ntohs(localAddress.sin_port));
}

int exitFlag=0;

//****************************************MAIN LOOP *************************************************
while(!exitFlag)
{
	displayMenu();
	bzero(buffer,MAXBUFSIZE); // clear buffer
	char string[MAXBUFSIZE]; // place to take user input
	int size; //size of user input
	fgets(string,MAXBUFSIZE,stdin);  // get user input

	size=strlen(string)-1; //size of user input, ignoring terminating character
	memcpy(buffer,string,size); //copy user input to buffer
	nbytes = sendPacket(sock,string,size,remote); //send user input to server
	if(strncmp(string,LS,strlen(LS))==0)//handle LS command
	{
		getDir(sock,remote);
	}
	else if(strncmp(string,EXIT,strlen(EXIT))==0)//handle EXIT command
		{
			exitFlag = exitServer(sock,remote);
		}
	else if(strncmp(string,GET,strlen(GET))==0)//handle GET command
	{
		char filename[sizeof(string)-strlen(GET)];
		bzero(filename,strlen(filename));
		strcpy(filename,string+4);//parse filename from user input
		downloadFile(sock,remote,buffer,filename);
	}
	else if(strncmp(string,PUT,strlen(GET))==0)//handle PUT command
	{
		sendFile(sock,remote,buffer);
	}
	else //print server response
	{
		nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
		printf("%s \n",buffer);
	}
}
//****************************************END MAIN LOOP *************************************************
printf("Good bye!\n");

return 0;
}





void getDir(int sock,struct sockaddr_in remote)
{
	int nbytes;
	int timeout;
	timeout = 0;
	char doneMessage[5] = "done"; //if server sends this, no more directory items
	char buf[MAXBUFSIZE];
	bzero(buf,MAXBUFSIZE);
	int remote_length=sizeof(remote);
	char ls[2] = "ls";
	nbytes = recvfrom(sock,buf,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
	while(nbytes<1)
	{
		sendPacket(sock,ls,2,remote); //if no incoming packets, resend
	}
	printf(">>>>Server Directory Contents<<<<\n");
	printf(">%s\n",buf); //print first line fo directory
	while(strncmp(doneMessage,buf,4)!=0) //check if done message has been sent
	{
		bzero(buf,MAXBUFSIZE);	
		nbytes = recvfrom(sock,buf,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
		printf(">%s\n"); //print subsequent directory items
	}
	printf(">>>>End of Directory<<<<\n");
	return;
}

int sendPacket(int sock, char* message, int length,struct sockaddr_in remote){
	time_t seconds;
	seconds = time(NULL);
	int nbytes;
	nbytes = sendto(sock, message, length, 0, (struct sockaddr*)&remote, sizeof(remote));
	return nbytes;
}

void displayMenu()
{
	printf("    Main Menu:\n");
	printf("> get <filename>\n");
	printf("> put <filename>\n");
	printf("> delete <filename>\n");
	printf("> ls\n");
	printf("> exit\n");
	return;
}

//sends exit message to server, waits for confirmation
//if timeout, assume server exited, ACK lost, and close client
int exitServer(int sock, struct sockaddr_in remote)
{
	int test;
	int nbytes;
	int timeOutCount=0;
	nbytes=-1;
	char buf[MAXBUFSIZE];
	int remote_length=sizeof(remote);
	bzero(buf,MAXBUFSIZE);
	nbytes = sendPacket(sock,EXIT,strlen(EXIT),remote);
	nbytes = recvfrom(sock,buf,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);//send exit message
	//keep sending exit while waiting for ACK
	//if no response, that means the server has exited, ACK lost
	while(strncmp(EXIT,buf,strlen(EXIT))!=0 && timeOutCount<10) 
	{
		timeOutCount+=1;
		printf("timeout, trying again\n");
		nbytes = sendPacket(sock,EXIT,strlen(EXIT),remote);
		nbytes = recvfrom(sock,buf,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
	}
	if(timeOutCount>9)
	{
		printf("No response from server, assume server exited\n");
	}
	else
	{
		printf("server exited, client now closing\n");
	}
	return 1;

}

//implements a simple send-and-wait 
//sender numbers packets, receiver compares to expected packet #
//receiver only ACKs when expected packets arrive
//slow, but reliable...
void downloadFile(int socket, struct sockaddr_in remote, char* buffer,char* filename)
{
	char buf[MAXBUFSIZE];
	memcpy(buf,buffer,MAXBUFSIZE);
	bzero(buffer,MAXBUFSIZE);
	int frameNumber=0; //appended/read from first 6 bytes of packet (up to 999,999 byte files are OK)
	int nbytes=0;
	int Sn = 0; //packet receiver is expecting
	int length = strlen(filename);
	int remote_length=sizeof remote;
	while(strncmp(buffer,GET,3)!=0) //looks for ACK from server to initiate file transfer, else requests transfer again
	{	
		sendPacket(socket,buffer,(sizeof GET) + length-1,remote);
		nbytes = recvfrom(socket,buffer,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
	}
	
	if((strncmp(buffer,"getFAIL",6)==0)) //if server responds that file cannot be opened
	{
		printf("Unable to download file\n");
		return;
	}
	filename[strlen(filename) - 1] = '\0';
	FILE *fp;
	fp = fopen(filename,"w+");//create file
	fclose(fp);
	fopen(filename,"r+");//open for appending, don't know why I did this, had a different plan at first
	char temp[10]; //holds file size sent from server
	char tempFrameHolder[6]; //holds packet number in every packet sent by server
	strcpy(temp,buffer+3); //gets filesize from ack sent by server
	int numberPackets = atoi(temp)/MAXBUFSIZE;
	if(atoi(temp)%MAXBUFSIZE!=0) numberPackets+=1; //if fileSize%chunksize !=0, we need one more packet to hold remaining data
	while(Sn < numberPackets) //until complete
	{
		bzero(tempFrameHolder,6);
		nbytes = recvfrom(socket,buffer,MAXBUFSIZE+6,0,(struct sockaddr*)&remote, &remote_length); //wait for packet, will timeout if not received
		memcpy(tempFrameHolder,buffer,6); // get packet sent
		if(atoi(tempFrameHolder)==Sn) //if correct frame, write to file, send ACK
		{
			fwrite(buffer+6,sizeof(char),nbytes-6,fp);
			sendPacket(socket,tempFrameHolder,6,remote); //ACK packet just received
			Sn+=1;
		}
		nbytes=0;
	}
	printf(">>>%s successfully downloaded<<<\n",filename);
	fclose(fp);
	return;
}

//stop-and wait
//sender puts packet number at beginning of packet
//if last packet sent was ACK'd, sends next packet
//else resends last packet
//client times out, so will not lock, unless server goes off-line completely, then will wait all day...
void sendFile(int sock, struct sockaddr_in remote, char* buffer)
{
	int remote_length = sizeof(remote);
	unsigned int bufferSizeTemp=0;
	char ACKbuffer[6];
	char temp[6];
	int nbytes = 0;
	int frameNumber=0;
	//int downloadComplete=0;
	//int Rn=0; //request number
	int Sn=0; //sequence number
	//int Sb=0; //sequence base
	
	int ACK=0; //last ACK received
	char filename[MAXBUFSIZE];
	strcpy(filename,buffer+4); //store filename here, for later use, maybe....
	//char* initMessage[MAXBUFSIZE]; // an ACK message contining file size
	//printf("in sendFile()\n");
	//sendPacket(sock,GET,3,remote); //send GET ACK
	
	FILE *fp; //file pointer to file to send
	size_t fileSize; //size of file to send
	fp = fopen(filename,"r"); // open file to send for reading only
	if (!fp)
	{
		printf("unable to open %s \n",filename);
		bzero(buffer,MAXBUFSIZE);
		strcpy(buffer,"getFAIL");
		sendPacket(sock,buffer,7,remote);
		return;
	}
	fseek(fp,0,SEEK_END); // go to end of file
	fileSize = ftell(fp); // and get file size
	rewind(fp); // go back to beginning of file
	int numberIterations = (fileSize/MAXBUFSIZE); // how many packets we have to send
	if(fileSize%MAXBUFSIZE!=0) numberIterations++; // add 1 for last packet if necessary
	bzero(buffer,MAXBUFSIZE);
	//construct ACK packet: "GET filesize". receiver can now determine # packets to be sent
	memcpy(buffer,GET,strlen(GET));
	bufferSizeTemp = strlen(GET);
	snprintf(buffer+strlen(GET),MAXBUFSIZE,"%d",fileSize);//store int in chars...inefficient...
	bufferSizeTemp+=11; 
	int bytecount=0;
	sendPacket(sock,buffer,bufferSizeTemp,remote); //sends ACK to requestor containing filesize
		while(Sn<numberIterations)
		{
				bzero(buffer,MAXBUFSIZE+6);
				bzero(ACKbuffer,6);
				snprintf(buffer,MAXBUFSIZE,"%d",Sn);//put packet # in first 6 bytes of packet
				fseek(fp,0,Sn*MAXBUFSIZE); //go to correct spot in file, not necessary since we are using stop-and-wait...
				if(Sn<numberIterations-1)
				{
					fread(buffer+6,sizeof(buffer[0]),MAXBUFSIZE,fp); // fill next 500 bytes with data
					sendPacket(sock,buffer,MAXBUFSIZE+6,remote); //5% of these messages are not delivered
				}
				else
				{
					fread(buffer+6,sizeof(buffer[0]),fileSize%MAXBUFSIZE,fp); // fill next 500 bytes with data
					sendPacket(sock,buffer,(fileSize%MAXBUFSIZE)+6,remote);
				}
				nbytes = recvfrom(sock,ACKbuffer,6,0,(struct sockaddr*)&remote, &remote_length); //wait for ACK
				if(strncmp(ACKbuffer,buffer,6)==0) //frame ACK'd was last frame sent
					{Sn+=1;} // if frame ack was OK, send next frame, else resend frame		
		}
	return;
}

