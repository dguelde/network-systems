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
#include <openssl/md5.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFSIZE 99999
#define TIMEOUT 1
#define LIST "list"
#define GET "get"
#define PUT "put"
#define QUIT "quit"

typedef struct ConfigData{
	char* userName[30];
	char* password[30];
	char* dfs1Addr[15];
	int dfs1Port;
	char* dfs2Addr[15];
	int dfs2Port;
	char* dfs3Addr[15];
	int dfs3Port;
	char* dfs4Addr[15];
	int dfs4Port;
	
}ConfigData;

struct FilePieceNode;

struct FilePieceNode{
	char* fileName[16]; //filename
	char* directory[16];
	char* extension[3];
	int piece[4]; //indicate if piece is present on any server, 1 if present, 0 otherwise
	struct FilePieceNode* next; //a linked list, just because...
};

// typedef struct Servers{
// 	int servers[4];
// }Servers;

char* getFullPath(char* fileName, char* subfolder, ConfigData configData);

//downloads file from other system
void processGET(struct ConfigData configData, char* fileName,char* subfolder,int server);

//get dir of servers
void processLIST(char* subFolder,struct ConfigData configData);

//uploads file to servers
void processPUT(struct ConfigData configData,char* fileName,char* subfolder);

//display main menu every loop
void displayMenu();

int getMD5(char* fileName);

//parse config file, store in struct
void readConf(struct ConfigData* configData, char* fileName);

//set up sockets for all 4 servers, store in struct
int configureSocket(char* address, int portNumber);






int main (int argc, char * argv[]){

int nbytes;                             // number of bytes sent/received               
char buffer[128];				
struct ConfigData configData;
memset(&configData, 0, sizeof(configData));
readConf(&configData,argv[1]);
int exitFlag=0;

///////testing/////////




///////testing/////////


//****************************************MAIN LOOP *************************************************
while(!exitFlag)
{
	displayMenu();
	bzero(buffer,128); // clear buffer
	char string[128]; // place to take user input
	bzero(string,128);
	int size; //size of user input

	fgets(string,128,stdin);  // get user input

	size=strlen(string)-1; //size of user input, ignoring terminating character
	memcpy(buffer,string,size); //copy user input to buffer

	if(strncmp(string,LIST,strlen(LIST))==0)//handle LS command
	{
		char* subfolder;
		char* junk;
		junk = strtok(string," ");
		subfolder = strtok(NULL,"\n");
		processLIST(subfolder,configData);
	}
	else if(strncmp(string,QUIT,strlen(QUIT))==0)//handle EXIT command
		{
			exitFlag = 1;
		}
	else if(strncmp(string,GET,strlen(GET))==0)//handle GET command
	{
		char* subfolder;
		char* fileName;
		char* junk = strtok(string," ");
		fileName = strtok(NULL," ");
		subfolder = strtok(NULL,"\n");
		
		
		int server1 = configureSocket((char*)configData.dfs1Addr,configData.dfs1Port);
		processGET(configData,fileName,subfolder,server1);
		close(server1);
		
		int server2 = configureSocket((char*)configData.dfs2Addr,configData.dfs2Port);
		processGET(configData,fileName,subfolder,server2);
		close(server2);
		
		int server3 = configureSocket((char*)configData.dfs3Addr,configData.dfs3Port);
		processGET(configData,fileName,subfolder,server3);
		close(server3);
		
		int server4 = configureSocket((char*)configData.dfs4Addr,configData.dfs4Port);
		processGET(configData,fileName,subfolder,server4);
		close(server4);

		int incomplete=0;
		char buffer[256];
		bzero(buffer,256);
		
		FILE* outFile = fopen(fileName,"wb");
		FILE* fp1 = fopen("chunk1","rb");
		int n;
		int sum=0;
		if(fp1)
		{
			while((n = fread(buffer,sizeof(char),256,fp1)))
			{
				sum=sum+n;
				
				fwrite(buffer,sizeof(char),n,outFile);
			}
			//printf("at location %d\n",ftell(outFile));
			//printf("%d bytes copied\n",sum);
			fclose(fp1);
			bzero(buffer,256);
			//remove("chunk1");
		}
		else
		{
			printf("%s Incomplete\n",fileName);
			incomplete = 1;
		}
		sum=0;
		FILE* fp2 = fopen("chunk2","rb");
		if(fp2 && incomplete == 0)
		{
			while((n = fread(buffer,sizeof(char),256,fp2)))
			{
				sum=sum+n;
				
				fwrite(buffer,sizeof(char),n,outFile);
			}
			//printf("at location %d\n",ftell(outFile));
			//printf("%d bytes copied\n",sum);
			fclose(fp2);
			bzero(buffer,256);
			//remove("chunk2");
		}
		else if(incomplete == 0)
		{
			incomplete = 1;
			printf("%s Incomplete\n",fileName);
		}
		FILE* fp3 = fopen("chunk3","rb");
		if(fp3 && incomplete == 0)
		{	sum=0;
			while((n = fread(buffer,sizeof(char),256,fp3)))
			{
				sum=sum+n;
				
				fwrite(buffer,sizeof(char),n,outFile);
			}
			//printf("at location %d\n",ftell(outFile));
			//printf("%d bytes copied\n",sum);
			fclose(fp3);
			bzero(buffer,256);
			//remove("chunk3");
		}
		else if(incomplete == 0)
		{
			incomplete = 1;
			printf("%s Incomplete\n",fileName);
		}
		FILE* fp4 = fopen("chunk4","rb");
		if(fp4 && incomplete == 0)
		{
			sum=0;
			while(n = fread(buffer,sizeof(char),256,fp4))
			{
				sum=sum+n;
				
				fwrite(buffer,sizeof(char),n,outFile);
			}
			printf("at location %d",ftell(outFile));
			printf("%d bytes copied",sum);
			fclose(fp4);
			bzero(buffer,256);
			//remove("chunk4");
		}
		else if(incomplete == 0)
		{
			incomplete = 1;
			printf("%s Incomplete\n",fileName);

		}
		remove("chunk1");
		remove("chunk2");
		remove("chunk3");
		remove("chunk4");
		if(incomplete==1)
		{
			remove(fileName);
		}
	}


	else if(strncmp(string,PUT,strlen(GET))==0)//handle PUT command
	{
		char* subfolder;
		char* fileName;
		char* junk = strtok(string," ");
		fileName = strtok(NULL," ");
		subfolder = strtok(NULL,"\n");
		processPUT(configData,fileName,subfolder);
	}
	
}
//****************************************END MAIN LOOP *************************************************
printf("Good bye!\n");

return 0;
}





