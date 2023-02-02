/* コンソール関係 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>
extern struct TASK* system_task;
void console_task(struct SHEET *sheet, int memtotal)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	struct MEMMAN *memman = task_now()->memman;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);//内存分配!!!
	pageman_link_page_32_m(pageman,fat,7,3,0);//
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
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 未使用マーク */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 日本語フォントファイルを読み込めたか？ */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	/* プロンプト表示 */
	cons_putchar(&cons, '>', 1);
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
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {	/* カーソルON */
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* カーソルOFF */
				if (cons.sht != 0) {
					boxfill32(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) {	/* コンソールの「×」ボタンクリック */
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
				if (i == 8 + 256) {
					/* バックスペース */
					if (cons.cur_x > 16) {
						/* カーソルをスペースで消してから、カーソルを1つ戻す */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* Enter */
					/* カーソルをスペースで消してから改行する */
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);	/* コマンド実行 */
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					/* プロンプト表示 */
					cons_putchar(&cons, '>', 1);
				} else {
					/* 一般文字 */
					if (cons.cur_x < 240) {
						/* 一文字表示してから、カーソルを1つ進める */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			/* カーソル再表示 */
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
	if (s[0] == 0x09) {	/* タブ */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht32(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	/* 32で割り切れたらbreak */
			}
		}
	} else if (s[0] == 0x0a) {	/* 改行 */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* 復帰 */
		/* とりあえずなにもしない */
	} else {	/* 普通の文字 */
		if (cons->sht != 0) {
			putfonts8_asc_sht32(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
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
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16; /* 次の行へ */
	} else {
		/* スクロール */
		if (sheet != 0) {
			for (y = 28; y < 28 + 112; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buf32[x + y * sheet->bxsize] = sheet->buf32[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + 112; y < 28 + 128; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buf32[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			if(sheet->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
				sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);//换行以后刷新图层
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

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (strcmp(cmdline, "reload") == 0) {
		sys_reboot();
	} else if (strcmp(cmdline, "shutdown") == 0) {
		struct FIFO32 *fifo = &system_task->fifo;//intel南桥的关机方法
		fifo32_put(fifo,8);
	} else if (strcmp(cmdline, "rdrand") ==0){
		cmd_rdrand(cons, cmdline);
	}
	else if (strcmp(cmdline,"desktop") ==0){//开启桌面
		desktop_start();
	}
	else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* コマンドではなく、アプリでもなく、さらに空行でもない */
			cons_putstr0(cons, "Bad command0.\n\n");
		}
	}
	return;
}
void cmd_rdrand(struct CONSOLE *cons, int memtotal){
	char s[60];
	int i=rdrand();
	sprintf(s,"rdrand is %d %x\n",i,i);
	cons_putstr0(cons, s);
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
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf32[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	if(sheet->ctl==*(int*)0x0fe4)//当前图层的控制器与活动图层控制器相同
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct CONSOLE *cons)
{
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
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

void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct MEMMAN *memman =  task_now()->memman;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = &system_task->fifo;
	struct task_abort struct_task_abort;
	int i;
	
	char str[30];
	sprintf(str,"sht is %x\n",cons->sht);
	cons_putstr0(cons,str);
	
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	//释放fat
	for(i=0;i<3;i++){
		void* p=pageman_unlink_page_32(pageman,(int)fat+0x1000*i,1);
		//memman_free_page_32(pageman,p);
	}
	memman_free_4k(memman, (int) fat, 4 * 2880);

	if (cons->sht != 0) {
		//释放图层
		for(i=0;i<(256 * 165+0xfff)>>12;i++){
			void* po=pageman_unlink_page_32(pageman,(int)(cons->sht->buf)+0x1000*i,1);
			//memman_free_page_32(pageman,po);
		}
		memman_free_4k(memman, (int) cons->sht->buf, 256 * 165);
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
	
	/* コマンドラインからファイル名を生成 */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; /* とりあえずファイル名の後ろを0にする */

	/* ファイルを探す */
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		/* 見つからなかったので後ろに".HRB"をつけてもう一度探してみる */
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		/* ファイルが見つかった場合 */
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat);
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
					for(i=0;i<(task->fhandle[i].size+0xfff)>>12;i++){
						void* po=pageman_unlink_page_32(pageman,(int)(task->fhandle[i].buf)+0x1000*i,1);
						//memman_free_page_32(pageman,po);
					}
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			for(i=0;i<(segsiz+0xfff)>>12;i++){
				void* po=pageman_unlink_page_32(task_now()->memman,(int)q+0x1000*i,1);
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
	} else if (edx == 9) {//应用程序用malloc
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16バイト単位に切り上げ */
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {//应用程序用free
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16バイト単位に切り上げ */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
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
					task_sleep(task);	/* FIFOが空なので寝て待つ */
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { /* カーソル用タイマ */
				/* アプリ実行中はカーソルが出ないので、いつも次は表示用の1を注文しておく */
				timer_init(cons->timer, &task->fifo, 1); /* 次は1を */
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	/* カーソルON */
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* カーソルOFF */
				cons->cur_c = -1;
			}
			if (i == 4) {	/* コンソールだけを閉じる */
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht->sid + 2024);	/* 2024～2279 */
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256) { /* キーボードデータ（タスクA経由）など */
				reg[7] = i - 256;
				return 0;
			}
		}
	} else if (edx == 16) {//获取计时器
		//task0=task_now();
		/*if(task->timerctl==0){
			task->timerctl=0;
		}*/
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;	/* 自動キャンセル有効 */
	} else if (edx == 17) {//初始化计时器
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {//计时器定时
		timer_settime((struct TIMER *) ebx, eax);
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
					(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				//reg[7] = (int) fh;
				reg[7] = (int) i+1;
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
			}
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
	} else if (edx == 28) {//获取鼠标
		
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
