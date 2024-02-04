#include<stdio.h>
#include<stdlib.h>
int main(int argc,char** argv){
	if(argc==1){
		printf("use: bin2asm infilename outfilename\n");
	}
	else if(argc!=3){
		printf("use: bin2asm infilename outfilename\n");
	}
	else{
		FILE* infile=fopen(argv[1],"rb");
		if(infile==NULL){
			printf("file open error!");
			return 1;
		}
		FILE* outfile=fopen(argv[2],"wb");
		if(outfile==NULL){
			printf("file open error!");
			return 1;
		}
		char byte;
		int i=0;
		while(1){
			byte=fgetc(infile);
			
			if(i<15){//一行16个字符
				if(i==0){
					fprintf(outfile,"db ");
				}
				fprintf(outfile,"0x%02X, ",(unsigned char)byte);
				i++;
			}
			else{
				fprintf(outfile,"0x%02X\n",(unsigned char)byte);
				i=0;
			}
			if(feof(infile)){
				break;
			}
		}
		fclose(infile);
		fclose(outfile);
	}
	return 0;
}