#include "bootpack.h"
void com_out_string(unsigned int port,char* str){
	int i;
	for(i=0;;i++){
		if(str[i]==0){
			break;
		}
		else{
			io_out8(port,str[i]);
		}
	}
	return;
}