void processLIST(char* subFolder, ConfigData configData)
{

	int server1 = configureSocket((char*)configData.dfs1Addr,configData.dfs1Port);
	int server2 = configureSocket((char*)configData.dfs2Addr,configData.dfs2Port);
	int server3 = configureSocket((char*)configData.dfs3Addr,configData.dfs3Port);
	int server4 = configureSocket((char*)configData.dfs4Addr,configData.dfs4Port);

	int servers[4];
	servers[0] = server1;
	servers[1] = server2;
	servers[2] = server3;
	servers[3] = server4;


	//for poll()
	int rv;
	struct pollfd ufds[1];
	

	//printf("a\n");
	char messageBuffer[64]; //big enough...
	bzero(messageBuffer,64);
	struct FilePieceNode* root = NULL;
	//printf("b\n");
	struct FilePieceNode* temp = root;
	//*******************************server1
	//printf("server1\n");
	//int server1 = configureSocket((char*)configData.dfs1Addr,configData.dfs1Port);
	ufds[0].fd = server1;
	ufds[0].events = POLLIN; // check for normal data
	sprintf(messageBuffer,"list:%s:%s:%s",configData.userName,configData.password,subFolder);
	send(server1,messageBuffer,strlen(messageBuffer),0);
	int exitFlag=0;
	while(exitFlag!=1)
	{
		//printf("c\n");


		rv=poll(ufds,1,1000);
		if(rv>0)
		{
			recv(server1,messageBuffer,4,0);
			if(!strncmp((char*)messageBuffer,(char*)"done",4)) //server sends done when done
			{
				//printf("d\n");
				exitFlag=1;
			}
			else //server sends 'file' when has file info to send, then send full path (subdir, filename, piece number)
			{ //in format path.filename.type.piece
				//printf("e\n");

				char* path = malloc(16);
				char* fileName = malloc(16);
				char* extension = malloc(3);
				char* junk;
				int pieceNumber;
				bzero(messageBuffer,64);
				recv(server1,messageBuffer,64,0);
				//printf("%s\n",messageBuffer);



				path = strtok((char*)messageBuffer,"."); //first period
				fileName = strtok(NULL,".");
				extension = strtok(NULL,".");
				pieceNumber = atoi(strtok(NULL,"\n"));
				//printf("path >>%s<< filename >>%s<< extension >>%s<< piecenumber >>%d<<\n",path,fileName,extension,pieceNumber);
				//printf("g\n");
				// see if its in the liked list, if it is, add piece, else add to list
				temp=root;
				//printf("temp name >>%s<< fileName >>%s<<\n",(char*)temp->fileName,fileName);
				
				while(temp!=NULL && strcmp((char*)temp->fileName,(char*)fileName))
				{
					//printf("h\n");
					temp = temp->next;
				}
				if(temp==NULL) //new node
				{
					//printf("i\n");
					struct FilePieceNode* newNode = (struct FilePieceNode*)malloc(sizeof(struct FilePieceNode));
					//printf("j\n");
					bzero(&newNode->fileName,16);
					bzero(&newNode->directory,16);
					bzero(&newNode->extension,3);
					memcpy(&newNode->fileName,fileName,strlen(fileName)); //filename
					//printf("k\n");
					memcpy(&newNode->fileName+strlen(fileName),(char*)".",1); //filename.
					//printf("l\n");
					memcpy(&newNode->fileName+strlen(fileName+1),extension,strlen(extension));//filename.extension
					//printf("m\n");
					memcpy(&newNode->directory,path,strlen(path));
					//printf("n\n");
					memcpy(&newNode->extension,extension,strlen(extension));
					for (int i=0;i<4;i++)
					{
						newNode->piece[i] = 0;
						//printf("o\n");
					}
					//printf("p\n");
					newNode->piece[pieceNumber-1] = 1;
					//printf("q\n");
					newNode->next = root;
					//printf("r\n");
					root = newNode;
					//printf("s\n");
					//printf("root name >>%s<<\n",(char*)root->fileName);
				}
				else
				{
					//printf("sheeeit\n");
					temp->piece[pieceNumber-1]=1;
				}
				send(server1,(char*)"a",1,0);
			}
		}
		else 
			{
				exitFlag=1;

			}
	}
	//*******************************server2
	//printf("server2\n");

	ufds[0].fd = server2;
	ufds[0].events = POLLIN; // check for normal data
	

	sprintf(messageBuffer,"list:%s:%s:%s",configData.userName,configData.password,subFolder);
	send(server2,messageBuffer,strlen(messageBuffer),0);
	exitFlag=0;
	while(exitFlag!=1)
	{
	//	printf("c\n");


		rv=poll(ufds,1,1000);
		if(rv>0)
		{
			recv(server2,messageBuffer,4,0);
			if(!strncmp((char*)messageBuffer,(char*)"done",4)) //server sends done when done
			{
	//			printf("d\n");
				exitFlag=1;
			}
			else //server sends 'file' when has file info to send, then send full path (subdir, filename, piece number)
			{ //in format path.filename.type.piece
	//			printf("e\n");

				char* path = malloc(16);
				char* fileName = malloc(16);
				char* extension = malloc(3);
				char* junk;
				int pieceNumber;
				bzero(messageBuffer,64);
				recv(server2,messageBuffer,64,0);
	//			printf("%s\n",messageBuffer);



				path = strtok((char*)messageBuffer,"."); //first period
				fileName = strtok(NULL,".");
				extension = strtok(NULL,".");
				pieceNumber = atoi(strtok(NULL,"\n"));
	//			printf("path >>%s<< filename >>%s<< extension >>%s<< piecenumber >>%d<<\n",path,fileName,extension,pieceNumber);
	//			printf("g\n");
				// see if its in the liked list, if it is, add piece, else add to list
				temp=root;
	//			printf("temp name >>%s<< fileName >>%s<<\n",(char*)temp->fileName,fileName);
				while(temp!=NULL && strcmp((char*)temp->fileName,(char*)fileName))
				{
	//				printf("h\n");
					temp = temp->next;
				}
				if(temp==NULL) //new node
				{
	//				printf("i\n");
					struct FilePieceNode* newNode = (struct FilePieceNode*)malloc(sizeof(struct FilePieceNode));
	//				printf("j\n");
					bzero(&newNode->fileName,16);
					bzero(&newNode->directory,16);
					bzero(&newNode->extension,3);
					memcpy(&newNode->fileName,fileName,strlen(fileName)); //filename
	//				printf("k\n");
					memcpy(&newNode->fileName+strlen(fileName),(char*)".",1); //filename.
	//				printf("l\n");
					memcpy(&newNode->fileName+strlen(fileName+1),extension,strlen(extension));//filename.extension
	//				printf("m\n");
					memcpy(&newNode->directory,path,strlen(path));
	//				printf("n\n");
					memcpy(&newNode->extension,extension,strlen(extension));
					for (int i=0;i<4;i++)
					{
						newNode->piece[i] = 0;
	//					printf("o\n");
					}
	//				printf("p\n");
					newNode->piece[pieceNumber-1] = 1;
	//				printf("q\n");
					newNode->next = root;
	//				printf("r\n");
					root = newNode;
	//				printf("s\n");
	//				printf("root name >>%s<<\n",(char*)root->fileName);
				}
				else
				{
	//				printf("sheeeit\n");
					temp->piece[pieceNumber-1]=1;
				}
				send(server2,(char*)"a",1,0);
			}
		}
		else 
			{
				exitFlag=1;

			}
	}
	//*******************************server3
	//	printf("server3\n");
		ufds[0].fd = server3;
		ufds[0].events = POLLIN; // check for normal data
	
	sprintf(messageBuffer,"list:%s:%s:%s",configData.userName,configData.password,subFolder);
	send(server3,messageBuffer,strlen(messageBuffer),0);
	exitFlag=0;
	while(exitFlag!=1)
	{
	//	printf("c\n");


		rv=poll(ufds,1,1000);
		if(rv>0)
		{
			recv(server3,messageBuffer,4,0);
			if(!strncmp((char*)messageBuffer,(char*)"done",4)) //server sends done when done
			{
	//			printf("d\n");
				exitFlag=1;
			}
			else //server sends 'file' when has file info to send, then send full path (subdir, filename, piece number)
			{ //in format path.filename.type.piece
	//			printf("e\n");

				char* path = malloc(16);
				char* fileName = malloc(16);
				char* extension = malloc(3);
				char* junk;
				int pieceNumber;
				bzero(messageBuffer,64);
				recv(server3,messageBuffer,64,0);
	//			printf("%s\n",messageBuffer);



				path = strtok((char*)messageBuffer,"."); //first period
				fileName = strtok(NULL,".");
				extension = strtok(NULL,".");
				pieceNumber = atoi(strtok(NULL,"\n"));
	//			printf("path >>%s<< filename >>%s<< extension >>%s<< piecenumber >>%d<<\n",path,fileName,extension,pieceNumber);
	//			printf("g\n");
				// see if its in the liked list, if it is, add piece, else add to list
				temp=root;
	//			printf("temp name >>%s<< fileName >>%s<<\n",(char*)temp->fileName,fileName);
				while(temp!=NULL && strcmp((char*)temp->fileName,(char*)fileName))
				{
	//				printf("h\n");
					temp = temp->next;
				}
				if(temp==NULL) //new node
				{
	//				printf("i\n");
					struct FilePieceNode* newNode = (struct FilePieceNode*)malloc(sizeof(struct FilePieceNode));
	//				printf("j\n");
					bzero(&newNode->fileName,16);
					bzero(&newNode->directory,16);
					bzero(&newNode->extension,3);
					memcpy(&newNode->fileName,fileName,strlen(fileName)); //filename
	//				printf("k\n");
					memcpy(&newNode->fileName+strlen(fileName),(char*)".",1); //filename.
	//				printf("l\n");
					memcpy(&newNode->fileName+strlen(fileName+1),extension,strlen(extension));//filename.extension
	//				printf("m\n");
					memcpy(&newNode->directory,path,strlen(path));
	//				printf("n\n");
					memcpy(&newNode->extension,extension,strlen(extension));
					for (int i=0;i<4;i++)
					{
						newNode->piece[i] = 0;
	//					printf("o\n");
					}
	//				printf("p\n");
					newNode->piece[pieceNumber-1] = 1;
	//				printf("q\n");
					newNode->next = root;
	//				printf("r\n");
					root = newNode;
	//				printf("s\n");
	//				printf("root name >>%s<<\n",(char*)root->fileName);
				}
				else
				{
	//				printf("sheeeit\n");
					temp->piece[pieceNumber-1]=1;
				}
				send(server3,(char*)"a",1,0);
			}
		}
		else 
			{
				exitFlag=1;

			}
	}
	//*******************************server4
	//	printf("server4\n");
	
	ufds[0].fd = server4;
	ufds[0].events = POLLIN; // check for normal data
	sprintf(messageBuffer,"list:%s:%s:%s",configData.userName,configData.password,subFolder);
	send(server4,messageBuffer,strlen(messageBuffer),0);
	exitFlag=0;
	while(exitFlag!=1)
	{
	//	printf("c\n");


		rv=poll(ufds,1,1000);
		if(rv>0)
		{
			recv(server4,messageBuffer,4,0);
			if(!strncmp((char*)messageBuffer,(char*)"done",4)) //server sends done when done
			{
	//			printf("d\n");
				exitFlag=1;
			}
			else //server sends 'file' when has file info to send, then send full path (subdir, filename, piece number)
			{ //in format path.filename.type.piece
	//			printf("e\n");

				char* path = malloc(16);
				char* fileName = malloc(16);
				char* extension = malloc(3);
				char* junk;
				int pieceNumber;
				bzero(messageBuffer,64);
				recv(server4,messageBuffer,64,0);
	//			printf("%s\n",messageBuffer);



				path = strtok((char*)messageBuffer,"."); //first period
				fileName = strtok(NULL,".");
				extension = strtok(NULL,".");
				pieceNumber = atoi(strtok(NULL,"\n"));
	//			printf("path >>%s<< filename >>%s<< extension >>%s<< piecenumber >>%d<<\n",path,fileName,extension,pieceNumber);
	//			printf("g\n");
				// see if its in the liked list, if it is, add piece, else add to list
				temp=root;
				while(temp!=NULL && strcmp((char*)temp->fileName,(char*)fileName))
				{
	//				printf("h\n");
					temp = temp->next;
				}
				if(temp==NULL) //new node
				{
	//				printf("i\n");
					struct FilePieceNode* newNode = (struct FilePieceNode*)malloc(sizeof(struct FilePieceNode));
	//				printf("j\n");
					bzero(&newNode->fileName,16);
					bzero(&newNode->directory,16);
					bzero(&newNode->extension,3);
					memcpy(&newNode->fileName,fileName,strlen(fileName)); //filename
	//				printf("k\n");
					memcpy(&newNode->fileName+strlen(fileName),(char*)".",1); //filename.
	//				printf("l\n");
					memcpy(&newNode->fileName+strlen(fileName+1),extension,strlen(extension));//filename.extension
	//				printf("m\n");
					memcpy(&newNode->directory,path,strlen(path));
	//				printf("n\n");
					memcpy(&newNode->extension,extension,strlen(extension));
					for (int i=0;i<4;i++)
					{
						newNode->piece[i] = 0;
	//					printf("o\n");
					}
	//				printf("p\n");
					newNode->piece[pieceNumber-1] = 1;
	//				printf("q\n");
					newNode->next = root;
	//				printf("r\n");
					root = newNode;
	//				printf("s\n");
	///				printf("root name >>%s<<\n",(char*)root->fileName);
				}
				else
				{
		//			printf("sheeeit\n");
					temp->piece[pieceNumber-1]=1;
				}
				send(server4,(char*)"a",1,0);
			}
		}
		else 
			{
				exitFlag=1;

			}
	}
	//walk liknked list, print directory...
	temp = root;
	while(temp!=NULL)
	{
		//printf("pieces %d %d %d %d\n",temp->piece[0],temp->piece[1],temp->piece[2],temp->piece[3]);
		if(temp->piece[0] == 1 && temp->piece[1] == 1 && temp->piece[2]==1 && temp->piece[3]==1) //all present
		{
			printf("%s %s.%s\n",temp->directory,temp->fileName,temp->extension);
		}
		else
		{
			printf("%s %s.%s Incomplete\n",temp->directory,temp->fileName,temp->extension);
		}
		temp=temp->next;
	}
	return;
}



