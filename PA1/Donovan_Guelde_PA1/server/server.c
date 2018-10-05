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
#include <dirent.h> // used to read directory for 'ls' menu command
#include <time.h>
/* You will have to modify the program below */
// starter code supplemented with stuff from beej's


#define MAXBUFSIZE 500
#define GREETING "greetings"
#define LS "ls"
#define GET "get"
#define DELETE "delete"
#define PUT "put"
#define EXIT "exit"
#define DONE "done"
#define SEND "send"


void sendDirectory( int sock, struct sockaddr_in remote);
int sendPacket(int sock, char* message, int length,struct sockaddr_in remote);
void downloadFile(int sock, struct sockaddr_in remote, char* buffer,char* filename);
void deleteFile(int sock, char filename[], struct sockaddr_in remote);
int exitNow(int sock, struct sockaddr_in remote);
void sendFile(int sock, struct sockaddr_in remote,char* buffer);

int main (int argc, char * argv[])
{
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine
	remote_length = sizeof(remote);
	//Causes the system to create a generic socket of type UDP (datagram)
	//if ((sock = **** CALL SOCKET() HERE TO CREATE UDP SOCKET ****) < 0)
	
	if((sock = socket(AF_INET, SOCK_DGRAM, 0))==-1) // UDP socket
	if(sock<0)
	{
		printf("unable to create socket");
	}
	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
		return -1;
	}

	
	
	
	//waits for an incoming message
	nbytes = recvfrom(sock, buffer, sizeof buffer,0,(struct sockaddr*)&remote, &remote_length);
	if(strncmp(buffer,GREETING,strlen(GREETING))==0) // if contact from server, send ack
	{
		printf("greeting received\n");
		nbytes = sendto(sock,GREETING,strlen(GREETING),0,(struct sockaddr*)&remote, remote_length);
	}
	buffer[nbytes]='\0';
	int exitFlag=0;
	while(!exitFlag)
//****************************************MAIN LOOP *************************************************
	{
		bzero(buffer,MAXBUFSIZE);
		//wait for message from client
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0,(struct sockaddr*)&remote, &remote_length);
		if(strncmp(buffer,EXIT,4)==0) // handle 'exit' message
			{
				exitFlag=exitNow(sock,remote);
			}
		
		else if (strncmp(buffer,LS,2)==0) // handle 'ls' message
			{
				sendDirectory(sock,remote);
			}
		else if (strncmp(buffer,DELETE,6)==0) // handle 'delete' message
		{
			char filename[MAXBUFSIZE];
			memcpy(filename,buffer+7,nbytes);
			deleteFile(sock,filename,remote);
		}
		else if (strncmp(buffer,GET,3)==0) // handle 'get' message
		{
			sendFile(sock,remote,buffer);
		}
		else if (strncmp(buffer,PUT,3)==0) // handle 'put' message
		{
			char filename[nbytes-strlen(PUT)+1];
			bzero(filename,strlen(filename));
			strcpy(filename,buffer+4);
			downloadFile(sock,remote,buffer,filename);
		}
		//if command not recognized, echo it back to client
		else nbytes = sendto(sock, buffer, nbytes, 0, (struct sockaddr*)&remote, sizeof(remote));
		bzero(buffer,MAXBUFSIZE);
	}
	//********************************************END MAIN LOOP *********************************
	close(sock);
	printf("Server exiting\n");
	return 0;
}


void sendDirectory(int sock, struct sockaddr_in remote)
{
	char doneMessage[5]="done"; //sent after all filenames are sent
	char buf[MAXBUFSIZE];
	FILE *fp;
	fp = popen("ls", "r"); //launches another thread and pipes the results of 'ls' to *fp
	while (fgets(buf, MAXBUFSIZE, fp) != NULL)
	{
		buf[strlen(buf)-1]='\0';
		sendPacket(sock,buf,strlen(buf),remote); //send filenames piped in by popen
		bzero(buf,MAXBUFSIZE);
	}
	sendPacket(sock,doneMessage,4,remote);
	pclose(fp); 
	return;
}

int sendPacket(int sock, char* message, int length,struct sockaddr_in remote)
{
int nbytes;
nbytes = sendto(sock, message, length, 0, (struct sockaddr*)&remote, sizeof(remote));
return nbytes;
}

