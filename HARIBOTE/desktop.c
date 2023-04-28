#include "bootpack.h"
#include <stdio.h>
#include <string.h>
struct TASK *desktop_start()
{
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
	task->tss.eip = ((int) &desktop_task)+((int)get_this());
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	task->task_sheet_max=8;//最大图层数量
	task->tss.cr3=0x268000;
	
	task->memman=memman;
	
	struct SHEET *sht=sheet_alloc(shtctl);
	(sht->flags)|=0x02;//图层锁定位生效
	sht->bxsize=shtctl->sheets[0]->bxsize;//图层的x大小为底板大小
	sht->bysize=shtctl->sheets[0]->bysize;//图层的y大小为底板大小
	sht->vx0=0;
	sht->vy0=0;
	sht->col_inv=0;
	sht->buf=memman_alloc_4k(memman,(sht->bxsize)*(sht->bysize));
	pageman_link_page_32_m(pageman,sht->buf,7,((sht->bxsize)*(sht->bysize)+0xfff)>>12,0);
	boxfill8(sht->buf, sht->bxsize, 15, 0,0,sht->bxsize-1,sht->bysize-1);//灰色底色
	sheet_updown(sht,1);//0是底板图层
	*((int *) (task->tss.esp + 4)) = (int) sht;
	//add_child_task(task_now0,task);//命令行是主进程的子进程
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}
struct SHEET* desktop_task(struct SHEET* sheet){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	//struct MEMMAN *memman = task_now()->memman;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char (*icon_buff)[32*32];//10个30*32大小的图标
	struct SHEET* icon_sheet[10];
	int i,j,k,l;
	char* s="123";
	struct TextView text0;
	struct TextView* text0_p=&text0;
	struct SHTCTL* shtctl=shtctl_init(memman,pageman, sheet->buf, sheet->bxsize, sheet->bysize);//图层控制器映射原始图层
	sheet->ctl_from=shtctl;//图层由控制器生成
	shtctl->vram4sht=sheet;//图层控制器的输出接入到了一个图层上
	shtctl->sheets0=sheet->ctl->sheets0;//复制申请数组
	shtctl->sheets0_size=sheet->ctl->sheets0_size;//复制申请数组大小
	struct SHEET *sheet_base=sheet_alloc(shtctl);//申请新的背景图层
	sheet_base->bxsize=sheet->bxsize;//复制横向宽度
	sheet_base->bysize=sheet->bysize;//复制纵向高度
	sheet_base->col_inv=-1;
	char* buff=memman_alloc_4k(memman,(sheet_base->bxsize)*(sheet_base->bysize));//新图层申请buff
	pageman_link_page_32_m(pageman,buff,7,((sheet_base->bxsize)*(sheet_base->bysize)+0xfff)>>12,0);
	
	sheet_refreshsub(shtctl->vram4sht->ctl, 0, 0, shtctl->vram4sht->ctl->xsize, shtctl->vram4sht->ctl->ysize, 0, shtctl->vram4sht->ctl->top);//刷新整个页面
	task_sleep(task_now());
	return sheet_base;
}