void displayMenu()
{
	printf("----Main Menu----\n");
	printf("> get <filename>\n");
	printf("> put <filename>\n");
	printf("> list\n");
	printf("> quit\n");
	return;
}

char* getFullPath(char* fileName, char* subfolder, ConfigData configData)
{
	if(strncmp((char*)subfolder,(char*)"/",1))
	{
		char* fullPath = malloc(strlen((char*)configData.userName) + strlen((char*)configData.password) + 2+ strlen((char*)subfolder) + strlen(fileName)+ 2+4);
		sprintf(fullPath,"get:%s:%s:/%s/%s",(char*)configData.userName,configData.password,subfolder,fileName);
		return fullPath;
	}
	else
	{
		char* fullPath = malloc(strlen((char*)configData.userName) + strlen((char*)configData.password) + 3 + strlen(fileName)+5);
		sprintf(fullPath,"get:%s:%s:/%s",(char*)configData.userName,configData.password,fileName);
		return fullPath;
	}
}






void readConf(ConfigData* configData, char* fileName) ///////////FOR PA3///////////////////
{
	//printf("in readConf\n");
	FILE* inFile;
	char str[256];
	inFile = fopen(fileName,"r");
	if(!inFile) 
	{
		printf("cannot open %s\n",fileName);
		exit(-1);
	}

	else
	{


		while(fgets(str,256,inFile)!=NULL)
		{

			if(!strncmp(str,"Server DFS1",11))
			{		
				char* junk = strtok(str," ");
				junk = strtok(NULL," ");
				char *address = strtok(NULL,":");
				memcpy(configData->dfs1Addr,address,strlen(address));
				char* strPortNum = strtok(NULL,"\n");
				sscanf(strPortNum,"%d",&configData->dfs1Port);
			}
			if(!strncmp(str,"Server DFS2",11))
			{
				char* junk = strtok(str," ");
				junk = strtok(NULL," ");
				char *address = strtok(NULL,":");
				memcpy(configData->dfs2Addr,address,strlen(address));
				char* strPortNum = strtok(NULL,"\n");
				sscanf(strPortNum,"%d",&configData->dfs2Port);

			}
			if(!strncmp(str,"Server DFS3",11))
			{
				char* junk = strtok(str," ");
				junk = strtok(NULL," ");
				char *address = strtok(NULL,":");
				//printf(">>%s<<\n",address);
				memcpy(configData->dfs3Addr,address,strlen(address));
				char* strPortNum = strtok(NULL,"\n");
				//printf(">>%s<<\n",strPortNum);
				sscanf(strPortNum,"%d",&configData->dfs3Port);

			}
			if(!strncmp(str,"Server DFS4",11))
			{
				char* junk = strtok(str," ");
				junk = strtok(NULL," ");
				char *address = strtok(NULL,":");
				//printf("%s\n",address);
				memcpy(configData->dfs4Addr,address,strlen(address));
				char* strPortNum = strtok(NULL,"\n");
				//printf("%s\n",strPortNum);
				sscanf(strPortNum,"%d",&configData->dfs4Port);
			}
			if(!strncmp(str,"Username",8))
			{
				memcpy(configData->userName,str+10,strlen(str)-11);
			}
			if(!strncmp(str,"Password",8))
			{
				memcpy(configData->password,str+10,strlen(str)-10);
			}
		}
	}
		
	return;
}