void deleteFile(int sock, char filename[], struct sockaddr_in remote)
{
	int nbytes;
	nbytes = 0;
	char buffer[MAXBUFSIZE];
	if(remove(filename)==0) //try to delete, if successful
	{
		bzero(buffer,MAXBUFSIZE);
		memcpy(buffer,filename,nbytes);
		memcpy(buffer + nbytes, " deleted successfully\0",22);
		sendPacket(sock,buffer,nbytes+22,remote);
	}
	else //delete not successful
	{
		bzero(buffer,MAXBUFSIZE);
		memcpy(buffer,filename,nbytes);
		memcpy(buffer + nbytes, " could not be deleted\0",22);
		sendPacket(sock,buffer,nbytes+22,remote);
	}
	return;
}

int exitNow(int sock, struct sockaddr_in remote)
{	
	sendPacket(sock,EXIT,strlen(EXIT),remote); //positive ACK to tell client we have exited
	return -1;
}

void sendFile(int sock, struct sockaddr_in remote, char* buffer)
{
	int remote_length = sizeof(remote);
	unsigned int bufferSizeTemp=0;
	char ACKbuffer[6];
	char temp[6];
	int nbytes = 0;
	int frameNumber=0;
	int downloadComplete=0;
	int Rn=0; //request number
	int Sn=0; //sequence number
	int Sb=0; //sequence base
	int ACK=0; //last ACK received
	char filename[MAXBUFSIZE];
	strcpy(filename,buffer+4); //store filename here, for later use, maybe....
	char* initMessage[MAXBUFSIZE]; // an ACK message contining file size
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
	else
	{
		printf("%s opened\n",filename);
	}
	fseek(fp,0,SEEK_END); // go to end of file
	fileSize = ftell(fp); // and get file size
	rewind(fp); // go back to beginning of file
	printf("filesize is %lu\n",fileSize);
	int numberIterations = (fileSize/MAXBUFSIZE); // how many packets we have to send
	if(fileSize%MAXBUFSIZE!=0) numberIterations++; // add 1 for last packet if necessary
	printf("will require %d packets\n",numberIterations);
	bzero(buffer,MAXBUFSIZE);
	//construct ACK packet: "GET filesize". client can now determine # packets to be sent
	memcpy(buffer,GET,strlen(GET));
	bufferSizeTemp = strlen(GET);
	snprintf(buffer+strlen(GET),MAXBUFSIZE,"%d",fileSize);//store int in chars...inefficient...
	bufferSizeTemp+=11; 
	int bytecount=0;
	sendPacket(sock,buffer,bufferSizeTemp,remote);
		while(Sn<numberIterations)
		{
				bzero(buffer,MAXBUFSIZE+6);
				bzero(ACKbuffer,6);
				snprintf(buffer,MAXBUFSIZE,"%d",Sn);//put packet # in first 6 bytes of packet, can handle files up to 476MB
				fseek(fp,0,Sn*MAXBUFSIZE);
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

void downloadFile(int socket, struct sockaddr_in remote, char* buffer,char* filename)
{
	char buf[MAXBUFSIZE];
	memcpy(buf,buffer,MAXBUFSIZE);
	bzero(buffer,MAXBUFSIZE);
	int frameNumber=0;
	int nbytes;
	int Sn = 0;
	nbytes=0;
	int length;
	length = strlen(filename);
	int remote_length=sizeof remote;
	while(strncmp(buffer,GET,3)!=0)
	{	
		sendPacket(socket,buffer,(sizeof GET) + length-1,remote);
		nbytes = recvfrom(socket,buffer,MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
	}
	
	if((strncmp(buffer,"getFAIL",6)==0))
	{
		printf("Unable to download file\n");
		return;
	}
	FILE *fp;
	fp = fopen(filename,"w+");//create file
	fclose(fp);
	fopen(filename,"r+");//open for appending
	char temp[10];
	char tempFrameHolder[6];
	strcpy(temp,buffer+3);
	int numberPackets = atoi(temp)/MAXBUFSIZE;
	if(atoi(temp)%MAXBUFSIZE!=0) numberPackets+=1;
	while(Sn < numberPackets)
	{
		bzero(tempFrameHolder,6);
		nbytes = recvfrom(socket,buffer,MAXBUFSIZE+6,0,(struct sockaddr*)&remote, &remote_length);
		memcpy(tempFrameHolder,buffer,6); // get packet sent
		if(atoi(tempFrameHolder)==Sn) //if correct frame, send ACK
		{
			fwrite(buffer+6,sizeof(char),nbytes-6,fp);
			sendPacket(socket,tempFrameHolder,6,remote); //ACK packet just received if correct #
			Sn+=1;
		}
		nbytes=0;
	}
	fclose(fp);

	return;
}
