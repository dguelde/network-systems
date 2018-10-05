#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <time.h>
#include <pthread.h>

#define BACKLOG 100
#define MAXBUFFERSIZE 99999


// struct cachedIP{
// 	char hostIPPair[64];
	
// };

// struct cachedHosts;

// struct cachedHosts{
// 	char* name;
// 	struct hostent* host;
// 	struct cachedHosts* next;
// };

pthread_mutex_t lock;

struct BannedWebSites;

struct BannedWebSites{
	char name[64];
	struct BannedWebSites* next;
};

struct HeaderParts{
	char requestMethod[256];
	char  URI[256];
	char  requestVersion[256];
	char  path[256];
	char  host[256];
	char  buffer[MAXBUFFERSIZE];
	int sizeOfBuffer;
	int port;
	int sizeOfRequestMethod;
	int sizeOfURI;
	int sizeOfRequestVersion;
	int sizeOfPath;
	int sizeOfHost;
};

struct ConfigData{
	char  port[32];
	char  root[64];
	int numberOfBannedPages;
	struct BannedWebSites* bannedWebSites;
	//struct cachedHosts* cachedHostList;
	int timeout;
};
int isInCache(char* md5Hash, char* type, struct ConfigData configData);

void saveToDNSCache(struct hostent* host, char* md5Hash, struct ConfigData configData, struct HeaderParts* headerParts);

//void saveToPageCache(char* buffer, int bufferLength, char* md5Hash, struct ConfigData configData);

char *str2md5(const char *str, int length);

void send403(int new_fd,struct HeaderParts* headerParts);

void readConf(struct ConfigData* configData);

void *get_in_addr(struct sockaddr *sa);

void handleRequest(int new_fd,struct ConfigData configData); //child process calls this to handle request

int parseHeader(char buffer[MAXBUFFERSIZE], int sizeOfBuffer, struct HeaderParts* headerParts);

void processGet(int new_fd, struct HeaderParts* headerParts, struct ConfigData configData);

void sendBadRequest(int new_fd,struct HeaderParts* headerParts);

void sendBadHTTP(int new_fd,struct HeaderParts* headerParts);



int main(int argc, char const *argv[])
{
	int numberThreads=0;
	system("ipconfig getifaddr en0"); //print ip address to terminal for convenience...
	struct ConfigData configData;
	bzero(&configData,sizeof(struct ConfigData));
	memcpy(configData.port,argv[1],strlen(argv[1]));
	configData.bannedWebSites = NULL;
	//configData.port = argv[1];
	readConf(&configData);
	struct BannedWebSites* temp = configData.bannedWebSites;
	int sock;                           		//This will be our listening socket
	struct sockaddr_storage their_addr;     	//store info about incoming connections
	socklen_t addr_size;
    struct sockaddr_in proxy;
    memset(&proxy,0,sizeof(proxy));
    struct addrinfo hints, *res;				//used to gather and store info about server's address
    int new_fd;									//file descriptor for incoming connection
    char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints); 			//zero out hints
    hints.ai_family = AF_INET;  				//use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;			//TCP
    hints.ai_flags = AI_PASSIVE;     			//fill in my IP for me
    getaddrinfo(NULL, argv[1], &hints, &res); //fill res with server's IP address info
    //if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol))==-1) // TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
	if(sock<0)
	{
		printf("unable to create socket");
	}
	int yes=1;
	// lose the pesky "Address already in use" error message THANKS BEEJ!!
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));;
	if (bind(sock, res->ai_addr, res->ai_addrlen) < 0)
	{
		printf("unable to bind socket\n");
		return -1;
	}
	printf("server: waiting for connections...\n");
		
	struct timeval tv;
	tv.tv_sec = 10; 
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	addr_size = sizeof(their_addr);
	listen(sock, BACKLOG);
	while(1) ////////////////////////server main loop/////////////////////////
		{
			
			new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);
			setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
			setsockopt(new_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv,sizeof(struct timeval));
			inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
			if(new_fd >=0) //connection received, fork child to handle connection
			{
				numberThreads+=1;

				if(fork() ==0) //child process handles request
				{
					int timeout = 0;
					printf("************child %d *************\n",numberThreads);
					
					handleRequest(new_fd, configData);
					close(sock); //child process does not use listening socket
					close(new_fd);
					printf("************child %d closed*************\n",numberThreads);
					exit(0); //child process closes after dealing with new connextion
				}
				else //parent process keeps listening on sock
				{
					close(new_fd); //parent does not use new_fd
				}
			}



		} //////////////////////end main loop///////////////////////////////
	close(sock);
	return 0;
}


