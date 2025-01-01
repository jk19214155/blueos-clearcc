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
	// 设置虚拟机控制字段
	asm_vmwrite(0x4002, 0x0001); // 启用Pin-based VM-execution controls
	asm_vmwrite(0x4004, 0x0040); // 启用Primary processor-based VM-execution controls
	asm_vmwrite(0x401E, 0x0000); // 设置CR0 guest/host mask
	asm_vmwrite(0x4020, 0x0000); // 设置CR4 guest/host mask

	// 设置客户机状态
	asm_vmwrite(0x6800, 0x00000033); // 设置客户机CS选择子
	asm_vmwrite(0x6802, 0x00000000); // 设置客户机CS基址
	asm_vmwrite(0x4800, 0x0000FFFF); // 设置客户机CS限制
	asm_vmwrite(0x4802, 0x00009B00); // 设置客户机CS访问权限

	asm_vmwrite(0x681C, 0x00000000); // 设置客户机RIP
	asm_vmwrite(0x6820, 0x00000002); // 设置客户机RFLAGS

	// 设置主机状态
	asm_vmwrite(0x0C00, 0x00000010); // 设置主机CS选择子
	asm_vmwrite(0x6C00, (unsigned long long)vmx_exit_handler); // 设置VM-exit处理程序地址

	// 启动虚拟机
	asm_vmlaunch();

	// 如果执行到这里，说明VM启动失败
	cons_putstr0(task_now()->cons, "VM启动失败\n");
}
void vmx_exit_handler(){
    // 获取VM-Exit原因
    unsigned long long exit_reason;
    asm_vmread(0x4402, &exit_reason);
    
    // 检查是否是中断导致的VM-Exit
    if ((exit_reason & 0xFFFF) == 1) {  // 1表示外部中断
        unsigned long long interrupt_info;
        asm_vmread(0x4016, &interrupt_info);  // 读取VM-Exit中断信息
        
        unsigned char interrupt_vector = interrupt_info & 0xFF;
        unsigned char interrupt_type = (interrupt_info >> 8) & 0x7;
        
        if (interrupt_type == 3) {  // 3表示硬件异常
            // 对虚拟机内部发送这个中断
            asm_vmwrite(0x4400, interrupt_info);  // 写入VM-Entry中断信息
            asm_vmwrite(0x440C, 0);  // 清除VM-Entry异常错误码
        } else if(interrupt_type==1){
            // 处理外部中断
            switch (interrupt_vector) {
                case 32:  // 时钟中断
                    inthandler20(0);
                    break;
                case 33:  // 键盘中断
                    inthandler21(0);
                    break;
                case 44:  // 鼠标中断
                    inthandler2c(0);
                    break;
                case 0x34:  // 高精度定时器中断
                    inthandler34(0);
                    break;
                default:
                    // 其他中断类型，可以根据需要添加处理
                    break;
            }
        }
    } else {
        // 处理其他VM-Exit原因
        switch (exit_reason & 0xFFFF) {
            case 0:  // 异常或不可屏蔽中断
                cons_putstr0(task_now()->cons, "虚拟机发生异常或不可屏蔽中断\n");
                // 可以根据具体异常类型进行处理
                break;
            
            case 2:  // 三重故障
                cons_putstr0(task_now()->cons, "虚拟机发生三重故障，退出虚拟机\n");
                return;  // 退出虚拟机
            
            case 3:  // INIT信号
                cons_putstr0(task_now()->cons, "虚拟机收到INIT信号\n");
                // 可以选择重新初始化虚拟机或退出
                break;
            
            case 4:  // SIPI
                cons_putstr0(task_now()->cons, "虚拟机收到SIPI\n");
                // 处理启动处理器间中断
                break;
            
            case 10:  // CPUID指令执行
                // 模拟CPUID指令的执行
                // 这里需要根据具体需求来实现CPUID的模拟
                break;
            
            case 12:  // HLT指令执行
                cons_putstr0(task_now()->cons, "虚拟机执行HLT指令\n");
                // 可以选择暂停虚拟机或继续执行
                break;
            
            case 18:  // VMCALL指令执行
                cons_putstr0(task_now()->cons, "虚拟机执行VMCALL指令\n");
                // 处理虚拟机调用
                break;
            
            case 28:  // 控制寄存器访问
                // 处理对控制寄存器的访问
                break;
            
            case 30:  // I/O指令
                // 模拟I/O指令的执行
                break;
            
            case 31:  // RDMSR和WRMSR指令
                // 处理对MSR的读写
                break;
            
            default:
                cons_putstr0(task_now()->cons, "未处理的VM-Exit原因\n");
                break;
        }
    }
    
    // 进行其他必要的处理...
    
    // 重新进入虚拟机
    asm_vmresume();
    
    // 如果执行到这里，说明VM重入失败
    cons_putstr0(task_now()->cons, "VM重入失败\n");
	task_switch();
}