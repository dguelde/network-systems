#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
//#include <signal.h>
#include <stdio.h>
//#include <fcntl.h>
//#include <errno.h>
//#include <sys/time.h>
#include <stdlib.h>
//#include <memory.h>
#include <string.h>
#include <sys/poll.h>


#define BACKLOG 100
#define MAXBUFFERSIZE 9999

struct HeaderParts{
	char* requestMethod[256];
	char* URI[256];
	char* requestVersion[256];
	int sizeOfRequestMethod;
	int sizeOfURI;
	int sizeOfRequestVersion;
};

struct ConfigData{
	char* port[256];
	char* root[256];
	char* defaultWebPage[256];
	char* ContentType[10][256];
	int numberOfContentTypes;
	int timeout;
};


void readConf(struct ConfigData* configData);

void *get_in_addr(struct sockaddr *sa);

void handleRequest(int new_fd, struct ConfigData configData); //child process calls this to handle request

int parseHeader(char buffer[MAXBUFFERSIZE], struct HeaderParts* headerParts);

void processGet(int new_fd, struct HeaderParts headerParts, struct ConfigData configData);

void sendBadRequest(int new_fd,struct HeaderParts headerParts);

int contentTypeSupported(struct ConfigData configData, struct HeaderParts headerParts);

void sendBadHTTP(int new_fd,struct HeaderParts headerParts);

void sendNotSupported(int new_fd, struct HeaderParts headerParts);

void sendNotImplemented(int new_fd, struct HeaderParts headerParts);



int main()
{
	
	struct ConfigData configData;
	bzero(&configData,sizeof(configData));
	system("ipconfig getifaddr en0"); //print ip address to terminal for convenience...
	readConf(&configData); //read ws.conf, save in struct
	int sock;                           		//This will be our listening socket
	struct sockaddr_storage their_addr;     	//store info about incoming connections
	socklen_t addr_size;
    struct addrinfo hints, *res;				//used to gather and store info about server's address
    int new_fd;									//file descriptor for incoming connection
    char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints); 			//zero out hints
    hints.ai_family = AF_UNSPEC;  				//use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;			//TCP
    hints.ai_flags = AI_PASSIVE;     			//fill in my IP for me
    getaddrinfo(NULL, (char*)configData.port, &hints, &res); //fill res with server's IP address info
	if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol))==-1) // TCP socket
	if(sock<0)
	{
		printf("unable to create socket");
	}
	int yes=1;
	// lose the pesky "Address already in use" error message THANKS BEEJ!!
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	
	if (bind(sock, res->ai_addr, res->ai_addrlen) < 0)
	{
		printf("unable to bind socket\n");
		return -1;
	}
	printf("server: waiting for connections...\n");
	while(1) //server main loop
	{
		addr_size = sizeof(their_addr);	
		listen(sock, BACKLOG);
		new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);
		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		if(new_fd >=0) //connection received
		{
			if(fork() ==0) //child process handles request
			{
				close(sock); //child process does not use listening socket
				handleRequest(new_fd,configData);
				close(new_fd);
				exit(0); //child process closes after dealing with new connextion
			}
			else //parent process keeps listening on sock
			{
				close(new_fd); //parent does not use new_fd
			}
		}
	}
	return 0;
}


