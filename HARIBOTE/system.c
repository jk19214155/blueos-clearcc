#include "bootpack.h"
#include <stdio.h>
#include <string.h>
int system_running=0;//系统运行标志
struct  task_abort *system_task_abort_list;//task回收队列
struct TASK *system_task;
void system_mainloop();
void* sheet_ctls[10];//10个桌面控制器
struct TASK* system_start(){
	if(system_running!=0){
		return system_task;
	}
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct PAGEMAN32 *pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	struct TASK *task = task_alloc(),*task_now0=task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);//图层控制器
	int i;
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,cons_fifo,7, 1,0);//
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);//内存分配!!!
	pageman_link_page_32_m(pageman,task->cons_stack,7,0x10,0);//
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = ((int) &system_mainloop)+((int)get_this());
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	task->task_sheet_max=8;//最大图层数量
	task->tss.cr3=0x268000;
	
	task->memman=memman;
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	
	system_running=1;//运行中
	system_task_abort_list=0;//没有要退出的程序
	system_task=task;
	return task;
}
void system_mainloop(){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = system_task;
	struct MEMMAN *memman = task_now()->memman;
	int a,b,c,d,e,i,j,k,l;
	for(;;){
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if(i==5){//关闭其他任务
				if(system_task_abort_list==0){
					continue;//没有要关闭的任务
				}
				struct task_abort *p=system_task_abort_list;
				for(;;){
					if(p==0){
						break;
					}
					else{
						struct TASK* task=p->task;//要终止的任务
						p=p->next;//指向下一个
						task_sleep(task);
						for(i=0;i<((int)(sizeof(struct MEMMAN))+0xfff)>>12;i++){
							void* po=pageman_unlink_page_32(pageman,(int)(task->memman)+0x1000*i,1);
							//memman_free_page_32(pageman,po);
						}
						memman_free_4k(memman,task->memman,sizeof(struct MEMMAN));//释放应用程序的内存控制器占用的内存
						for(i=0;i<(64 * 1024+0xfff)>>12;i++){
							void* po=pageman_unlink_page_32(pageman,(int)(task->cons_stack)+0x1000*i,1);
							//memman_free_page_32(pageman,po);
						}
						memman_free_4k(memman, task->cons_stack, 64 * 1024);//释放栈使用的内存
						for(i=0;i<(128 * 4+0xfff)>>12;i++){
							void* po=pageman_unlink_page_32(pageman,(int)(task->fifo.buf)+0x1000*i,1);
							//memman_free_page_32(pageman,po);
						}
						memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);//释放fifo使用的内存
						memman_free_page_32(pageman,(task->tss).cr3);//释放页表
						
						task->flags = 0; /* task_free(task); の代わり */
					}
				}
				system_task_abort_list=0;
			}
			if(i==6){
				
			}
		}
	}
}
void system_task_abort(struct task_abort* task){
	if(system_task_abort_list==0){
		system_task_abort_list=task;
		task->next=0;
	}
	else{
		struct task_abort* p=system_task_abort_list;
		for(p;p->next==0;p=p->next);
		p->next=task;
		task->next=0;
	}
}