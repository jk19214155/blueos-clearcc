#include "bootpack.h"
void read_ia32_vmx_basic(unsigned int* high,unsigned int* low){
	rdmsr(0x480,high,low);
	return;
}
void write_ia32_vmx_basic(unsigned int high,unsigned int low){
	rdmsr(0x480,high,low);
	return;
}
void vmx_enable(){
	int i=store_cr4();
	i|=1<<13;//cr4.vmex
	load_cr4(i);
	int ia32_vmx_basic_high,ia32_vmx_basic_low;
	read_ia32_vmx_basic(&ia32_vmx_basic_high,&ia32_vmx_basic_low);
}