void handleRequest(int new_fd,struct ConfigData configData) //child process calls this to handle request
{
	int nbytes;
	char buffer[MAXBUFFERSIZE];
	struct HeaderParts headerParts;
	nbytes = recv(new_fd,buffer,MAXBUFFERSIZE,0); //read incomming message
	parseHeader(buffer,&headerParts); //parse first line for request type etc.
	if(strncmp((char*)headerParts.requestVersion,"HTTP/1.0",8)==0)
	{
		if(strcmp((char*)headerParts.requestMethod,"GET")==0) //its a GET requestMethod
					processGet(new_fd, headerParts, configData);
		else if(strcmp((char*)headerParts.requestMethod,"POST")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else if(strcmp((char*)headerParts.requestMethod,"HEAD")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else if(strcmp((char*)headerParts.requestMethod,"DELETE")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else if(strcmp((char*)headerParts.requestMethod,"OPTIONS")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else //bad request method
			sendBadRequest(new_fd,headerParts);
	}

	else if(strncmp((char*)headerParts.requestVersion,"HTTP/1.1",8)==0) //if http1.1, reuse socket
	{
		if(strcmp((char*)headerParts.requestMethod,"GET")==0) //its a GET requestMethod
			processGet(new_fd, headerParts, configData);
		else if(strcmp((char*)headerParts.requestMethod,"POST")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else if(strcmp((char*)headerParts.requestMethod,"HEAD")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else if(strcmp((char*)headerParts.requestMethod,"DELETE")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else if(strcmp((char*)headerParts.requestMethod,"OPTIONS")==0) //if method not supported
			sendNotImplemented(new_fd,headerParts);
		else //bad request method
			sendBadRequest(new_fd,headerParts);
		int timeOutValue = configData.timeout*1000;
		struct pollfd ufds[1];
		ufds[0].fd = new_fd;
		ufds[0].events = POLLIN;
		int timeOut=0;
		int counter=0;
		int bytesRead=0;
		int rv;
		while(!timeOut)
		{
			bzero(buffer,MAXBUFFERSIZE);
			nbytes=0;
			rv = poll(ufds,1,timeOutValue);
			if(rv==0) timeOut=1;
			else
			{
				bzero(buffer,MAXBUFFERSIZE);
				nbytes = recv(new_fd,buffer,MAXBUFFERSIZE,0); //read incomming message
				parseHeader(buffer,&headerParts); //parse first line for request type etc.
				if(strcmp((char*)headerParts.requestMethod,"GET")==0) //its a GET requestMethod
					processGet(new_fd, headerParts, configData);
				else if(strcmp((char*)headerParts.requestMethod,"POST")==0) //if method not supported
					sendNotImplemented(new_fd,headerParts);
				else if(strcmp((char*)headerParts.requestMethod,"HEAD")==0) //if method not supported
					sendNotImplemented(new_fd,headerParts);
				else if(strcmp((char*)headerParts.requestMethod,"DELETE")==0) //if method not supported
					sendNotImplemented(new_fd,headerParts);
				else if(strcmp((char*)headerParts.requestMethod,"OPTIONS")==0) //if method not supported
					sendNotImplemented(new_fd,headerParts);
				else //bad request method
					sendBadRequest(new_fd,headerParts);
			}
		counter+=1;
		}
	}
	else
	{
		sendBadHTTP(new_fd,headerParts);
	}
	return;
}


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int parseHeader(char buffer[MAXBUFFERSIZE], struct HeaderParts* headerParts)
{
	char* requestMethod;
	char* URI;
	char* requestVersion;
	requestMethod = strtok(buffer, " ");
	URI = strtok(NULL," ");
	requestVersion = strtok(NULL, "\r\n");
	if(strncmp(URI,"/\0",2)==0) //if no file specified
		{
			URI = "index.html\0"; //send index.html
		}
	else if(strncmp(URI,"/",1)==0) //get rid of leading '/', it breaks my 'open()' call
		{
			memmove(URI,URI+1,strlen(URI));
		}
	memcpy(headerParts->requestMethod,requestMethod,strlen(requestMethod)+1);
	headerParts->sizeOfRequestMethod = strlen(requestMethod);
	memcpy(headerParts->URI,URI,strlen(URI)+1);
	headerParts->sizeOfURI = strlen(URI);
	memcpy(headerParts->requestVersion,requestVersion,strlen(requestVersion)+1);
	headerParts->sizeOfRequestVersion = strlen(requestVersion);
	return 0;
}

void processGet(int new_fd,struct HeaderParts headerParts, struct ConfigData configData)
{
	int nbytes;	
	if(contentTypeSupported(configData,headerParts)==0)
	{
		sendNotSupported(new_fd,headerParts);
		return;
	}
	
	int bytesRead;
	FILE* inFile;
	char buffer[MAXBUFFERSIZE];
	char* filePath[strlen((char*)configData.root) + strlen((char*)headerParts.URI)];
	strcpy((char*)filePath, (char*)configData.root);
	strcat((char*)filePath,"/");
	strcat((char*)filePath,(char*)headerParts.URI);
	inFile = fopen((char*)filePath,"rb");
		if(inFile!=NULL)
		{
			fseek(inFile, 0, SEEK_END);
			unsigned long len = (unsigned long)ftell(inFile);
			rewind(inFile);
			char OKHeader[MAXBUFFERSIZE];
			if(strncmp((char*)headerParts.requestVersion,"HTTP/1.1",8)==0)
				sprintf(OKHeader,"%s 200 OK\nContent-Type: %s\nContent-Length %d\nConnection: keep-alive\n\n",(char*)headerParts.requestVersion,strrchr((char*)headerParts.URI,'.'),len);
			else if(strncmp((char*)headerParts.requestVersion,"HTTP/1.0",8)==0)
				sprintf(OKHeader,"%s 200 OK\nContent-Type: %s\nContent-Length %d\n\n",(char*)headerParts.requestVersion,strrchr((char*)headerParts.URI,'.'),len);
			//nbytes = send(new_fd, OKHeader, strlen(OKHeader), 0);
			write(new_fd,OKHeader,strlen(OKHeader));
			bytesRead=fread(buffer,1, MAXBUFFERSIZE, inFile);
			while  (bytesRead > 0)
						{
							//send (new_fd, buffer, bytesRead,0);
							write(new_fd,buffer,bytesRead);
							bytesRead=fread(buffer,1, MAXBUFFERSIZE, inFile);
						}
			fclose(inFile);
		}
		else 
		{
			char message404[MAXBUFFERSIZE];
			sprintf(message404,"%s 404 Not Found\n\n<html><body>404 Not Found Reason URL does not exist: %s </body></html>",(char*)headerParts.requestVersion,(char*)headerParts.URI);
			nbytes = send(new_fd, message404, strlen(message404), 0);
			//printf("%s failure %d\n",headerParts.URI,nbytes);
		}
	return;
}


void readConf(struct ConfigData* configData)
{
	FILE* inFile;
	char str[MAXBUFFERSIZE];
	inFile = fopen("ws.conf","r");
	if(!inFile) 
	{
		printf("cannot open ws.conf\n");
		exit(-1);
	}
	while(fgets(str,MAXBUFFERSIZE,inFile)!=NULL)
	{

		char temp[256];
		if(!strncmp(str,"#service port number",20))
		{
			bzero(temp,256);
			fgets(temp,256,inFile);
			memcpy(configData->port,temp+7,strlen(temp)-8);

		}
		else if(!strncmp(str,"#document root",14))
		{
			bzero(temp,256);
			fgets(temp,256,inFile);
			memcpy(configData->root,temp+16,strlen(temp)-20);
		}
		else if(!strncmp(str,"#default web page",17))
		{
			
			bzero(temp,256);
			fgets(temp,256,inFile);
			memcpy(configData->defaultWebPage,temp+15,strlen(temp)-16);
		}

		else if(strncmp(str,"#connection timeout",19)==0)
		{
			
			bzero(temp,256);
			fgets(temp,256,inFile);

			
			char* temp2 = strtok(temp," ");
			
			char* temp3 = strtok(NULL," ");
			
			char* temp4 = strtok(NULL,"\n");
			
			configData->timeout = atoi(temp4);
			
			
		}	
		else if(!strncmp(str,"#Content-Type",13))
		{

			bzero(temp,256);
			int counter=0;
			do
			{
				bzero(temp,256);
				fgets(temp,256,inFile);
				if(strncmp(temp,".",1)==0)
				{
					char* temp2 = strtok(temp," ");
					memcpy(configData->ContentType[counter],temp2,strlen(temp2));
					counter+=1;
				}}
			while(strncmp(temp,".",1)==0);

			configData->numberOfContentTypes = counter;
			fseek(inFile,-1*strlen(temp),SEEK_CUR);
		}

	}
	return;
}


void sendBadRequest(int new_fd,struct HeaderParts headerParts)
{
	char message400[MAXBUFFERSIZE];
	sprintf(message400,"HTTP/1.1 400 Bad Request\n\n<html><body>400 Bad Request Reason: Invalid Method: %s </body></html>",(char*)headerParts.requestMethod);
	int nbytes = send(new_fd, message400, strlen(message400), 0);
	return;
}

void sendBadHTTP(int new_fd,struct HeaderParts headerParts)
{
	char message400[MAXBUFFERSIZE];
	sprintf(message400,"HTTP/1.1 400 Bad Request\n\n<html><body>400 Bad Request Reason: Invalid HTTP-Version: %s </body></html>",(char*)headerParts.requestVersion);
	int nbytes = send(new_fd, message400, strlen(message400), 0);
	return;
}

int contentTypeSupported(struct ConfigData configData, struct HeaderParts headerParts)
{
	int i;
	int supported=0;
	char* fileType;
	fileType=strrchr((char*)headerParts.URI,'.');
	for(i=0;i<configData.numberOfContentTypes;i++)
	{
		if(strncmp(fileType,(char*)configData.ContentType[i],strlen(fileType))==0)
		{	
			supported=1;
		}
	}
	return supported;
}

void sendNotSupported(int new_fd, struct HeaderParts headerParts)
{
	char message400[MAXBUFFERSIZE];
	sprintf(message400,"%s 415 Unsupported Media Type\n\n<html><body>415 The file type of the web request is unsupported: %s </body></html>",(char*)headerParts.requestVersion,(char*)headerParts.URI);
	int nbytes = send(new_fd, message400, strlen(message400), 0);
	return;
}

void sendNotImplemented(int new_fd, struct HeaderParts headerParts)
{
	char message501[MAXBUFFERSIZE];
	sprintf(message501,"%s 501 Not Implemented\n<html><body>501 Not Implemented: %s </body></html>",(char*)headerParts.requestVersion,(char*)headerParts.requestMethod);
	int nbytes = send(new_fd, message501, strlen(message501), 0);
	return;
}