void *get_in_addr(struct sockaddr *sa)
{
	//printf("in get_in_addr\n");
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void handleRequest(int new_fd,struct ConfigData configData) //child process calls this to handle request
{
	pthread_mutex_lock(&lock);
	//printf("in handleRequest\n");
	int timeout = 0;
	int nbytes = 0;
	char buffer[MAXBUFFERSIZE];
	struct HeaderParts *headerParts = (struct HeaderParts*)malloc(sizeof(struct HeaderParts));
	bzero(headerParts,sizeof(struct HeaderParts));
	//accept(sock, (struct sockaddr *)&their_addr, &addr_size);
	nbytes = read(new_fd,buffer,MAXBUFFERSIZE); //read incomming message
	
		
		
		parseHeader(buffer, nbytes,headerParts); //parse first line for request type etc.
		//memcpy(headerParts.buffer,buffer,nbytes);
		//headerParts.sizeOfBuffer = nbytes;
		if((strncmp((char*)headerParts->requestVersion,"HTTP/1.0",8)==0) || (strncmp((char*)headerParts->requestVersion,"HTTP/1.1",8)==0))
		 {
			if(strcmp((char*)headerParts->requestMethod,"GET")==0) //its a GET requestMethod
				processGet(new_fd, headerParts, configData);	
			else //bad request method
				sendBadRequest(new_fd,headerParts);
		}
		
		 else
		 {
		 	sendBadHTTP(new_fd,headerParts);
		 }	
		 bzero(buffer,nbytes);

	close(new_fd);
	
	free(headerParts);
	pthread_mutex_unlock(&lock);
	return;
}

int parseHeader(char buffer[MAXBUFFERSIZE], int sizeOfBuffer, struct HeaderParts* headerParts)
{
	
	char requestMethod[16];
	char URI[MAXBUFFERSIZE];
	char requestVersion[32];
	bzero(&headerParts->buffer,MAXBUFFERSIZE);
	memcpy(&headerParts->buffer,buffer,sizeOfBuffer);
	headerParts->sizeOfBuffer = sizeOfBuffer;
	char* path;
	char* temp;
	char* temp3;
	char* temp2;
	char* temp4;
	char* host;
	int portSpecified=0;
	sscanf(buffer,"%s %s %s",requestMethod,URI,requestVersion);
	if(!strncmp(requestMethod,"GET",3))
	{
		
		for(int i=7;i<strlen(URI);i++)
		{
			if(URI[i]==':')
			{
				portSpecified=1;
			}
		}
		strcpy(temp2,URI);
		
		
		if(portSpecified==0)
		{
			//printf("temp2 = %s\n",temp2);
			temp=strtok(temp2,"//");
			host = strtok(NULL,"/");
			path = strtok(NULL,"****");
			headerParts->sizeOfHost = strlen(host);
			//printf("hostLength = %d\n",headerParts->sizeOfHost);
			//printf("host = >>%s<<\n",host);
			//printf("path = >>%s<<\n",path);
			memcpy(headerParts->host,host,strlen(host));
			if(path!=NULL)
				memcpy(headerParts->path,path,strlen(path));
			else
				memcpy(headerParts->path,(char*)"NULL",4);
			headerParts->port=80;
			
		}
		else
		{
			//printf("temp2 = %s\n",temp2);
			//temp3=strtok(temp2,"/");
			//printf("port specified\n");
			temp=strtok(temp2,"//");
			host=strtok(NULL,":");
			headerParts->sizeOfHost = strlen(host);
			printf("hostLength = %d\n",headerParts->sizeOfHost);
			char* newHost[32];
			memcpy(newHost,host+1,strlen(host)-1);
			//printf("newHost = >>%s<<\n",newHost);
			temp3 = strtok(NULL,"/");
			headerParts->port = atoi(temp3);
			//printf("host = %s\n",host);
			memcpy(headerParts->host,newHost,strlen((char*)newHost));
			path = strtok(NULL,"****");
			//printf("host = >>%s<<\n",host);
			//printf("path = >>%s<<\n",path);
			if(path!=NULL)
			{
				headerParts->sizeOfPath=strlen(path);
				memcpy(headerParts->path,path,strlen(path));
			}
			else
			{
				headerParts->sizeOfPath=0;
				memcpy(headerParts->path,(char*)"NULL",4);
			}
			//printf("temp2 = %s\n",temp2);
			//memcpy(headerParts->path,temp,strlen(temp));
			
		}

		memcpy(headerParts->requestMethod,requestMethod,strlen(requestMethod));
		memcpy(headerParts->requestVersion,requestVersion,strlen(requestVersion));
		memcpy(headerParts->URI,URI,strlen(URI));
		
	}
	else
	{
		memcpy(headerParts->requestMethod,requestMethod,strlen(requestMethod));	
		memcpy(headerParts->requestVersion,requestVersion,strlen(requestVersion));
		memcpy(headerParts->URI,URI,strlen(URI));
	}
	//printf("************************END PARSE HEADER***************************\n");

	return 0;
}

void sendBadRequest(int new_fd,struct HeaderParts* headerParts)
{
	printf("in sendBadRequest\n");
	//char* message400 = "ERROR 400 Bad Request\n\n";
	//sprintf(message400,"Error 400 Bad Request. Only GET supportes");
	char message400[] = "HTTP/1.0 400 Bad Request\n<html><body>400 Bad Request Reason: Unsupported Method, only GET supported</body></html>\n\n";
	//sprintf(message400,"HTTP/1.1 400 Bad Request\n\n<html><body>400 Bad Request Reason: Unsupported Method, only GET supported</body></html>\n\n");


	//int nbytes = send(new_fd, message400, strlen(message400), 0);
	//close(new_fd);
	return;
}

void sendBadHTTP(int new_fd,struct HeaderParts* headerParts)
{
	//printf("in sendBadHTTP\n");
	char message400[MAXBUFFERSIZE];
	bzero(message400,MAXBUFFERSIZE);
	sprintf(message400,"HTTP/1.0 400 Bad Request\n\n<html><body>400 Bad Request Reason: Invalid HTTP-Version: %s </body></html>\n\n",(char*)headerParts->requestVersion);
	//int nbytes = send(new_fd, message400, strlen(message400), 0);
	return;
}

void send403(int new_fd,struct HeaderParts* headerParts)
{
	//printf("in send403\n");
	char* message = "ERROR 403 Forbidden\n\n";
	//sprintf(message,"HTTP/1.1 400 Bad Request\n\n<html><body>400 Bad Request Reason: Invalid HTTP-Version: %s </body></html>",(char*)headerParts.requestVersion);
	int nbytes = send(new_fd, message, strlen(message), 0);
	//close(new_fd);
	return;
}


void readConf(struct ConfigData* configData)
{
	//configData->cachedHostList = NULL;
	FILE* inFile;
	char str[MAXBUFFERSIZE];
	inFile = fopen("ws.conf","r");
	if(!inFile) 
	{
		//printf("cannot open ws.conf\n");
		exit(-1);
	}
	while(fgets(str,MAXBUFFERSIZE,inFile)!=NULL)
	{

		char temp[256];
		bzero(temp,256);
		if(!strncmp(str,"#document root",14))
		{
			bzero(temp,256);
			fgets(temp,256,inFile);
			memcpy(configData->root,temp+16,strlen(temp)-20);
			//printf("%s\n",configData->root);
		}
		

		else if(strncmp(str,"#connection timeout",19)==0)
		{
			configData->timeout = atoi(str+20);	
		}	
		else if(!strncmp(str,"#Banned Web Pages",17))
		{
			bzero(temp,256);
			fgets(temp,256,inFile);
			int counter=0;
			do
			{
				if(strlen(temp)>1)
				{
					struct BannedWebSites* newBannedSite = (struct BannedWebSites*)malloc(sizeof(struct BannedWebSites));
					memcpy(newBannedSite->name,temp,strlen(temp)-1);
					newBannedSite->next = configData->bannedWebSites;
					configData->bannedWebSites = newBannedSite;
					counter+=1;
				}
			}
			while(fgets(temp,256,inFile)>0);
			configData->numberOfBannedPages = counter;
			fseek(inFile,-1*strlen(temp),SEEK_CUR);
		}
	}
	return;
}

void processGet(int new_fd,struct HeaderParts* headerParts, struct ConfigData configData)
{
	struct hostent* host;
	//char URI[strlen((char*)headerParts.URI)];
	int newSock,newSockfd;
	int nbytes=0;
	
	char buffer[MAXBUFFERSIZE];
	
	char hostTemp[32];
	bzero(hostTemp,32);
	memcpy(hostTemp,headerParts->host,headerParts->sizeOfHost);
	printf("%d\n",headerParts->sizeOfHost);
	char* pathTemp = (char*)headerParts->path;
	//memcpy(hostTemp,headerParts.host,headerParts.sizeOfHost);
	
	//printf("pathTemp = >>%s<<\n",pathTemp);
	//printf("hostTemp = >>%s<<\n",hostTemp);

	struct BannedWebSites* temp = configData.bannedWebSites;
	int isBanned=0;
	while(temp!=NULL)
	{
		if(!strncmp((char*)temp->name,(char*)headerParts->host,strlen((char*)temp->name)))
			isBanned=1;
		temp = temp->next;
	}
	if(isBanned==1)
	{
		send403(new_fd, headerParts);
		return;
	}
	
	
	//printf("Aasdf\n");
	
	
	//bzero(host,sizeof(host));
	
	struct sockaddr_in host_addr;

	host_addr.sin_port=htons(headerParts->port);
	host_addr.sin_family=AF_INET;
	//printf("realHost = %s\n",headerParts.host);
	host = gethostbyname((char*)headerParts->host);

	printf("B\n");
	
	bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length); //breaks
	printf("C\n");
	//char* hash = str2md5((char*)headerParts.host,strlen((char*)headerParts.host));
	newSock=socket(AF_INET,SOCK_STREAM,0);
	int connected = connect(newSock,(struct sockaddr*)&host_addr,sizeof(host_addr));
	//printf(buffer,"Connected to %s  IP - %s\n",headerParts.host,inet_ntoa(host_addr.sin_addr));
	if(connected<0)
		printf("Error in connecting to remote server\n");
	else
		printf("Connected to %s  IP - %s\n",headerParts->host,inet_ntoa(host_addr.sin_addr));
	
	
	// bzero(buffer,MAXBUFFERSIZE);
	// if(strncmp((char*)headerParts.path,"NULL",4)==0)
	// {
	// 	sprintf(buffer,"GET / HTTP/1.1\nHost: %s\nConnection: close\r\n\r\n",headerParts.host);	
		
	// }
	// else
	// {
	// 	sprintf(buffer,"GET /%s HTTP/1.1\nHost: %s\nConnection: close\r\n\r\n",pathTemp,headerParts.host);
		
	// }

	
	char* pageHash = str2md5(strcat((char*)headerParts->host,pathTemp),strlen((char*)headerParts->host) + strlen(pathTemp));
	if(!isInCache(pageHash,"WP",configData))
	{
		FILE* outFile;
		char* tempRoot = (char*)configData.root;
		printf("tempRoot = %s\n",tempRoot);

		char* filePath = strcat(strcat(tempRoot,pageHash),"WP");
		outFile = fopen(filePath,"a");
		// printf("sending new page\n");
		// printf("\n*************sending:\n>>%s<<\n******************\n",buffer);
		printf("**********%s***********\n",headerParts->buffer);
		nbytes = write(newSock,headerParts->buffer,headerParts->sizeOfBuffer);
		do
		{
			printf("nbytes = %d\n",nbytes);
			
			bzero((char*)buffer,MAXBUFFERSIZE);
			nbytes=read(newSock,buffer,MAXBUFFERSIZE);
			// printf("\n*************received:\n>>%s<<\n******************\n",buffer);
			if(nbytes>0)
			{
			fwrite(buffer,nbytes,1,outFile);
			}
			//saveToPageCache(buffer,nbytes,pageHash,configData);
			//printf("received buffer = >>%s<<\n",buffer);
			//printf("nbytes = %d\n",nbytes);
			if(nbytes>0)
			{
			write(new_fd,buffer,nbytes);
			}
			//printf("sent buffer = >>%s<<\n",buffer);
			
		}
		while(nbytes>0);
		fclose(outFile);
		close(newSock);
	}
	else //send cached page
	{
		printf("sending buffred page\n");
		FILE* fp;
		char* rootTemp = (char*)configData.root;
		char* filePath = strcat(strcat(rootTemp,pageHash),"WP");
		//printf("reading file %s\n",filePath);
		//char* filePath = strncat((char*)headerParts.host,(char*)headerParts.path,strlen((char*)headerParts.path));
		//char* fullPath = strncat((char*)configData.root,filePath,strlen(filePath));
		fp=fopen(filePath,"r");
		nbytes = fread(buffer,1 ,MAXBUFFERSIZE  ,fp);
		do
		{
			//printf("sending buffred page\n");
			//printf("sent buffer = >>%s<<\n",buffer);
			write(new_fd,buffer,nbytes);
			nbytes = fread(buffer,1,MAXBUFFERSIZE ,fp);
		}

		while(nbytes>0);
	}



}
	
