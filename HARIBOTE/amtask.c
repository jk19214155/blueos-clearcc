#include "bootpack.h"
//本文件是高级多任务管理器
EFI_STATUS atask_alloc(void** task,unsigned int code_page_num,unsigned int data_page_num,unsigned int ring_num){
	*task=task_alloc();
	
	return 0;
}