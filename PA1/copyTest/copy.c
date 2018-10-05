#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const int chunkSize=500;
int main()
{
	char* inFileName = "foo.jpg";
	char* outFileName = "fooCopy.jpg";
	FILE *fp_in;
	FILE *fp_out;
	size_t fileSize;
	unsigned char* buffer = malloc(chunkSize);

	fp_in = fopen(inFileName,"rb");
	fp_out = fopen(outFileName,"w+");
	if (!fp_in)
	{
		printf("unable to open input file\n");
		return -1;
	}
	else
	{
		printf("input file opened\n");
	}
	if (!fp_out)
	{
		printf("unable to open output file\n");
		return -1;
	}
	else
	{
		printf("output file created\n");
	}
	fclose(fp_out);



	fopen(outFileName,"r+");
	fseek(fp_in,0,SEEK_END);
	fileSize = ftell(fp_in);
	rewind(fp_in);
	printf("%lu\n",fileSize);
	int numberIterations = (fileSize/chunkSize);
	if(fileSize%chunkSize!=0) numberIterations++;
	printf("%d\n",numberIterations);
	int i;
	int areEqual;
	for(i=0;i<numberIterations-1;i=i+2)
	{
		//if(i==numberIterations-1 || i==1)
		//{
		//	printf("reading  %lu-%lu\n",fileSize - (fileSize - (i * chunkSize)),2*fileSize - (i+1)*chunkSize-1);
		//}
		fseek(fp_in,0,i*chunkSize);
		fread(buffer,sizeof(buffer[0]),chunkSize,fp_in);
		fseek(fp_out,0,i*chunkSize);
		fwrite(buffer,sizeof(buffer[0]),chunkSize,fp_out);
		
	}
	for(i=1;i<numberIterations-1;i=i+2)
	{
		//if(i==numberIterations-1 || i==1)
		//{
		//	printf("reading  %lu-%lu\n",fileSize - (fileSize - (i * chunkSize)),2*fileSize - (i+1)*chunkSize-1);
		//}
		fseek(fp_in,0,i*chunkSize);
		fread(buffer,sizeof(buffer[0]),chunkSize,fp_in);
		fseek(fp_out,0,i*chunkSize);
		fwrite(buffer,sizeof(buffer[0]),chunkSize,fp_out);
		
	}



	memset(buffer,0,chunkSize);
	fread(buffer,sizeof(buffer[0]),fileSize%chunkSize,fp_in);
	fwrite(buffer,sizeof(buffer[0]),fileSize%chunkSize,fp_out);
	fclose(fp_in);
	fclose(fp_out);
	



	return 0;
}