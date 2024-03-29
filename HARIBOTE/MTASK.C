/* マルチタスク関係 */

#include "bootpack.h"

struct TASKCTL *taskctl=0;
struct TIMER *task_timer;
int zero_task_lock=0;//零号任务锁
unsigned task_ready=0;
extern char buff[1024];
struct TASKCTL *task_ctl_now(void){
	return taskctl;
}
struct TASK *task_now(void)
{
	if(task_ready!=0){
		struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
		return tl->tasks[tl->now];
	}
	else{
		return 0;
	}
}

void add_child_task(struct TASK* task_now0,struct TASK* task){
	//return;
	if(task_now0->child_task==0){//没有孩子进程
		task_now0->child_task=task;//直接代入
		//task->brother_task=task;//兄弟进程是自己
		task->father_task=task_now0;//父进程是当前进程
	}
	else{//有孩子进程
		struct TASK* p;
		//p是初始子进程，p的兄弟进程不是初始子进程，p指向下一个进程
		for(p=task_now0->child_task;(p->brother_task)!=(task_now0->child_task);p=p->brother_task);
		task->brother_task=p->brother_task;//task的兄弟进程是p的兄弟进程
		p->brother_task=task;//最后兄弟进程成为一个进程环
		task->father_task=task_now0;//父进程是当前进程
	}
	return;
}

struct TASK* find_child_task(struct TASK* task){//函数使用深度优先，用于在一个进程结束时，递归找出所有还在运行的子进程
	//return 0;
	if(task->child_task==0){
		return 0;//无子进程
	}
	else{
		struct TASK* p=task->child_task,*p0;
		p0=p;
		for(;;){
			if(p->child_task!=0){//有子进程进入子进程
				p=p->child_task;
				p0=p;
				continue;
			}
			else if(p->brother_task!=p0){//没有子进程就找兄弟进程
				p=p->brother_task;
				continue;
			}
			else{//当前任务环的兄弟进程都没有子进程
				return p;
			}
		}
	}
}

int unlink_task(struct TASK* task){
	//return 0;
	if(task->child_task!=0){//还有子进程
		//return -1;
		//考虑到可能有进程树移植的情况，因此这里不返回错误
	}
	else{
		struct TASK* p;
		for(p=task;p->brother_task!=task;p=p->brother_task);
		if(p==task){//环上只有自己
			p=p->father_task;
			p->child_task=0;//把自己移除
			return 0;
		}
		else{
			p->brother_task=task->brother_task;//重新连接环
			return 0;
		}
	}
}

void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2; /* 動作中 */
	return;
}