int configureSocket(char* address, int portNumber)
{
	int sock;                           		
	struct sockaddr_in remote;				//server's address info
	int remote_length=sizeof(remote);	
	struct timeval timeOut; //used for client timeout
	timeOut.tv_sec = 1; //should timeout after one second, but not consistent, don't know why not...
	timeOut.tv_usec = 0; //no microseconds
	memset((char *) &remote, 0, sizeof(remote)); //zero out struct
	remote.sin_family = AF_INET; //internet protocol
	remote.sin_port = htons(portNumber); //port specified in CLA by user
	if(inet_aton(address,&remote.sin_addr)==0) // put server IP into server's sockad_in struct
	{
		printf("inet_aton fail\n");
		return -1;
	}

	if ((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==-1) //open socket
	{
		printf("unable to open socket\n");
		return -1;
	}
	
	if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeOut, sizeof(struct timeval))<0)
	{
		printf("cannot set sockopt\n");
		return -1;
	}
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(struct timeval))<0)
	{
		printf("cannot set sockopt\n");
		return -1;
	}
	//fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	if(connect(sock,(struct sockaddr *)&remote, sizeof(remote)) < 0)
	{
		printf("cannot connect to %s:%d\n",address,portNumber);
		return -1;
	}
	return sock;
}







void processPUT(ConfigData configData,char* fileName,char* subfolder)
{

	int server1 = configureSocket((char*)configData.dfs1Addr,configData.dfs1Port);
	int server2 = configureSocket((char*)configData.dfs2Addr,configData.dfs2Port);
	int server3 = configureSocket((char*)configData.dfs3Addr,configData.dfs3Port);
	int server4 = configureSocket((char*)configData.dfs4Addr,configData.dfs4Port);

	int servers[4];
	servers[0] = server1;
	servers[1] = server2;
	servers[2] = server3;
	servers[3] = server4;
	


	
	//bzero(fileBuffer,MAXBUFSIZE);
	
	FILE *fp; //file pointer to file to send
	int fileSize; //size of file to send
	int chunkSize;
	int lastChunkSize;
	fp = fopen(fileName,"rb"); // open file to send for reading only
	if(!fp)
	{
		printf("unable to open %s \n",fileName);
	}
	else
	{
		//get filesize, read file to buffer
		int piece1Location1, piece2Location1, piece3Location1, piece4Location1;
		int piece1Location2, piece2Location2, piece3Location2, piece4Location2;
		char* buffer[MAXBUFSIZE];
		int bytesRead;
		int nbytes=0;
		fseek(fp,0,SEEK_END); // go to end of file
		fileSize = ftell(fp); // and get file size
		rewind(fp); // go back to beginning of file
		chunkSize = fileSize/4;
		lastChunkSize = fileSize - (chunkSize*3);
		char* fileBuffer = malloc(fileSize);
		bzero(fileBuffer,fileSize);
		fread(fileBuffer,sizeof(fileBuffer[0]),fileSize,fp); //assumes fileSize < MAXBUFSIZE
		fclose(fp);


		// set up poll()
		int rv;
		struct pollfd ufds[4];
		ufds[0].fd = servers[0];
		ufds[0].events = POLLIN; // check for normal data
		ufds[1].fd = servers[1];
		ufds[1].events = POLLIN; // check for just normal data
		ufds[2].fd = servers[2];
		ufds[2].events = POLLIN; // check for just normal data
		ufds[3].fd = servers[3];
		ufds[3].events = POLLIN; // check for just normal data


		//construct 'header' messages (userName:password:fileName:subfolder:size)
		char* fileName1 = malloc(64);
		snprintf(fileName1,strlen(fileName)+5,".%s.1",fileName);
		char* initializationMessage1 = malloc(128);
		bzero(initializationMessage1,128);
		snprintf(initializationMessage1,128,"put:%s:%s:%s:%s:%d",(char*)configData.userName,(char*)configData.password,fileName1,subfolder,chunkSize);

		char* fileName2 = malloc(64);
		snprintf(fileName2,strlen(fileName)+5,".%s.2",fileName);
		char* initializationMessage2 = malloc(128);
		bzero(initializationMessage2,128);
		snprintf(initializationMessage2,128,"put:%s:%s:%s:%s:%d",(char*)configData.userName,(char*)configData.password,fileName2,subfolder,chunkSize);

		char* fileName3 = malloc(64);
		snprintf(fileName3,strlen(fileName)+5,".%s.3",fileName);
		char* initializationMessage3 = malloc(128);
		bzero(initializationMessage3,128);
		snprintf(initializationMessage3,128,"put:%s:%s:%s:%s:%d",(char*)configData.userName,(char*)configData.password,fileName3,subfolder,chunkSize);

		char* fileName4 = malloc(64);
		snprintf(fileName4,strlen(fileName)+5,".%s.4",fileName);
		char* initializationMessage4 = malloc(128);
		bzero(initializationMessage4,128);
		snprintf(initializationMessage4,128,"put:%s:%s:%s:%s:%d",(char*)configData.userName,(char*)configData.password,fileName4,subfolder,lastChunkSize);
		




		
		

		//printf("filesize: %d\n",fileSize);
		//printf("filename: %s\n",fileName);
		unsigned int hash = getMD5(fileName);
		hash=hash%4;
		piece1Location1 = hash; // 0, 1, 2, 3
		piece1Location2 = (hash + 3)%4; // 3, 0, 1, 2
		piece2Location1 = (hash + 1)%4;  //1,2,3,0
		piece2Location2 = hash;// 0,1,2,3
		piece3Location1 = (hash + 2)%4; 
		piece3Location2 = (hash + 1)%4; //2,3,0,1
		piece4Location1 = (hash + 3)%4;
		piece4Location2 = (hash + 2)%4;
		//printf("hash = %d\n",hash);
		//printf("chunkSize = %d\n",chunkSize);
		//printf("last chunkSize = %d\n",lastChunkSize);

		//make temp files, easier to stream to TCP from file....
		char* firstChunk = malloc(chunkSize);
		bzero(firstChunk,chunkSize);
		memcpy(firstChunk,fileBuffer,chunkSize);

		FILE* fp = fopen("temp1","wb");
		fwrite(firstChunk,1,chunkSize,fp);
		fclose(fp);

		char* secondChunk = malloc(chunkSize);
		bzero(secondChunk,chunkSize);
		memcpy(secondChunk,fileBuffer+chunkSize,chunkSize);

		fp = fopen("temp2","wb");
		fwrite(secondChunk,1,chunkSize,fp);
		fclose(fp);

		char* thirdChunk = malloc(chunkSize);
		bzero(thirdChunk,chunkSize);
		memcpy(thirdChunk,fileBuffer+2*chunkSize,chunkSize);

		fp = fopen("temp3","wb");
		fwrite(thirdChunk,1,chunkSize,fp);
		fclose(fp);

		char* lastChunk = malloc(lastChunkSize);
		bzero(lastChunk,lastChunkSize);
		memcpy(lastChunk,fileBuffer+3*chunkSize,lastChunkSize);

		fp = fopen("temp4","wb");
		fwrite(lastChunk,1,lastChunkSize,fp);
		fclose(fp);
		
		char* confirmation[32];
		bzero(confirmation,32);
		
			
			//chunks 1 and 2
			nbytes = send(servers[piece1Location1],initializationMessage1,strlen(initializationMessage1),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece1Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece1Location1,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp1","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece1Location1],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				//write(servers.servers[0],(char*)"",0);
				recv(servers[piece1Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece1Location1,confirmation);
				fclose(fp);
			}
			nbytes = send(servers[piece2Location2],initializationMessage2,strlen(initializationMessage2),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece2Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece2Location2,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp2","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece2Location2],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				fclose(fp);
				recv(servers[piece2Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece2Location2,confirmation);
			}


			//chunks 2 and 3
			nbytes = send(servers[piece2Location1],initializationMessage2,strlen(initializationMessage2),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece2Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece2Location1,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp2","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece2Location1],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				write(servers[piece2Location1],(char*)"",0);
				recv(servers[piece2Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece2Location1,confirmation);
				fclose(fp);
			}
			nbytes = send(servers[piece3Location2],initializationMessage3,strlen(initializationMessage3),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece3Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece3Location2,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp3","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece3Location2],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				fclose(fp);
				recv(servers[piece3Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece3Location2,confirmation);
			}




			//chunks 3 and 4
			nbytes = send(servers[piece3Location1],initializationMessage3,strlen(initializationMessage3),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece3Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece3Location1,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp3","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece3Location1],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				//write(servers.servers[piece3Location1],(char*)"",0);
				recv(servers[piece3Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece3Location1,confirmation);
				fclose(fp);
			}
			nbytes = send(servers[piece4Location2],initializationMessage4,strlen(initializationMessage4),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece4Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece4Location2,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp4","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece4Location2],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				fclose(fp);
				recv(servers[piece4Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece4Location2,confirmation);
			}



			//chunks 4 and 1
			nbytes = send(servers[piece4Location1],initializationMessage4,strlen(initializationMessage4),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece4Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece4Location1,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp4","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece4Location1],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				//write(servers.servers[piece3Location1],(char*)"",0);
				recv(servers[piece4Location1],confirmation,32,0);
				//printf("server %d says %s\n",piece4Location1,confirmation);
				fclose(fp);
			}
			nbytes = send(servers[piece1Location2],initializationMessage1,strlen(initializationMessage1),0);
			bzero(confirmation,32);
			rv = poll(ufds,4,1000);
			if(rv > 0) //server is online
			{
				recv(servers[piece1Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece1Location2,confirmation);
				if(!strncmp((char*)confirmation,"KO",2))
				{
					printf("Invalid Username/Password. Please try again.\n");
					return;
				}
				bzero(confirmation,32);
				fp = fopen("temp1","rb");
				bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				while  (bytesRead > 0)
				{
					//send (new_fd, buffer, bytesRead,0);
					write(servers[piece1Location2],buffer,bytesRead);
					bytesRead=fread(buffer,1, MAXBUFSIZE, fp);
				}
				fclose(fp);
				recv(servers[piece1Location2],confirmation,32,0);
				//printf("server %d says %s\n",piece1Location2,confirmation);
			}
		
	}	
	remove("temp1");
	remove("temp2");
	remove("temp3");
	remove("temp4");
	close(servers[0]);
	close(servers[1]);
	close(servers[2]);
	close(servers[3]);
	//free(initializationMessage);
	return;
	
}