char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
    char* returnValue = out;
    free(out);
    return returnValue;
}

void saveToDNSCache(struct hostent* host, char* md5Hash, struct ConfigData configData, struct HeaderParts* headerParts)
{	//bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
	//printf("saving DNS to cache\n");
	struct in_addr **addr_list;
	//int i;
	char ip[16];
	addr_list = (struct in_addr **) host->h_addr_list;
	char* h_addr;
	//printf("A\n");
	short h_length = host->h_length;
	
	//printf("B\n");
	//printf("h_length = %d\n",h_length);
	int i;
	for(i = 0; addr_list[i] != NULL; i++) //http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
	   {
	       //printf("%d\n",i);
	       //printf("%s\n",inet_ntoa(*addr_list[i]));
	       //strcpy(ip , inet_ntoa(*addr_list[i]) );
	       //printf("%s\n",ip);
	       break;
	   }
	//printf("%s\n",ip);
	//printf("saving: %s %d\n",h_addr,h_length);

	FILE* outFile;
	char* tempRoot = (char*)configData.root;
	char* filePath = strcat(strcat(tempRoot,md5Hash),"IP");
	outFile = fopen(filePath,"wb");
	fprintf(outFile,"%s 1\n",ip);
	//fprintf(outFile,"1\n");
	fclose(outFile);
	return;

}