void task_remove(struct TASK *task)
{
	int i;
	struct TASK* p;
	struct TASKLEVEL *tl = &taskctl->level[task->level];
goto next;
	for(;;){
		p=find_child_task(task);
		if(p!=0){
			task_remove(p);//先关闭子进程
			unlink_task(p);
		}
		else{
			break;
		}
	}
next:
	/* 找到task的位置 */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* ここにいた */
			break;
		}
	}

	tl->running--;//运行中任务减少
	if (i < tl->now) {
		tl->now--; /* 移除的任务在当前任务之前，因此编号减少了 */
	}
	if (tl->now >= tl->running) {
		/* 当前运行标识符超过了任务数量则回到0 */
		tl->now = 0;
	}
	task->flags = 1; /* スリープ中 */

	/* 从被移除的任务后开始向前移动填补空位 */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void)
{
	int i;
	/* 一番上のレベルを探す */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* 見つかった */
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	unsigned long long cr3=load_cr3();
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	memman_link_page_64_m(pageman,cr3,taskctl,7,(sizeof (struct TASKCTL)+0xfff)>>12,0);//
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		//taskctl->tasks0[i].sel = TASK_GDT0  * 8;
		
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		//set_segmdesc(gdt + TASK_GDT0 + i, 103, ((unsigned long long) &taskctl->tasks0[i].tss)&0xffffffff, AR_TSS32);
		//set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 31, ((unsigned long long) taskctl->tasks0[i].ldt)&0xffffffff, AR_LDT);
		taskctl->tasks0[i].name="NONAME";//没有名字
	}
	set_segmdesc(gdt + TASK_GDT0 , 103, ((unsigned long long) &(taskctl->tss))&0xffffffff, AR_TSS32);
	set_segmdesc(gdt + TASK_GDT0 + 1, (((unsigned long long) &(taskctl->tss))>>32)&0xffff,(((unsigned long long) &(taskctl->tss))>>48)&0xffff , 0);
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}
	taskctl->id_high=0;
	taskctl->id_low=0;
	zero_task_lock=0;//放开锁
	task = task_alloc();
	task->tss.cr3= cr3;
	task->memman=memman;
	task->flags = 2;	/* 動作中マーク */
	task->priority = (timer_get_fps(1)/50)>=1?(timer_get_fps(1)/50):1; /* 0.02秒 */
	task->level = 0;	/* 最高レベル */
	task->name="mainloop";
	task_add(task);
	task_switchsub();	/* レベル設定 */
	load_tr(TASK_GDT0*8);
	task_timer = timer_alloc(1);
	timer_settime(1,task_timer, task->priority,0);
	idle = task_alloc();
	idle->tss.rsp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	memman_link_page_64_m(pageman,cr3,idle->tss.rsp- 64 * 1024,7,(64*1024+0xfff)>>12,0);//
	idle->tss.rip = ((unsigned long long) &task_idle)+((unsigned long long)get_this());
	idle->tss.cr3=cr3;
	idle->father_task=0;
	idle->child_task=0;
	idle->brother_task=idle;
	idle->name="idle";
	task_start(idle);
	task_run(idle, MAX_TASKLEVELS - 1, 1);
	task_ready=1;//任务环境已经准备好
	add_child_task(task,idle);
	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* 使用中マーク */
			task->tss.rflages = 0x00000202; /* IF = 1; */
			task->tss.rip=0;
			task->tss.rbx=0;
			task->tss.rsi=0;
			task->tss.rdi=0;
			task->tss.rbp=0;
			task->tss.ldtr=0;
			task->tss.rsp=0;
			task->tss.iomap = 0x40000000;
			//void* p=memman_alloc_page_32(pageman);
			//task->tss.cr3= 0x00268000;//暂时使用os的页表
			task->tss.cr3=0;//动态申请
			task->father_task=0;
			task->child_task=0;
			task->brother_task=task;//兄弟是自己
			task->index=0;
			task->taskctl=0;
			task->timerctl=0;
			task->shtctl=0;
			task->id_high=taskctl->id_high++;//注册id
			task->id_low=taskctl->id_low++;
			task->msg_box.flag=0;//不使能消息接收
			task->msg_box.msg=0;
			task->mem_use=0;//内存占用计数器
			task->root_dir_addr=0;
			task->fifo32_mouse_event=0;//不监听鼠标
			//task->fifo_mouse_updown_listen_num=0;//不监听鼠标事件
			task->flags_a=0;
			return task;
		}
	}
	return 0; /* もう全部使用中 */
}

unsigned int task_get_esp0(){//这个函数执行后 EAX会保存esp0的值 注意执行完成本函数后执行非函数调用任意其他语句都可能导致EAX发生变化
	return task_now()->tss.rsp0;
}

void task_start(struct TASK *task){
	unsigned long long cr3=load_cr3();
	//切换到目标进程的cr3
	store_cr3(task->tss.cr3);
	unsigned long long* esp=(task->tss.rsp);
	sprintf(buff,"task_start:rsp=%x\n",task->tss.rsp);
	com_out_string(0x3f8,buff);
	*(--esp)=(task->tss.rip);
	*(--esp)=(task->tss.rbx);
	*(--esp)=(task->tss.rsi);
	*(--esp)=(task->tss.rdi);
	*(--esp)=(task->tss.rbp);
	*(--esp)=(task->tss.cr3);
	*(--esp)=(task->tss.rflages);
	*(--esp)=(task->sel);
	*(--esp)=(task->tss.ldtr);
	*(--esp)=0x1212;//r12
	*(--esp)=0x1313;//r13
	*(--esp)=0x1414;//r14
	*(--esp)=0x1515;//r15
	task->tss.rsp=esp;
	store_cr3(cr3);
	return;
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* レベルを変更しない */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* 動作中のレベルの変更 */
		task_remove(task); /* これを実行するとflagsは1になるので下のifも実行される */
	}
	if (task->flags != 2) {
		/* スリープから起こされる場合 */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* 次回タスクスイッチのときにレベルを見直す */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	sprintf(buff,"task_sleep:start task=%s\n",task->name);
	com_out_string(0x3f8,buff);
	if (task->flags == 2) {
		/*正在运行*/
		now_task = task_now();
		task_remove(task); /* これを実行するとflagsは1になる */
		if (task == now_task) {
			/* 自分自身のスリープだったので、タスクスイッチが必要 */
			task_switchsub();
			now_task = task_now(); /* 設定後での、「現在のタスク」を教えてもらう */
			//farjmp(0, now_task->sel);
			//com_out_string(0x3f8,"task_sleep: task==now_task\n");
			sprintf(buff,"task_sleep:now task=%s new task=%s\n",task->name,now_task->name);
			com_out_string(0x3f8,buff);
			now_task->flags_a&=~(1<<2);//发生了协同切换
			struct TASK* task_a=&taskctl->tasks0[0];
			task->tss.rsp0=taskctl->tss.rsp0;
			task->tss.rsp1=taskctl->tss.rsp1;
			task->tss.rsp2=taskctl->tss.rsp2;

			
			taskctl->tss.rsp0=now_task->tss.rsp0;
			taskctl->tss.rsp1=now_task->tss.rsp1;
			taskctl->tss.rsp2=now_task->tss.rsp2;
			asm_task_switch32(&(task->tss.rsp),&(now_task->tss.rsp));
		}
	}
	//com_out_string(0x3f8,"task_sleep: end\n");
	return;
}

