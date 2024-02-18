#include "bootpack.h"
void com_out_string(unsigned int port,char* str){
	int i;
	for(i=0;;i++){
		if(str[i]==0){
			break;
		}
		if(str[i]==0||((unsigned char)str[i])>=0x80){
			io_out8(port,'/');
			io_out8(port,'!');
			io_out8(port,'!');
			io_out8(port,'!');
			io_out8(port,'\n');
			break;
		}
		else{
			io_out8(port,str[i]);
		}
	}
	return;
}