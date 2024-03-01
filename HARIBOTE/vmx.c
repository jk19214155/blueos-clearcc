#include "bootpack.h"
static unsigned int vmcs_id;
static unsigned long long ia32_vmx_basic_high,ia32_vmx_basic_low;
static void* vmxon_space;
extern char buff[1024];
void read_ia32_vmx_basic(unsigned long long* high,unsigned long long* low){
	io_rdmsr(0x480,high,low);
	return;
}
void write_ia32_vmx_basic(unsigned long long high,unsigned long long low){
	io_wrmsr(0x480,high,low);
	return;
}
EFI_STATUS vmm_init(){
	vmx_enable();
	return 0;
}
EFI_STATUS vmm_task(){
	BLUEOS_VMCS vmcs;
	struct TASK* task=task_now();
	struct MEMMAN* memman=task->memman;
	struct CONSOLE* cons=task->cons;
	cons_putstr0(cons, "BLUEOS VMM\n");
	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			int i = fifo32_get(&task->fifo);
			io_sti();
			if(i==4){
				break;
			}
		}
	}
	return 0;
}
void vmx_enable(){
	unsigned long long i=store_cr4();
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
