#include "bootpack.h"
void init_sse42(){
	unsigned int cr4 = store_cr4();
	cr4|=(1<<9);//开启 OSFXSR 支持 FXRTOR FXSAVE 指令 支持SSE指令的执行
	cr4|=(1<<10);//开启 OSXMMEXCPT 之后 numeric 异常产生#XM异常而不是#UD异常
	load_cr4(cr4);
	unsigned int cr0= load_cr0();
	cr0|=(1<<1);//允许FWAIT WAIT 执行
	cr0&=~(1<<3);
	store_cr0(cr0);
	return;
}