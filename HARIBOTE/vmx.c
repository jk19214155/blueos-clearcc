#include "bootpack.h"
static unsigned int vmcs_id;
static unsigned int ia32_vmx_basic_high,ia32_vmx_basic_low;
static void* vmxon_space;
void read_ia32_vmx_basic(unsigned int* high,unsigned int* low){
	io_rdmsr(0x480,&high,&low);
	return;
}
void write_ia32_vmx_basic(unsigned int high,unsigned int low){
	io_wrmsr(0x480,high,low);
	return;
}
void vmx_enable(){
	int i=store_cr4();
	struct PAGEMAN32* pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	unsigned long long cr3=load_cr3();
	i|=1<<13;//cr4.vmex
	load_cr4(i);
	read_ia32_vmx_basic(&ia32_vmx_basic_high,&ia32_vmx_basic_low);
	vmcs_id=ia32_vmx_basic_low;
	vmxon_space=memman_alloc_page_32(pageman);
	*(unsigned int*)vmxon_space=vmcs_id;
	asm_vmxon(vmxon_space);
	return;
}
BLUEOS_VMCS* vmx_vm_alloc(){
	struct PAGEMAN32* pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	BLUEOS_VMCS* blueos_vmcs=memman_alloc_page_32(pageman);
	blueos_vmcs->vmcs=memman_alloc_page_32(pageman);
	blueos_vmcs->ept=memman_alloc_page_32(pageman);	
	*(unsigned int*)(blueos_vmcs->vmcs)=vmcs_id;
	return blueos_vmcs;
}
void vmx_vm_start(BLUEOS_VMCS* blueos_vmcs){
	asm_vmptrld(blueos_vmcs->vmcs);
	asm_vmwrite(0x2024,blueos_vmcs->ept);
	
}
