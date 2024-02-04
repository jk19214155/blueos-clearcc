#include "bootpack.h"
unsigned int vmcs_id;
unsigned int ia32_vmx_basic_high,ia32_vmx_basic_low;
void read_ia32_vmx_basic(unsigned int* high,unsigned int* low){
	io_rdmsr(0x480,high,low);
	return;
}
void write_ia32_vmx_basic(unsigned int high,unsigned int low){
	io_wrmsr(0x480,high,low);
	return;
}
void vmx_enable(){
	int i=store_cr4();
	i|=1<<13;//cr4.vmex
	load_cr4(i);
	read_ia32_vmx_basic(&ia32_vmx_basic_high,&ia32_vmx_basic_low);
	vmcs_id=ia32_vmx_basic_low;
	
}
