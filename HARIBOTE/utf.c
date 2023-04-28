#include "bootpack.h"
unsigned int utf8_to_codepoint(unsigned char* str){
	if((str[0]>>4)==0x0f){//四字节
		char a1=(str[0]<<4)|(str[1]>>2)&0xf;
		char a2=
		char a3=
		char a4=
		str+=4;
		return 0;
	}
	else if((str[0]>>4)==0x0e){//三字节
		char a1=(str[0]<<4)|(str[1]>>2)&0xf;
		char a2=(str[1]<<6)|(str[2]&0x3f);
		str+=3;
		return (a1<<8)|a2;
	}
	else if((str[0]>>4)==0x0c){//二字节
		char a1=(str[0]<<4)|(str[1]>>2)&0xf;
		char a2=(str[1]<<6)|(str[2]&0x3f);
		str+=2;
		return (a1<<8)|a2;
	}
	else if(((str[0]>>7)==0x00)){//一字节
		str++;
		return str[-1];
	}
	else{
		return -1;
	}
}