// void saveToPageCache(char* buffer, int bufferLength, char* md5Hash, struct ConfigData configData)
// {
// 	FILE* outFile;
// 	char* tempRoot = (char*)configData.root;
// 	char* filePath = strcat(strcat(tempRoot,md5Hash),"WP");
// 	//printf("saving file %s\n",filePath);
// 	outFile = fopen(filePath,"a");
// 	fwrite(buffer,sizeof(char),bufferLength,outFile);
// 	fclose(outFile);
// 	return;
// }

int isInCache(char* md5Hash, char* type, struct ConfigData configData)
{

	int isInCache = 1;
	char* filename;
	char* junk1,*junk2;
	int modifiedDays, modifiedHours,modifiedMinutes,modifiedSeconds;
	char* trash1, *trash2;
	int currentDays,currentHours,currentMinutes, currentSeconds;
	if(!strncmp(type,"DNS",3))
	{
		char* tempRoot = (char*)configData.root;
		filename = strcat(strcat(tempRoot,md5Hash),"IP");
	}
	else
	{
		char* tempRoot = (char*)configData.root;
		filename = strcat(strcat(tempRoot,md5Hash),"WP");
		//printf("cache filename = %s\n",filename);
	}
	struct stat attr;

	if(stat(filename, &attr)!=0) //if file does not exist
	//return ( access( name.c_str(), F_OK ) != -1 );
	//if(access(filename,F_OK)!=-1)
	{
		// printf("not in cache\n");
		isInCache= 0;
	}
	else //if file exists, check time since last modification
	{
		//stat(filename, &attr);
		char* lastModifiedTime =ctime(&attr.st_mtime);
		junk1 = strtok(lastModifiedTime," ");
		junk2 = strtok(NULL," ");
		modifiedDays = atoi(strtok(NULL," "));
		modifiedHours = atoi(strtok(NULL,":"));
		modifiedMinutes = atoi(strtok(NULL,":"));
		modifiedSeconds = atoi(strtok(NULL," "));
		time_t rawtime;
	  	//struct tm * timeinfo;
	  	time ( &rawtime );
	  	char* timeinfo = asctime(localtime ( &rawtime ));
	  	trash1 = strtok(timeinfo," ");
		trash2 = strtok(NULL," ");
		currentDays = atoi(strtok(NULL," "));
		currentHours = atoi(strtok(NULL,":"));
		currentMinutes = atoi(strtok(NULL,":"));
		currentSeconds = atoi(strtok(NULL," "));
	  	int totalSecondsPassed = (currentDays-modifiedDays)*86400 + (currentHours - modifiedHours) * 3600 +
	  				(currentMinutes-modifiedMinutes)*60 + (currentSeconds-modifiedSeconds);
	  	//printf("cur min %d cur sec %d mod min %d mod sec %d\n",currentMinutes,currentSeconds,modifiedMinutes,modifiedSeconds);
	  	//printf("seconds passed = %d\n",totalSecondsPassed);
	  	if(!strncmp(type,"DNS",3))
	  	{
	  		if(totalSecondsPassed > 3600) //DNS lookups expire after 1 hour
	  		{
	  			remove(filename);
	  			isInCache=0;
	  		}
	  	}
	  	else //cahced web pages expire after 60 seconds
	  	{
		if(totalSecondsPassed > 60)
	  		{
	  			remove(filename);
	  			isInCache=0;
	  		}
	  	}
	}
	
  	//printf("inCache? %d\n",isInCache);
	return isInCache;
}
