/* コンソール関係 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>
void cmd_fdir(struct CONSOLE *cons);
extern struct TASK* system_task;
char text_buff[100];
unsigned int _cons_read_file(char* buff,unsigned int* size,unsigned int fat32_addr,unsigned part_base_lba,FAT32_HEADER* mbr,unsigned int start_lba_low,unsigned int start_lba_high){
	unsigned int i;
	for(i=0;;i++){
		//如果多读一个簇就会超过size设定的大小
		if((i+1)*(mbr->BPB_SecPerClus)*512>size){
			*size=i*(mbr->BPB_SecPerClus)*512;//设定已读内容数量
			return 0;
		}
		//扇区位置:逻辑分区基地址+保留分区+FAT所占的分区*FAT数量+(簇号-2)*簇大小
		dmg_read(buff+i*512*(mbr->BPB_SecPerClus),part_base_lba+mbr->BPB_ResvdSecCnt+(mbr->BPB_FATSz32)*(mbr->BPB_NumFATs)+(start_lba_low-2)*(mbr->BPB_SecPerClus),mbr->BPB_SecPerClus,0);
		if(*((unsigned int*)fat32_addr+start_lba_low)==0x0fffffff){
			break;
		}
		start_lba_low=((int*)fat32_addr)[start_lba_low];
	}
	return 0;
}
void console_task(struct SHEET *sheet, int memtotal)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	struct MEMMAN *memman = task_now()->memman;
	int i, *fat = *(int**)0x0026f028;//(int *) memman_alloc_4k(memman, 4 * 2880);//内存分配!!!
	//pageman_link_page_32_m(pageman,fat,7,3,0);//
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[30];
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);

	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;
	if (cons.sht != 0) {
		cons.timer = timer_alloc(0);
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(0,cons.timer, timer_get_fps(0)/2 ,0);
	}
	//file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 未使用マーク */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 日本語フォントファイルを読み込めたか？ */
		task->langmode = 0;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;
	//打开文件根目录
	if(*(unsigned int*)0x0026f030!=0){
		FAT32_HEADER* mbr=*(unsigned int*)0x0026f024;
		unsigned int fat32_addr=*(unsigned int*)0x0026f028;
		unsigned int part_base_lba=*(unsigned int*)0x0026f030;
		unsigned int root_lba=mbr->BPB_Root;
		//task_now()->root_dir_addr=memman_alloc_4k(memman,8192);//4k页就够了
		//pageman_link_page_32_m(pageman,task_now()->root_dir_addr,7,2,0);
		unsigned int size=8192;
		//_cons_read_file(task_now()->root_dir_addr,&size,fat32_addr, part_base_lba,mbr, mbr->BPB_Root,0);
		task_now()->root_dir_addr=file_loadfile2(mbr->BPB_Root, &size, fat32_addr);
		*(unsigned int*)0x0026f038=task_now()->root_dir_addr;
	}
	else{
		task_now()->root_dir_addr=0;
	}
	/*接下来获取文件夹里的文件数量*/
	//int num=_get_file_number(root_dir_addr,8192);
	//(int*)(0x0026f038)=num;
	/* プロンプト表示 */
	cons_putchar(&cons, '>', 1);
	/*启用鼠标点击事件*/
	task->fifo32_mouse_event=0x20000000;
	task->mouse_x=0;
	task->mouse_y=0;
	task->mouse_btn=0;
	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons.sht != 0) { /* カーソル用タイマ */
				if (i != 0) {
					timer_init(cons.timer, &task->fifo, 0); /* 次は0を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1); /* 次は1を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(0,cons.timer, timer_get_fps(0)/2 ,0);
			}
			if (i == 2) {	//光标关闭
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	//光标开启
				if (cons.sht != 0) {
					boxfill32(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) {	//退出信号
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) {//键盘事件
				if (i == 8 + 256) {
					/* バックスペース */
					if (cons.cur_x > 16) {
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* Enter */
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);	/* コマンド実行 */
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					//显示提示符
					cons_putchar(&cons, '>', 1);
				} else {
					/* 一般文字 */
					if (cons.cur_x < 240) {
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			if(i==task->fifo32_mouse_event){//获得了鼠标事件
				MOUSESTATUS* mouse_status=fifo_mouse_get(&(task->fifom));
				task->mouse_x=mouse_status->x;
				task->mouse_y=mouse_status->y;
				task->mouse_btn=mouse_status->btn;
				task->sheet_mouse_on=mouse_status->sht;
			}
			//光标的重新显示
			if (cons.sht != 0) {
				if (cons.cur_c >= 0) {
					boxfill32(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				if((cons.sht)->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
					sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {	/* 空格 */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht32(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x >= 8 + 1024) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) { 
				break;	/* 32で割り切れたらbreak */
			}
		}
	} else if (s[0] == 0x0a) {	/* 换行 */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* 回车 */
		/* 不进行任何处理 */
	} else {	/* 普通文字 */
		if (cons->sht != 0) {
			putfonts8_asc_sht32(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			/* 当移动量为0时,不移动光标 */
			cons->cur_x += 8;
			if (cons->cur_x >= 8 + 1024) {//输出超界
				cons_newline(cons);//换行
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	if (cons->cur_y < 28 + 600) {//如果在界限内则直接换行
		cons->cur_y += 16;
	} else {
		//滚动屏幕
		if (sheet != 0) {
			for (y = 28; y < 28 + (768-2*28) - 16; y++) {
				for (x = 8; x < 8 + 1024; x++) {
					sheet->buf32[x + y * sheet->bxsize] = sheet->buf32[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + (768-2*28)-16; y < 28 + (768-2*28); y++) {
				for (x = 8; x < 8 + 1024; x++) {
					sheet->buf32[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			if(sheet->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
				sheet_refresh(sheet, 8, 28, 8 + 1024, 28 + 768);//换行以后刷新图层
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x = 16;
	}
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}
void cmd_cd(struct CONSOLE *cons,char* cmdline);

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (asm_sse_strcmp(cmdline, "cls",3) == 16 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (asm_sse_strcmp(cmdline, "dir",3) == 16 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (asm_sse_strcmp(cmdline, "exit",4) == 16) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (asm_sse_strcmp(cmdline, "reload",6) == 16) {
		sys_reboot();
	} else if (asm_sse_strcmp(cmdline, "shutdown",8) == 16) {
		struct FIFO32 *fifo = &system_task->fifo;//intel南桥的关机方法
		fifo32_put(fifo,8);
	} else if (asm_sse_strcmp(cmdline, "rdrand",6) ==16){
		cmd_rdrand(cons, cmdline);
	}
	else if (asm_sse_strcmp(cmdline,"desktop",7) ==16){//开启桌面
		desktop_start();
	}
	else if (asm_sse_strcmp(cmdline,"fdir",4) ==16){//显示所有文件
		cmd_fdir(cons);
	}
	else if (strncmp(cmdline,"cd ",3) ==0){//切换目录
		cmd_cd(cons,cmdline);
	}
	else if (strncmp(cmdline,"task ",4) ==0){//切换目录
		cmd_task(cons,cmdline);
	}
	else if (asm_sse_strcmp(cmdline,"12341234",4) == 16){
		cons_putstr0(cons, "sse OK.\n\n");
	}
	else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* コマンドではなく、アプリでもなく、さらに空行でもない */
			cons_putstr0(cons, "Bad command0.\n\n");
		}
	}
	return;
}
void cmd_task(struct CONSOLE *cons,char* cmdline){
	int i,j;
	struct TASKCTL* taskctl=task_ctl_now();
	for(i=0;i<MAX_TASKS;i++){
		if(taskctl->tasks0[i].flags!=0){
			sprintf(text_buff,"%s id: %d mem: %d\n",taskctl->tasks0[i].name,taskctl->tasks0[i].id_low,taskctl->tasks0[i].mem_use);
			cons_putstr0(cons,text_buff);
		}
	}
	cons_putstr0(cons,"\n");
	int lvl=0;//当前等级
	struct TASK* task=&(taskctl->tasks0[0]);//获取初始任务
	for(;;){
		for(i=0;i<lvl;i++){
			cons_putstr0(cons,"-");
		}
		sprintf(text_buff,"%s id: %d mem: %d\n",task->name,task->id_low,task->mem_use);
		cons_putstr0(cons,text_buff);
		if(task->child_task!=0){//有孩子
			task=task->child_task;//进入孩子进程
			lvl++;
			continue;
		}
		else if(task->brother_task!=task){//有兄弟进程
			task=task->brother_task;//进入兄弟进程
		}
		else{//既没有子进程也没有兄弟进程了，需要返回
			for(;;){//找到下一个可以处理的位置
				if(task->father_task!=0){//返回父进程
					task=task->father_task;//进入父进程
					lvl--;
					task=task->brother_task;//进入兄弟进程
					if(task->father_task!=0){
						if(task->father_task->child_task==task){//父进程的子进程是自己
							continue;//当前循环已经完成
						}
						else{//继续
							break;
						}
					}
					else{//是创世任务
						task=0;
						break;
					}
				}
				else{
					task=0;
					break;
				}
			}
			if(task==0){//没有进程了
				break;
			}
		}
	}
}
void find_intel_gpu(struct CONSOLE *cons){
    // PCI配置空间基地址
    unsigned int pci_config_space = 0xCF8; 
    int bus,dev,func;
    // 枚举PCIe根桥下的所有总线
    for(bus = 0; bus < 256; bus++){
        // 枚举每一总线上的所有设备
        for(dev = 0; dev < 32; dev++){
			for(func=0;func<8;func++){
				// 选择要访问的总线和设备
				unsigned int id = (bus<<16) | (dev<<11) | (1<<31) | (func<<8);
				io_out32(pci_config_space,id); 
				unsigned int u = io_in32(pci_config_space+4);
				unsigned short vendor_id =u&0xffff;
				if(vendor_id==0xffff){//设备不存在
					continue;
				}
				sprintf(text_buff,"Found DEVICE %d-%d-%d: %x ", bus,dev,func, u);
				cons_putstr0(cons,text_buff);

				io_out32(pci_config_space,id|0x08);
				u = io_in32(pci_config_space+4);
				sprintf(text_buff,"class %x ", u);
				cons_putstr0(cons,text_buff);
				if(1){
					io_out32(pci_config_space,id|0x10);
					u = io_in32(pci_config_space+4);
					sprintf(text_buff,"BAR0 %x ", u);
					cons_putstr0(cons,text_buff);
					
					io_out32(pci_config_space,id|0x14);
					u = io_in32(pci_config_space+4);
					sprintf(text_buff,"BAR1 %x \n", u);
					cons_putstr0(cons,text_buff);
					
					io_out32(pci_config_space, id|0x18); 
					u = io_in32(pci_config_space+4);
					sprintf(text_buff,"BAR2 %x ", u);
					cons_putstr0(cons,text_buff); 
					
					io_out32(pci_config_space, id|0x1c); 
					u = io_in32(pci_config_space+4);
					sprintf(text_buff,"BAR3 %x ", u);
					cons_putstr0(cons,text_buff); 

					io_out32(pci_config_space, id|0x20); 
					u = io_in32(pci_config_space+4);
					sprintf(text_buff,"BAR4 %x ", u);
					cons_putstr0(cons,text_buff); 
				}
				cons_putstr0(cons,"\n");
			}
        }
    }
}

void cmd_rdrand(struct CONSOLE *cons, int memtotal){
	char s[60];
	int i=rdrand();
	sprintf(s,"rdrand is %d %x\n",i,i);
	cons_putstr0(cons, s);
	find_intel_gpu(cons);
}
void cmd_mem(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman =  task_now()->memman;
	char s[60];
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
	sprintf(s,"t_p: %d l_p: %d\n\n",*(int*)0x0026f004,*(int*)0x0026f00c);
	cons_putstr0(cons, s);
	return;
}

void cmd_cls(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y <  768 - 8; y++) {
		for (x = 8; x < 1024-8; x++) {
			sheet->buf32[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	if(sheet->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
		sheet_refresh(sheet, 8, 28, 8 + 1024, 28 + 768);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct CONSOLE *cons)
{
	//struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	struct FILEINFO *finfo=task_now()->root_dir_addr;
	int i, j;
	char s[60];
	if(task_now()->root_dir_addr==0){//文件系统没有准备好
		cons_putstr0(cons, "file system not ready\n");
		return;
	}
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				int year,month,date;
				year= (finfo[i].date)>>9;
				month= ((finfo[i].date)>>5)&0x0f;
				date=(finfo[i].date)&0x1f;
				int hour=(finfo[i].time)>>11;
				int min=((finfo[i].time)>>5)&0x3f;
				int second=(finfo[i].time)&0x1f;
				sprintf(s, "filename.ext   %7d %5dY%2dM%2dD %2d:%2d:%2d\n", finfo[i].size,year,month,date,hour,min,second);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_fdir(struct CONSOLE *cons)
{
	//struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	struct FILEINFO *finfo=task_now()->root_dir_addr;
	int i, j;
	char s[50];
	if(task_now()->root_dir_addr==0){//文件系统没有准备好
		cons_putstr0(cons, "file system not ready\n");
		return;
	}
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if (finfo[i].type  != 0x0f) {
				sprintf(s, "filename.ext   %7d:%5d:%5x ", finfo[i].size,finfo[i].clustno,finfo[i].type);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
				if((finfo[i].type&0x01)!=0){
					s[0]='R';
				}
				else{
					s[0]='-';
				}
				if((finfo[i].type&0x02)!=0){
					s[1]='H';
				}
				else{
					s[1]='-';
				}
				if((finfo[i].type&0x04)!=0){
					s[2]='S';
				}
				else{
					s[2]='-';
				}
				if((finfo[i].type&0x08)!=0){
					s[3]='V';
				}
				else{
					s[3]='-';
				}
				if((finfo[i].type&0x10)!=0){
					s[4]='D';
				}
				else{
					s[4]='-';
				}
				if((finfo[i].type&0x20)!=0){
					s[5]='A';
				}
				else{
					s[5]='-';
				}
				s[6]='\n';
				s[7]=0;
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_cd(struct CONSOLE *cons,char* cmdline){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct MEMMAN *memman =  task_now()->memman;
	int i;
	if(*(unsigned int*)0x0026f030!=0){
		struct FILEINFO* finfo = file_full_search(cmdline+3, task_now()->root_dir_addr, 0x10,0x08,224);//查找文件夹并且不是系统文件
		if(finfo==0){
			cons_putstr0(cons,"file not found\n");
		}
		else{
			FAT32_HEADER* mbr=*(unsigned int*)0x0026f024;
			unsigned int fat32_addr=*(unsigned int*)0x0026f028;
			unsigned int part_base_lba=*(unsigned int*)0x0026f030;
			unsigned int dir_lba=finfo->clustno;
			if(task_now()->root_dir_addr!=0){
				void* po=pageman_unlink_page_32_m(pageman,task_now()->root_dir_addr,2,1);
			}
			task_now()->root_dir_addr=memman_alloc_4k(memman,8192);//4k页就够了
			pageman_link_page_32_m(pageman,task_now()->root_dir_addr,7,2,0);
			for(i=0;i<8192/4;i+=4){//清零
				((int*)(task_now()->root_dir_addr))[i]=0;
			}
			unsigned int size=8192;
			if(dir_lba!=0){//如果不是0的话就打开目录
				_cons_read_file(task_now()->root_dir_addr,&size,fat32_addr, part_base_lba,mbr, dir_lba,0);
			}
			else{//否则打开根目录
				_cons_read_file(task_now()->root_dir_addr,&size,fat32_addr, part_base_lba,mbr, mbr->BPB_Root,0);
			}
		}
	}
	else{
		cons_putstr0(cons,"file system not ready\n");
	}
}

void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct MEMMAN *memman =  task_now()->memman;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = &system_task->fifo;
	struct task_abort struct_task_abort;
	int i;
	if (cons->sht != 0) {
		timer_cancel(0,cons->timer);
	}
	//释放fat
	//for(i=0;i<3;i++){
	//	void* p=pageman_unlink_page_32(pageman,(int)fat+0x1000*i,1);
	//	//memman_free_page_32(pageman,p);
	//}
	//memman_free_4k(memman, (int) fat, 4 * 2880);

	if (cons->sht != 0) {
		//释放图层
		for(i=0;i<(1024*768*4+0xfff)>>12;i++){
			void* po=pageman_unlink_page_32(pageman,(int)(cons->sht->buf)+0x1000*i,1);
			//memman_free_page_32(pageman,po);
		}
		memman_free_4k(memman, (int) cons->sht->buf, 1024*768*4);
		sheet_free(cons->sht);
	} 
	struct_task_abort.task=task;
	system_task_abort(&struct_task_abort);
	fifo32_put(fifo, 5);
	for (;;) {
		task_sleep(task);
	}
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)//cmd start命令调用此函数
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	/* コマンドラインに入力された文字列を、一文字ずつ新しいコンソールに入力 */
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	/* コマンドラインに入力された文字列を、一文字ずつ新しいコンソールに入力 */
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
	struct TASK *task = task_now();
	unsigned char mode = cmdline[9] - '0';
	if (mode <= 2) {
		task->langmode = mode;
	} else {
		cons_putstr0(cons, "mode number error.\n");
	}
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct MEMMAN *memman = task_now()->memman;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL *shtctl;
	struct SHEET *sht;
	if(task_now()->root_dir_addr==0){//文件系统没有准备好
		cons_putstr0(cons, "file system not ready\n");
		return 0;
	}
	/* コマンドラインからファイル名を生成 */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; /* とりあえずファイル名の後ろを0にする */

	/* ファイルを探す */
	finfo = file_search(name, task_now()->root_dir_addr, 224);
	if (finfo == 0 && name[i - 1] != '.') {
		/* 見つからなかったので後ろに".HRB"をつけてもう一度探してみる */
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, task_now()->root_dir_addr, 224);
	}
	
	if (finfo != 0) {
		/* ファイルが見つかった場合 */
		appsiz = finfo->size;
		//*(unsigned int*)0x0026f03c=finfo;
		p = file_loadfile2(finfo->clustno, &appsiz, task_now()->fat);
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(task_now()->memman, segsiz);//内存分配!!!
			pageman_link_page_32_m(pageman,q,7,(segsiz+0xfff)>>12,0);//
			task->ds_base = (int) q;
			set_segmdesc(task->ldt + 2, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 3, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			for (i = 0; i < 8; i++) {//清空文件记录缓存
				task->fhandle[i].buf = 0;
			}
			start_app(0x1b, 2 * 8 + 4, esp, 3 * 8 + 4, &(task->tss.esp0));/*eip,cs,esp,ss,esp0*/
			struct SHTCTL** shtctl_base=*(int*)0x0026f018;
			int j;
			for(j=0;j<(*(int*)0x0026f01c);j++){//遍历所有图层控制器
				shtctl = shtctl_base[j];//图层控制器
				for (i = 0; i < MAX_SHEETS; i++) {
					sht = &(shtctl->sheets0[i]);
					if ((sht->flags & 0x11) == 0x11 && sht->task == task) {//检索所有当前任务创建的且是应用程序标签的图层
						/* アプリが開きっぱなしにした下じきを発見 */
						sheet_free(sht);	//释放
					}
				}
			}
			for (i = 0; i < 8; i++) {	//查看打开的文件列表
				if (task->fhandle[i].buf != 0) {
					sprintf(text_buff,"app file page free: %d\n",(task->fhandle[i].size+0xfff)>>12);
					cons_putstr0(cons,text_buff);
					for(i=0;i<(task->fhandle[i].size+0xfff)>>12;i++){
						void* po=pageman_unlink_page_32(pageman,(int)(task->fhandle[i].buf)+0x1000*i,1);
						//memman_free_page_32(pageman,po);
					}
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(0,&task->fifo);
			for(i=0;i<(segsiz+0xfff)>>12;i++){
				void* po=pageman_unlink_page_32(pageman,(int)q+0x1000*i,1);
				//memman_free_page_32(pageman,po);
			}
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}
		for(i=0;i<((appsiz+0xfff)>>12);i++){
			void* po=pageman_unlink_page_32(pageman,(int)p+0x1000*i,1);
			//memman_free_page_32(pageman,po);
		}
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	/* ファイルが見つからなかった場合 */
	return 0;
}

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1;	/* eaxの次の番地 */
		/* 保存のためのPUSHADを強引に書き換える */
		/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
		/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = task_now()->memman;
	struct MEMMAN *memman_os=(struct MEMMAN *) MEMMAN_ADDR;
	//cons_putstr0(cons,"sys api call \n");
	if (edx == 1) {//命令行输出字符
		cons_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {//命令行输出字符串0结尾
		cons_putstr0(cons, (char *) ebx + ds_base);
	} else if (edx == 3) {//命令行输出字符串指定长度
		cons_putstr1(cons, (char *) ebx + ds_base, ecx);
	} else if (edx == 4) {//结束应用程序api_end
		return &(task->tss.esp0);
	} else if (edx == 5) {//创建窗口
	/*原来的代码分配的内存空间在用户空间，需要将这部分映射到高地址被全局访问*/
		/*接下来链接目标页面*/
		int addr_from=(ebx + ds_base);
		int addr_to=memman_alloc_4k(memman_os,esi*edi*4+0x1fff);//这里需要根据窗口大小修改
		//int addr_to=0xd0000000;
		{
			int i;
			char s[30];
			for(i=0;i<((esi*edi*4+0x1fff+(addr_from&0xfff))>>12);i++){
				int paddr_from=0xffc00000|(((addr_from+i*0x1000)>>10)&0xfffffffc);//获取目标页面的链接地址
				int paddr_to=0xffc00000|(((addr_to+i*0x1000)>>10)&0xfffffffc);//获取目标页面的链接地址
				*(int*)paddr_to=*(int*)paddr_from;
			}
		}
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, addr_to+(addr_from&0xfff), esi, edi, eax);
		make_window32((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
		sheet_updown(sht, shtctl->top); /* 今のマウスと同じ高さになるように指定： マウスはこの上になる */
		reg[7] = (int) sht;
	} else if (edx == 6) {//在指定窗口画一个字符
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		putfonts32_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) {
			if(sht->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
				sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
	} else if (edx == 7) {//在指定窗口画一个矩形
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		boxfill32(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			if(sht->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
				sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 8) {//应用程序用malloc_init
		memman_init((struct MEMMAN *) (ebx + ds_base));
		ecx &= 0xfffffff0;	/* 16バイト単位に */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
		cons_putstr0(cons,"memman init\n");
		
	} else if (edx == 9) {//应用程序用malloc
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16バイト単位に切り上げ */
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
		cons_putstr0(cons,"memman malloc\n");
	} else if (edx == 10) {//应用程序用free
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16バイト単位に切り上げ */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
		cons_putstr0(cons,"memman free\n");
	} else if (edx == 11) {//在指定图层画一个像素
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		sht->buf32[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			if(sht->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
				sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
	} else if (edx == 12) {//强制刷新窗口
		sht = (struct SHEET *) ebx;
		if(sht->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
			sheet_refresh(sht, eax, ecx, esi, edi);
	} else if (edx == 13) {//在指定窗口画直线
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			if (eax > esi) {
				i = eax;
				eax = esi;
				esi = i;
			}
			if (ecx > edi) {
				i = ecx;
				ecx = edi;
				edi = i;
			}
			if(sht->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
				sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {//关闭窗口(直接清楚图层)
		sheet_free((struct SHEET *) ebx);
	} else if (edx == 15) {//获取键盘按键
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	/* FIFO为空则等待 */
				} else {
					io_sti();
					reg[7] = -1;//没有数据
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { /* 光标用定时器*/
				timer_init(cons->timer, &task->fifo, 1);
				timer_settime(0,cons->timer, timer_get_fps(0)/2,0);
			}
			if (i == 2) {	/* 光标开启 */
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* 光标关闭*/
				cons->cur_c = -1;
			}
			if (i == 4) {	
				timer_cancel(0,cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht->sid + 2024);	/* 2024～2279 */
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256 && i<=511) { /* キーボードデータ（タスクA経由）など */
				reg[7] = i - 256;
				return 0;
			}
			if(i==task->fifo32_mouse_event){//获得了鼠标事件
				MOUSESTATUS* mouse_status=fifo_mouse_get(&(task->fifom));
				task->mouse_x=mouse_status->x;
				task->mouse_y=mouse_status->y;
				task->mouse_btn=mouse_status->btn;
				task->sheet_mouse_on=mouse_status->sht;
			}
		}
	} else if (edx == 16) {//获取计时器
		//task0=task_now();
		/*if(task->timerctl==0){
			task->timerctl=0;
		}*/
		reg[7] = (int) timer_alloc(0);
		((struct TIMER *) reg[7])->flags2 = 1;	/* 自動キャンセル有効 */
	} else if (edx == 17) {//初始化计时器
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {//计时器定时
		timer_settime(0,(struct TIMER *) ebx, (timer_get_fps(0)*eax)/100,0);
	} else if (edx == 19) {//计时器释放
		timer_free((struct TIMER *) ebx);
	} else if (edx == 20) {//播放声音(蜂鸣器发声)
		if (eax == 0) {
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}
	} else if (edx == 21) {//打开文件fopen
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i < 8) {
			finfo = file_search((char *) ebx + ds_base,
					task_now()->root_dir_addr, 224);
			if (finfo != 0) {
				//reg[7] = (int) fh;
				reg[7] = (int) i+1;//返回代号
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
				cons_putstr0(cons,"fopen ok\n");
			}
			else{
				cons_putstr0(cons,"fopen err\n");
			}
		}
		else{
			cons_putstr0(cons,"fopen full\n");
		}
	} else if (edx == 22) {//关闭文件fclose
		fh = &task->fhandle[eax-1];
		if(fh->buf!=0){
			for(i=0;i<(fh->size+0xfff)>>12;i++){
				void* p=pageman_unlink_page_32(pageman,(int)(fh->buf)+0x1000*i,1);
				//memman_free_page_32(pageman,p);
			}
			memman_free_4k(memman, (int) fh->buf, fh->size);
			fh->buf = 0;
		}
	} else if (edx == 23) {//文件定位fseek
		fh = &task->fhandle[eax-1];
		if (ecx == 0) {
			fh->pos = ebx;
		} else if (ecx == 1) {
			fh->pos += ebx;
		} else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	} else if (edx == 24) {//获取文件大小fsize
		fh = &task->fhandle[eax-1];
		if (ecx == 0) {
			reg[7] = fh->size;
		} else if (ecx == 1) {
			reg[7] = fh->pos;
		} else if (ecx == 2) {
			reg[7] = fh->pos - fh->size;
		}
	} else if (edx == 25) {//读取文件fread
		fh =  &task->fhandle[eax-1];
		for (i = 0; i < ecx; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
	} else if (edx == 26) {//获取命令行
		cons_putstr0(cons,"api get command:");
		cons_putstr0(cons,task->cmdline);
		cons_putstr0(cons,"\n");
		i = 0;
		for (;;) {
			*((char *) ebx + ds_base + i) =  task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= ecx) {
				break;
			}
			i++;
		}
		reg[7] = i;
	} else if (edx == 27) {//设置语言
		reg[7] = task->langmode;
	} else if (edx == 28) {//获取按键加强版
		//返回值：eax ==-1 无数据
		//eax == 0 键盘数据 数据在 edx
		//eax ==1 鼠标数据 x坐标在esi y坐标在edi 按键状态在 edx 命中的图层
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	/* FIFOが空なので寝て待つ */
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { 
				timer_init(cons->timer, &task->fifo, 1); 
				timer_settime(0,cons->timer, timer_get_fps(0)/2,0);
			}
			if (i == 2) {	//光标开启
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	//光标关闭
				cons->cur_c = -1;
			}
			if (i == 4) {	//关闭信号
				timer_cancel(0,cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht->sid + 2024);	/* 2024～2279 */
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256 && i<=511) { /* キーボードデータ（タスクA経由）など */
				reg[7] = 0;//eax=0 键盘事件
				reg[5] = i-256;//edx=字符编码
				return 0;//返回键盘事件
			}
			if(i==task->fifo32_mouse_event){//获得了鼠标事件
				MOUSESTATUS* mouse_status=fifo_mouse_get(&(task->fifom));
				task->mouse_x=mouse_status->x;
				task->mouse_y=mouse_status->y;
				task->mouse_btn=mouse_status->btn;
				task->sheet_mouse_on=mouse_status->sht;
				reg[7]=1;//eax=1 鼠标事件
				reg[5]=mouse_status->btn;//edx=mouse->btn
				reg[1]=mouse_status->x;//esi=mouse->x
				reg[0]=mouse_status->y;//edi=mouse->y
				//reg[1]==mouse_status->x;//esi=mouse->x
				//reg[0]==1314520;//edi=mouse->y
				reg[4]=mouse_status->sht;//ebx==mouse->sht
				return 0;//0返回鼠标事件
			}
		}
	}
	return 0;
}

int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* 異常終了させる */
}

int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	sprintf(s,"ERROR CODE =%08X\n",esp[10]);
	cons_putstr0(cons, s);
	sprintf(s,"APP CS =%08X\n",esp[12]);
	cons_putstr0(cons, s);
	sprintf(s,"APP ESP =%08X\n",esp[14]);
	cons_putstr0(cons, s);
	sprintf(s,"APP SS =%08X\n",esp[15]);
	cons_putstr0(cons, s);
	sprintf(s,"APP DS =%08X\n",esp[8]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* 異常終了させる */
}


void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf32[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}
void cons_set_system_task(struct TASK* task){
	system_task=task;
}
