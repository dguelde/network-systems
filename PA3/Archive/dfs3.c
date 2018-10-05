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
#include <dirent.h>

#define MAXBUFSIZE 99999
#define TIMEOUT 1
#define LIST "list"
#define GET "get"
#define PUT "put"
#define QUIT "quit"
#define BACKLOG 10
#define DFSNUM "DFS3"
typedef struct UserInfo{
	char* userName[32];
	char* password[32];
	char* root[32];
}UserInfo;

UserInfo* readConf(char* fileName);

void *get_in_addr(struct sockaddr *sa);

void handleRequest(int new_fd, UserInfo* userArray);

void processPUT(int new_fd, char* userName, char* fileName, char* subfolder,int chunkSize);

void processGET(int new_fd, char* userName, char* fullPath);

void processLIST(int new_fd, char* userName, char* fullPath);




int main(int argc, char const *argv[])
{
	
	UserInfo* userArray = readConf((char*)argv[1]);
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
    getaddrinfo(NULL, argv[2], &hints, &res); //fill res with server's IP address info
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
		printf("listening\n");
		new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);
		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		if(new_fd >=0) //connection received
		{
			if(fork() ==0) //child process handles request
			{
				close(sock); //child process does not use listening socket
				handleRequest(new_fd,userArray);
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

UserInfo* readConf(char* fileName)
{

	UserInfo *userArray = malloc(10*sizeof(UserInfo));
	FILE* inFile;
	char str[64];
	inFile = fopen(fileName,"r");
	if(!inFile) 
	{
		printf("cannot open %s\n",fileName);
		exit(-1);
	}
	else
	{
		int counter=0;
		while(fgets(str,64,inFile)!=NULL)
		{
			char* userName = strtok(str," ");
			char* password = strtok(NULL,"\n");
			char* root = (char*) malloc(strlen(userName)+1);
			strcpy(root,"/");
			strcat(root,userName);
			UserInfo newUser;
			memcpy(newUser.userName,userName,strlen(userName));
			memcpy(newUser.password,password,strlen(password));
			memcpy(newUser.root,root,strlen(root));
			userArray[counter]=newUser;
			counter+=1;
			memset(&newUser,0,sizeof(UserInfo));
		}
	}
	fclose(inFile);
	return userArray;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void handleRequest(int new_fd, UserInfo* userArray)
{
	int nbytes;
	char buffer[128];
	nbytes=0;
	bzero(buffer,128);
	nbytes = recv(new_fd,buffer,128,0); //read incomming message
	printf("received: >>%s<<\n",buffer);
	char* command = strtok(buffer,":");
	char* userName = strtok(NULL,":");
	char* password = strtok(NULL,":");
	
	int authorizedUser = 0;
	int authorizedPassword=0;
	for(int i = 0;i<10;i++)
	{
		if(!strncmp(userName,(char*)userArray[i].userName,strlen(userName)))
		{
			authorizedUser=1;
			if(!strncmp(password,(char*)userArray[i].password,strlen(password)))
			{
				authorizedPassword=1;
			}
		}
	}
	if(authorizedPassword+authorizedUser!=2) // bad userName/password
	{
		send(new_fd,(char*)"KO",2,0); //not ok...haha...
		
	}


	else //user,password OK, process request
	{
		if(!strncmp(PUT,command, strlen(command)))
		{
			char* fileName = strtok(NULL,":");
			char* subfolder = strtok(NULL,":");
			int chunkSize = atoi(strtok(NULL,"\n"));
			send(new_fd,(char*)"OK",2,0);
			processPUT(new_fd, userName, fileName, subfolder,chunkSize);

			bzero(buffer,128);
			nbytes = recv(new_fd,buffer,128,0); //read incomming message
			printf("received: >>%s<<\n",buffer);
			command = strtok(buffer,":");
			userName = strtok(NULL,":");
			password = strtok(NULL,":");
			fileName = strtok(NULL,":");
			subfolder = strtok(NULL,":");
			chunkSize = atoi(strtok(NULL,"\n"));
			send(new_fd,(char*)"OK",2,0);
			processPUT(new_fd, userName, fileName, subfolder,chunkSize);
			
		}

		if(!strncmp(GET,command, strlen(command)))
		{
			printf("in processGet main\n");
			char* fullPath = strtok(NULL,"\n");
			processGET(new_fd, userName, fullPath);

			bzero(buffer,128);
			nbytes = recv(new_fd,buffer,128,0); //read incomming message
			printf("received: >>%s<<\n",buffer);
			command = strtok(buffer,":");
			userName = strtok(NULL,":");
			password = strtok(NULL,":");
			fullPath = strtok(NULL,"\n");
			processGET(new_fd, userName, fullPath);

			bzero(buffer,128);
			nbytes = recv(new_fd,buffer,128,0); //read incomming message
			printf("received: >>%s<<\n",buffer);
			command = strtok(buffer,":");
			userName = strtok(NULL,":");
			password = strtok(NULL,":");
			fullPath = strtok(NULL,"\n");
			processGET(new_fd, userName, fullPath);

			bzero(buffer,128);
			nbytes = recv(new_fd,buffer,128,0); //read incomming message
			printf("received: >>%s<<\n",buffer);
			command = strtok(buffer,":");
			userName = strtok(NULL,":");
			password = strtok(NULL,":");
			fullPath = strtok(NULL,"\n");
			processGET(new_fd, userName, fullPath);	
		}

		if(!strncmp(LIST,command, strlen(command)))
		{
			char* subfolder = strtok(NULL,"\n");
			processLIST(new_fd, userName, subfolder);
		}

	}
}

void processPUT(int new_fd, char* userName, char* fileName, char* subfolder, int chunkSize)
{
	int nbytes = 1;
	int bytesReceived = 0;
	char* userPath = malloc(strlen(userName) + 7);
	sprintf(userPath,"./%s/%s",DFSNUM,userName);
	DIR* dir = opendir(userPath); //check for user folder
	if (dir)
	{
	    /* Directory exists. */
	    closedir(dir);
	}
	else if (ENOENT == errno)
	{
	    mkdir(userPath, 0777);
	}

	char* subDir = malloc(strlen(userName) + strlen(subfolder) + 2);
	sprintf(subDir,"%s/%s",userPath,subfolder);
	DIR* sub = opendir(subDir); //check for subfolder
	if (sub)
	{
	    /* Directory exists. */
	    closedir(sub);
	}
	else if (ENOENT == errno)
	{
	    mkdir(subDir, 0777);
	}
	FILE* fp;
	char* fullPath = malloc(128);
	bzero(fullPath,128);
	sprintf(fullPath,"%s/%s",subDir,fileName);
	printf("full file path = >>%s<<\n",fullPath);
	char* fileBuffer[MAXBUFSIZE];
	bzero(fileBuffer,MAXBUFSIZE);
	
	fp = fopen(fullPath,"ab");
	while(bytesReceived<chunkSize)
	{
		nbytes = recv(new_fd,fileBuffer,MAXBUFSIZE,0); //read incomming message
		fwrite(fileBuffer,1,nbytes,fp);
		bytesReceived+=nbytes;
		printf("nbytes = %d\n",nbytes);
		printf("bytesReceived = %d\n",bytesReceived);
	}
	send(new_fd,(char*)"OK",2,0);
	fclose(fp);

	
}

void processGET(int new_fd, char* userName, char* fullPath)
{
	int fileSize;
	char* buffer[MAXBUFSIZE];
	bzero(buffer,MAXBUFSIZE);
	int bytesRead;
	printf("in processGet\n");
	//char* readPath = malloc(128);
	char readPath[128];
	bzero(readPath,128);
	sprintf(readPath,"%s/%s%s",DFSNUM,userName,fullPath);
	printf("full file path = >>%s<<\n",readPath);
	FILE* fp = fopen(readPath,"rb");
	if(fp) // we have this chunk...
	{
		printf("got it\n");
		send(new_fd,(char*)"OK",2,0);
		fseek(fp,0,SEEK_END); // go to end of file
		fileSize = ftell(fp); // and get file size
		rewind(fp); // go back to beginning of file
		printf("%d\n",fileSize);
		//char* strSize = malloc(16);
		char strSize[16];
		bzero(strSize,16);
		sprintf(strSize,"%d",fileSize);
		//printf("%s size = %lu\n",strSize,strlen(strSize));
		send(new_fd,strSize, 16,0);
		
		bzero(buffer,MAXBUFSIZE);
		bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
		while  (bytesRead > 0)
		{
			//send (new_fd, buffer, bytesRead,0);
			printf("bytes read = >>%d<<\n",bytesRead);
			write(new_fd,buffer,bytesRead);
			bzero(buffer,MAXBUFSIZE);
			bytesRead=fread(buffer,sizeof(char), MAXBUFSIZE, fp);
		}
		//free(strSize);
	}
	else
	{
		send(new_fd,(char*)"KO",2,0); //ain't got it...
		//free(readPath);
	}
		
	
}

void processLIST(int new_fd, char* userName, char* fullPath) //gets directory of specified subfolder: User->myDir
{

	printf("In LIST\n");
	char messageBuffer[64];	
	char fileName[32];
	bzero(fileName,32);
	struct dirent *dir;
	DIR *directory;
	char subfolder[strlen(userName)+1+strlen(fullPath)+7];
	sprintf(subfolder,"./%s/%s/%s",DFSNUM,userName,fullPath);
	printf("%s\n",subfolder);
	
	if(directory = opendir(subfolder))
	{
		while ((dir = readdir(directory)) != NULL)
		{
			if (dir->d_type == DT_REG)
			{
				bzero(messageBuffer,64);
				sprintf (fileName, "%s", dir->d_name);
				sprintf(messageBuffer,"%s.%s",subfolder,fileName);
				printf("%s\n",messageBuffer);
				send(new_fd,messageBuffer,strlen(messageBuffer),0);
				recv(new_fd,messageBuffer,1,0); //send something to interrupt stream...
			}
		}
	}
	send(new_fd,(char*)"done",4,0);

}