void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	struct TSS32* old_tss32;
	//old_tss32=&((t1->task[t1->now]).TSS32);
	tl->now++;//切?到下一个任?
	if (tl->now == tl->running) {//如果切?到?尾?返回??
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(1,task_timer, new_task->priority,0);
	if (new_task != now_task) {
		//com_out_string(0x3f8,"task switch ok\n");
		now_task->flags_a|=(1<<2);//发生了抢占切换
		struct TASK* task_a=&taskctl->tasks0[0];
		now_task->tss.rsp0=taskctl->tss.rsp0;
		now_task->tss.rsp1=taskctl->tss.rsp1;
		now_task->tss.rsp2=taskctl->tss.rsp2;
		
		taskctl->tss.rsp0=new_task->tss.rsp0;
		taskctl->tss.rsp1=new_task->tss.rsp1;
		taskctl->tss.rsp2=new_task->tss.rsp2;
		sprintf(buff,"task_switch:now fromtask =%s to=%s\n",now_task->name,new_task->name);
		com_out_string(0x3f8,buff);
		
		asm_task_switch32(&(now_task->tss.rsp),&(new_task->tss.rsp));
		//task_switch32(old_tss32,&(new_task->tss));
	}
	if((new_task->flags_a)&(1<<3)==1){//当前任务待摧毁
		if((new_task->flags_a)&(1<<2)==0){//没有置位标志 是协调切换
			fifo32_put(&new_task->fifo,4);//结束任务
		}
		else{//是抢占切换
			task_get_esp0();
			asm_end_app();
		}
	}
	return;
}
void task_lanch(int num){
	
}
void task_get_index(){
	
}
TASK_MSG* task_get_msg(struct TASK *task){
	if(task->msg_box.msg==0){//没有消息
		return -1;
	}
	else{
		TASK_MSG* msg=task->msg_box.msg;//获取消息
		task->msg_box.num--;//消息减少
		task->msg_box.msg=(task->msg_box.msg)->next;//移动到下一个
		return msg;//返回消息
	}
}
unsigned int task_send_msg(struct TASK *task_sender,struct TASK *task_geter,TASK_MSG* msg){
	if(((task_geter->msg_box).flag)&1!=0){//开启了消息接收
		msg->sender_pid=task_sender->id_low;//注册发送者id
		msg->sender_pid=task_geter->id_low;//注册接收者id
		msg->next=task_geter->msg_box.msg;//新增消息的下一个是列表
		task_geter->msg_box.msg=msg;//列表的开头是新消息
		struct FIFO32 *fifo=&(task_geter->fifo);
		fifo32_put(fifo, task_geter->msg_box.fifo_sel);//送入编号
		return 0;
	}
	else{
		return -1;
	}
}

void task_msg_init(struct TASK *task,unsigned int fifo_sel){
	task->msg_box.fifo_sel=fifo_sel;
	task->msg_box.num=0;//没有新消息
	task->msg_box.flag=1;//开启接收信息
	task->msg_box.musk=0;//没有屏蔽
	task->msg_box.msg=0;//没有消息
	return;
}