int getMD5(char* fileName)
{
	int mod = 0;
	FILE *pipe;
	char sysCall[64];
	strcpy(sysCall,"md5 ");
	strcat(sysCall, fileName);
	pipe = popen(sysCall, "r");
	unsigned long int md5Int=0;
		if (pipe != NULL)
		{
			char output[128];
			char* junk;
			char* md5;
			/* read the md5 digest string from the command output */
			fread(output, 1, sizeof output - 1, pipe);
			junk = strtok(output," ");
			junk = strtok(NULL," ");
			junk = strtok(NULL," ");
			md5 = strtok(NULL,"\n");
			int digit;
			char* character;
			character = md5+31;
			digit = strtol((char*)character,NULL,16);
			mod = digit % 4;
		}

		/* close the pipe */
		pclose(pipe);
	
	return mod;
}
void processGET(ConfigData configData, char* fileName, char* subfolder, int server)
{

	//set up poll()
	int rv;
	struct pollfd ufds[1];
	ufds[0].fd = server;
	ufds[0].events = POLLIN; // check for normal data
	
	char* fileBuffer[MAXBUFSIZE];
	int nbytes;
	//printf("%s\n",fileName);
	//printf("%s\n",subfolder);
	char* chunk1Name = malloc(strlen(fileName) + 4);
	char* chunk2Name = malloc(strlen(fileName) + 4);
	char* chunk3Name = malloc(strlen(fileName) + 4);
	char* chunk4Name = malloc(strlen(fileName) + 4);
	sprintf(chunk1Name,".%s.1",fileName);
	sprintf(chunk2Name,".%s.2",fileName);
	sprintf(chunk3Name,".%s.3",fileName);
	sprintf(chunk4Name,".%s.4",fileName);
	char* fullPathChunk1 = getFullPath(chunk1Name,subfolder,configData);
	char* fullPathChunk2 = getFullPath(chunk2Name,subfolder,configData);
	char* fullPathChunk3 = getFullPath(chunk3Name,subfolder,configData);
	char* fullPathChunk4 = getFullPath(chunk4Name,subfolder,configData);

	int serverDead = 0;
	
	//printf("getting chunk 1\n");
	//printf("%s\n",fullPathChunk1);
	nbytes = send(server,fullPathChunk1,strlen(fullPathChunk1),0); //send get request
	bzero(fileBuffer,MAXBUFSIZE);
	rv = poll(ufds,4,1000);
	if(rv>0)
	{
		nbytes = recv(server,fileBuffer,2,0); //OK or fail
		//printf("buffer = %s\n",fileBuffer);
		if(!strncmp((char*)fileBuffer,(char*)"OK",2)) //file incoming
		{
			nbytes = recv(server,fileBuffer,16,0); //receive file size
			FILE* fp = fopen("chunk1","wb");
			int bytesReceived=0;
			int chunkSize;
			chunkSize = atoi(strtok((char*)fileBuffer,"\n"));
			//printf("chunk size = %d\n",chunkSize);
			//printf("saving as: >>%s<<\n",fileName);
			bzero(fileBuffer,MAXBUFSIZE);
			while(bytesReceived<chunkSize)
			{
				nbytes = recv(server,fileBuffer,MAXBUFSIZE,0); //read incomming message
				fwrite(fileBuffer,sizeof(char),nbytes,fp);
				bytesReceived+=nbytes;
				//printf("nbytes = %d\n",nbytes);
				//printf("bytesReceived = %d\n",bytesReceived);
			}
			fclose(fp);
		}
		bzero(fileBuffer,MAXBUFSIZE);
		//printf("getting chunk 2\n");
		//printf("%s\n",fullPathChunk2);
		nbytes = send(server,fullPathChunk2,strlen(fullPathChunk2),0); //send get request
		nbytes = recv(server,fileBuffer,2,0); //OK or fail
		//printf("server = %d buffer = %s\n",server,fileBuffer);
		if(!strncmp((char*)fileBuffer,(char*)"OK",2)) //file incoming
		{
			bzero(fileBuffer,MAXBUFSIZE);
			nbytes = recv(server,fileBuffer,16,0); //receive file size
			FILE* fp = fopen("chunk2","wb");
			int bytesReceived=0;
			int chunkSize;
			chunkSize = atoi(strtok((char*)fileBuffer,"\n"));
			//printf("chunk size = %d\n",chunkSize);
			//printf("saving as: >>%s<<\n",fileName);
			bzero(fileBuffer,MAXBUFSIZE);
			while(bytesReceived<chunkSize)
			{
				nbytes = recv(server,fileBuffer,MAXBUFSIZE,0); //read incomming message
				fwrite(fileBuffer,sizeof(char),nbytes,fp);
				bytesReceived+=nbytes;
				//printf("nbytes = %d\n",nbytes);
				//printf("bytesReceived = %d\n",bytesReceived);
			}
			fclose(fp);
		}
		bzero(fileBuffer,MAXBUFSIZE);
		//printf("getting chunk 3\n");
		//printf("%s\n",fullPathChunk3);
		nbytes = send(server,fullPathChunk3,strlen(fullPathChunk3),0); //send get request
		nbytes = recv(server,fileBuffer,2,0); //OK or fail
		//printf("server = %d buffer = %s\n",server,fileBuffer);
		if(!strncmp((char*)fileBuffer,(char*)"OK",2)) //file incoming
		{
			bzero(fileBuffer,MAXBUFSIZE);
			nbytes = recv(server,fileBuffer,16,0); //receive file size
			FILE* fp = fopen("chunk3","wb");
			int bytesReceived=0;
			int chunkSize;
			chunkSize = atoi(strtok((char*)fileBuffer,"\n"));
			//printf("chunk size = %d\n",chunkSize);
			//printf("saving as: >>%s<<\n",fileName);
			while(bytesReceived<chunkSize)
			{
				nbytes = recv(server,fileBuffer,MAXBUFSIZE,0); //read incomming message
				fwrite(fileBuffer,1,nbytes,fp);
				bytesReceived+=nbytes;
				//printf("nbytes = %d\n",nbytes);
				//printf("bytesReceived = %d\n",bytesReceived);
			}
			fclose(fp);
		}
		bzero(fileBuffer,MAXBUFSIZE);
		//printf("getting chunk 4\n");
		//printf("%s\n",fullPathChunk2);
		nbytes = send(server,fullPathChunk4,strlen(fullPathChunk4),0); //send get request
		nbytes = recv(server,fileBuffer,2,0); //OK or fail
		//printf("server = %d buffer = %s\n",server,fileBuffer);
		if(!strncmp((char*)fileBuffer,(char*)"OK",2)) //file incoming
		{
			bzero(fileBuffer,MAXBUFSIZE);
			nbytes = recv(server,fileBuffer,16,0); //receive file size
			FILE* fp = fopen("chunk4","wb");
			int bytesReceived=0;
			int chunkSize;
			chunkSize = atoi(strtok((char*)fileBuffer,"\n"));
			//printf("chunk size = %d\n",chunkSize);
			//printf("saving as: >>%s<<\n",fileName);
			while(bytesReceived<chunkSize)
			{
				nbytes = recv(server,fileBuffer,MAXBUFSIZE,0); //read incomming message
				fwrite(fileBuffer,1,nbytes,fp);
				bytesReceived+=nbytes;
				//printf("nbytes = %d\n",nbytes);
				//printf("bytesReceived = %d\n",bytesReceived);
			}
			fclose(fp);
		}
		bzero(fileBuffer,MAXBUFSIZE);
	}

}




