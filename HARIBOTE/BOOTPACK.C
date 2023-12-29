/* bootpackのメイン */

#include "bootpack.h"

#define KEYCMD_LED		0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

EFI_GUID varGuid = {0xb105cc01, 0x2018, 0x0401, {0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xef, 0x00}};;
EFI_SYSTEM_TABLE* Systemtable_base;
void* gThis;
void HariMain(void* this,EFI_SYSTEM_TABLE* Systemtable)
{
	//com_out_string(0x3f8,"hello\n");
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct SHTCTL *shtctl[10];//图层管理器
	struct SHEET sheet_temp;//临时图层
	int shtctl_point=0;
	char s[40];
	int s_p;
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	*(unsigned int*)0x0026f040=&mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct MEMMAN *memman_core;
	unsigned int *buf_back, buf_mouse[256];
	struct SHEET *sht_back[10], *sht_mouse;
	struct TASK *task_a, *task;
	struct SHEET* sheet_mouse_on=0;//当前鼠标锁定的图层
	struct SHEET* sheet_mouse_on_last=0;//上次鼠标中断的图层
	int mouse_on_header=0;//鼠标抓取了标题栏吗？
	struct PAGEMAN32 struct_pageman;
	//void* sys_esp;
	static char keytable0[0x80] = {//基本码表
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {//shift码表
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	static char keytable2[0x80] = {//6E码表
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0x0a,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '?',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
	};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	int key_ctrl = 0,key_alt = 0;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
	struct SHEET *sht = 0, *key_win, *sht2;
	int *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];
	struct PAGEMAN32 *pageman = &struct_pageman;
	Systemtable_base=Systemtable;
	gThis=this;
	init_gdtidt(this);//初始化gdt和idt
	//init_acpi();//初始化acpi
	//acpi_shutdown();
	//memtotal = memtest(0x00900000, 0xbfffffff);//初始化内存
	memman_init(memman);
	//memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	//memman_free(memman, 0x00900000, memtotal - 0xb0000000);
	memman_free(memman, 0xc0200000, 0xdfffffff);//虚拟空间的地址大小
	
	init_page(pageman);//初始化分页
	memmam_link_page_32_m(pageman,0x268000,0xc0000000,0x00270007,16,1);//gdt映射到本来的位置
	memmam_link_page_32_m(pageman,0x268000,0xc0020000,0x0026f007,1,1);//idt映射到本来的位置
	memmam_link_page_32_m(pageman,0x268000,0xc0100000,0x00280007,128,1);//haribote.hrb映射到高地址
	memmam_link_page_32_m(pageman,0x268000,0x07,0x07,0x900,1);//低地址映射到本来的位置
	memmam_link_page_32_m(pageman,0x268000,0xe0000000,0xe0000007,0x20000-1024,1);//高地址映射到本来的位置
	//memmam_link_page_32_m(pageman,0x268000,0xfffff000,0x268007,1,1);//顶端页面映射自身
	*((struct PAGEMAN32 **)ADR_PAGEMAN)=pageman;//保存变量
	
	store_cr3(0x268000);//内核页表加载
	j=load_cr0();
	j|=0x80000000;//开启分页
	store_cr0(j);
	load_gdtr(0xffff,0xc0000000);//重载gdt至高位内存
	load_idtr(0xffff,0xc0020800);//重载idt至高位内存
	//if(support_apic()==1){//存在local_apic?使用local-apic?理中断
		init_apic((void*)0xfee00000);
	//}
	//else{
	//	init_pic();//初始化pic
	//}
	init_pit();//定时器在任务之前初始化
	init_hpet_timer();//初始化高精度定时器
	init_sse42();
	sprintf(s,"fifobuff:%x\n",fifobuf);
	com_out_string(0x3f8,s);
	sprintf(s,"fifo:%x\n",&fifo);
	com_out_string(0x3f8,s);
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;//系统fifo
	init_keyboard(&fifo, 256);//键盘从25开始
	enable_mouse(&fifo, 512, &mdec);//鼠标从512开始
	//if(support_apic()==0){//使用apic时执行此代码
	//	io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可(11111000) */
	//	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */
	//}
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	//运行第一个任务
	task_a = task_init(memman);
	task_a->memman=memman;//注册内存控制器
	task->tss.cr3=0x268000;
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	task_a->langmode = 0;
	//创建公共图层数组
	struct View* p=(unsigned char *) memman_alloc_4k(memman, sizeof(struct View [512]));//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,p,7,(sizeof(struct View [512])+0xfff)>>12,0);//没有目的链接地址
	//准备两个图层控制器
	*(int*)0x0026f01c=2;//两个桌面图层控制器
	for(i=0;i<2;i++){//10个图层管理器
		shtctl[i] = shtctl_init(memman,pageman, binfo->vram, binfo->scrnx, binfo->scrny);
		shtctl[i]->sheets0=p;
		shtctl[i]->sheets0_size=512;
		(shtctl[i]->func).sheet_refreshsub=((unsigned int)(shtctl[i]->func).sheet_refreshsub)+this;//增加偏移量
	}
	*((int *) 0x0fe4) = (int) shtctl[shtctl_point];//当前正在显示的图层
	*((int*)0x0026f018)=shtctl;//保存图层数组
	/* sht_back */
	//第一屏幕背景
	sht_back[0]  = sheet_alloc(shtctl[0]);
	buf_back  = (unsigned int *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny * 4);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,buf_back,7,((binfo->scrnx * binfo->scrny * 4)+0xfff)>>12,0);//没有目的链接地址
	////
	sheet_setbuf(sht_back[0], buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
	init_screen32(buf_back, binfo->scrnx, binfo->scrny);
	//sht_back[0]=desktop_task(sht_back[0]);//调用测试
	
	//第二屏幕背景
	sht_back[1]  = sheet_alloc(shtctl[1]);
	buf_back  = (unsigned int *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny * 4);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,buf_back,7,((binfo->scrnx * binfo->scrny * 4)+0xfff)>>12,0);//没有目的链接地址
	////
	sheet_setbuf(sht_back[1], buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
	init_screen32(buf_back, binfo->scrnx, binfo->scrny);
	
	
	/* sht_cons */
	//key_win = open_console(shtctl[0], memtotal);
	key_win = 0;
	
	/* sht_mouse */
	//第一屏幕鼠标
	sht_mouse = sheet_alloc(shtctl[0]);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor32(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
	my = (binfo->scrny - 28 - 16) / 2;
	//排布第二控制器
	sheet_slide(sht_back[1],  0,  0);
	//sheet_slide(key_win,   32, 4);
	//sheet_slide(sht_mouse[1], mx, my);
	sheet_updown(sht_back[1],  0);
	//sheet_updown(key_win,   1);
	//sheet_updown(sht_mouse[1], 2);
	//keywin_on(key_win);
	//排布第一控制器
	sheet_slide(sht_back[0],  0,  0);
	//sheet_slide(key_win,   32, 4);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back[0],  0);
	//sheet_updown(key_win,   1);
	sheet_updown(sht_mouse, 1);
	sheet_mouse_on_last=sht_back[0];
	/* 最初にキーボード状態との食い違いがないように、設定しておくことにする */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	*(unsigned int*)0x0026f03c =&sht_back;
goto st_next;
	/* nihongo.fntの読み込み */
	//fat = (int *) memman_alloc_4k(memman, 4 * 2880);//内存分配!!!
	//memmam_link_page_32_m(pageman,0x268000,fat,7,3,0);//
	
	//file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

	//finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	
	if (finfo != 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, fat);
	} else {
		nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);//内存分配!!!
		memmam_link_page_32_m(pageman,0x268000,nihongo,7,  22841,0);//
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /* フォントがなかったので半角部分をコピー */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			nihongo[i] = 0xff; /* フォントがなかったので全角部分を0xffで埋め尽くす */
		}
	}
	*((int *) 0x0fe8) = (int) nihongo;//字库数据位置
	for(i=0;i<3;i++){
		void* p=pageman_unlink_page_32(pageman,(int)fat+0x1000*i,1);
		//memman_free_page_32(pageman,p);
	}
	memman_free_4k(memman, (int) fat, 4 * 2880);
st_next:
	system_start();//启动系统进程
	device_init();//初始化存储设备驱动
	start_task_disk();//启动磁盘服务
	io_sti();
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			// キーボードコントローラに送るデータがあれば、送る 
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			// FIFOがからっぽになったので、保留している描画があれば実行する 
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
				io_sti();
			}
		} else {
			i = fifo32_get(&fifo);
			//i = get_a_key(&fifo);
			io_sti();
			if (key_win != 0 && key_win->flags == 0) {	/* ウィンドウが閉じられた */
				if (shtctl[shtctl_point]->top == 1) {	/* もうマウスと背景しかない */
					key_win = 0;
				} else {
					key_win = shtctl[shtctl_point]->sheets[shtctl[shtctl_point]->top - 1];
					keywin_on(key_win);
				}
			}
			if (256 <= i && i <= 511) { /* キーボードデータ */
				if(i == 0xe0 + 256){//e0扩展码
					s[0]=keytable2[i - 256];
				}
				else if (i < 0x80 + 256) { //标准编码
					if (key_shift == 0) {//shift状态
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {	/* 入力文字がアルファベット */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	/* 大文字を小文字に変換 */
					}
				}
				if (s[0] != 0 && key_win != 0) { /* 通常文字、バックスペース、Enter */
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}
				if (i == 256 + 0x0f && key_win != 0) {	/* Tab */
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl[shtctl_point]->top - 1;
					}
					key_win = shtctl[shtctl_point]->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a) {	/* 左シフト ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* 右シフト ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* 左シフト OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* 右シフト OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				//这里是有问题的，没有考虑左右ctrl分别处理的情况
				if(i==256+0x1d){//ctrl on
					key_ctrl|=1;
				}
				if(i==256+0x9d){//ctrl off
					key_ctrl&=~1;
				}
				if(i==256+0x38){//alt left
					key_alt|=1;
				}
				if(i==256+0x38+0x80){//alt left
					key_alt&=~1;
				}
				if(key_alt!=0 && i>=(0x3b+256) && i<=(0x44+256)){//ctrl + shfit + 1~9
					int index=i-0x3b-256;//0~8
					if(index<*(int*)0x0026f01c){//正确的切换范围
						if(sheet_mouse_on!=0){//有一个窗口被选择
							sheet_updown(sheet_mouse_on,-1);//隐藏原图层
							sheet_mouse_on->ctl=shtctl[index];//链接到新的图层控制器
							sheet_mouse_on->height=-1;//保持-1;
							sheet_updown(sheet_mouse_on,shtctl[index]->top+1);//移到顶端
							sheet_updown(sht_mouse,-1);//隐藏鼠标
							sht_mouse->ctl=shtctl[index];//链接到新的图层控制器
							sheet_updown(sht_mouse,254);//移到顶端
						}
						else{//没有选中图层就切换
							if (key_win != 0) {//释放窗口焦点
								keywin_off(key_win);
							}
							key_win=0;//当前没有窗口被选中
							sheet_updown(sht_mouse,-1);
							sht_mouse->ctl=shtctl[index];//链接到新的图层控制器
							sheet_updown(sht_mouse,254);//移到顶端
						}
						*(int*)0x0fe4=shtctl[index];
						shtctl_point=index;
						shtctl[index]->func.sheet_refreshsub(shtctl[index], 0, 0, shtctl[index]->xsize, shtctl[index]->ysize, 0, shtctl[index]->top);//刷新整个页面
					}
				}
				if (i == 256 + 0x3b && key_shift != 0 && key_win != 0) {	/* Shift+F1 */
					task = key_win->task;
					if (task != 0 && task->tss.ss0 != 0) {
						cons_putstr0(task->cons, "\nBreak(key) :\n");
						io_cli();	/* 強制終了処理中にタスクが変わると困るから */
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
						task_run(task, -1, 0);	/* 終了処理を確実にやらせるために、寝ていたら起こす */
					}
				}
				if (i == 256 + 0x3c && key_shift != 0) {	/* Shift+F2 */
					/* 新しく作ったコンソールを入力選択状態にする（そのほうが親切だよね？） */
					if (key_win != 0) {
						keywin_off(key_win);
					}
					key_win = open_console(shtctl[shtctl_point], memtotal);
					sheet_slide(key_win, 32, 32);
					sheet_updown(key_win, shtctl[shtctl_point]->top);
					keywin_on(key_win);
				}
				if (i == 256 + 0x57) {	/* F11 */
					sheet_updown(shtctl[shtctl_point]->sheets[1], shtctl[shtctl_point]->top - 1);
				}
				if (i == 256 + 0xfa) {	/* キーボードがデータを無事に受け取った */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	/* キーボードがデータを無事に受け取れなかった */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			} else if (512 <= i && i <= 767) { /* マウスデータ */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* 鼠标移动 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					new_mx = mx;
					new_my = my;
					for (j = shtctl[shtctl_point]->top - 1; j > 0; j--) {//遍历当前控制器所有图层,除了鼠标图层和背景图层
							sht = shtctl[shtctl_point]->sheets[j];//获取图层
							x = mx - sht->vx0;//基于图层的x坐标
							y = my - sht->vy0;//基于图层的y坐标
							if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {//鼠标位于其范围内
								if (sht->buf32[y * sht->bxsize + x] != sht->col_inv) {//透明色判断，点击区域不是透明色
									/*查看是否发生了鼠标当前位于图层的切换*/
									if(sheet_mouse_on_last!=sht&&sheet_mouse_on_last!=0&&sheet_mouse_on_last->task!=0&&sheet_mouse_on_last->task->fifo32_mouse_event!=0){//更换了图层
										fifo_mouse_put(&(sheet_mouse_on_last->task->fifom),0,0,mdec.btn)->sht=0;//传入鼠标数据
										fifo32_put(&(sheet_mouse_on_last->task->fifo),(sheet_mouse_on_last->task->fifo32_mouse_event));//传入鼠标事件标志
									}
									/*查看图层是否支持传入数据*/
									if(sht->task!=0 && sht->task->fifo32_mouse_event!=0){//允许传入鼠标数据
										io_cli();
										fifo_mouse_put(&(sht->task->fifom),x,y,mdec.btn)->sht=sht;//传入鼠标数据
										fifo32_put(&(sht->task->fifo),(sht->task->fifo32_mouse_event));//传入鼠标事件标志
										io_sti();
									}
									sheet_mouse_on_last=sht;
									break;
								}
							}
					}
					
					if ((mdec.btn & 0x01) == 0){//左键抬起
						sheet_mouse_on=0;//释放锁
						mouse_on_header=0;
						//sheet_updown(sht_mouse,254);//移到顶端
					}
					if ((mdec.btn & 0x01) != 0) {//左键按下
						/* 左ボタンを押している */
						if (mmx < 0) {
							/* 通常モードの場合 */
							/* 上の下じきから順番にマウスが指している下じきを探す */
							for (j = shtctl[shtctl_point]->top - 1; j > 0; j--) {//遍历当前控制器所有图层,除了鼠标图层和背景图层
								sht = shtctl[shtctl_point]->sheets[j];//获取图层
								x = mx - sht->vx0;//基于图层的x坐标
								y = my - sht->vy0;//基于图层的y坐标
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {//鼠标位于其范围内
									if (sht->buf32[y * sht->bxsize + x] != sht->col_inv) {//透明色判断，点击区域不是透明色
										if(sheet_mouse_on!=sht && sheet_mouse_on!=0){//不是同一个图层,且鼠标当前有图层选择
											break;
										}
										//if((sht->flags)&0x02!=0x02){//图层锁定位没有生效
										//}
										if(sheet_mouse_on==0){//此代码只在点击时触发一次
											sheet_updown(sht, shtctl[shtctl_point]->top - 1);//当前图层上移
											if (sht != key_win) {//点击的窗口不是当前活动窗口
												if(key_win!=0){//当前有活动窗口
													keywin_off(key_win);//取消活动状态
												}
												key_win = sht;//焦点位于新窗口
												keywin_on(key_win);
											}
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21 && mouse_on_header==0) {//鼠标没有命中锁定图层位于标题栏区域进入窗口移动移动模式
											mmx = mx;
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
											//sheet_updown(sht_mouse,-1);//移到底端
										}else{
											mouse_on_header=-1;//禁用窗口移动模式
										}
										if (sht->ctl->vram4sht==0 && sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {//图层是基础图层鼠标位于X按钮位置强制结束程序
											/* 「×」ボタンクリック */
											if ((sht->flags & 0x10) != 0) {		//应用程序的窗口
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli();	/* 强制结束期间关闭中断 */
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app+get_this();
												io_sti();
												task_run(task, -1, 0);
											} else {	//是一个命令行窗口，向其发送终止信号
												task = sht->task;
												//sheet_updown(sht, -1); /* とりあえず非表示にしておく */
												//keywin_off(key_win);
												//key_win = shtctl[shtctl_point]->sheets[shtctl[shtctl_point]->top - 1];
												//keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										}
										sheet_mouse_on=sht;
										break;
									}
								}
							}
							if(j<=0){//鼠标没有命中任何一个窗口
								sheet_mouse_on=-1;//那就不再命中任何图层直到松手
							}
						} else {
							x = mx - mmx;	//窗口移动的计算
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;	//移动坐标的更新
						}
					} else {
						//左键释放
						mmx = -1;
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);//移动图层
							new_wx = 0x7fffffff;
						}
					}
				}
			} else if (768 <= i && i <= 1023) {	/* 命令行关闭 */
				close_console(shtctl[shtctl_point]->sid[i - 768]);
			} else if (1024 <= i && i <= 2023) {
				close_constask(taskctl->tasks0 + (i - 1024));
			} else if (2024 <= i && i <= 2279) {	/* 仅关闭命令行 */
				sht2 = shtctl[shtctl_point]->sid[i - 2024];
				for(i=0;i<(256 * 165+0xfff)>>12;i++){
					void* po=pageman_unlink_page_32(pageman,(int)(sht2->buf)+0x1000*i,1);
					//memman_free_page_32(pageman,po);
				}
				memman_free_4k(memman, (int) sht2->buf, 256 * 165);
				sheet_free(sht2);
			}
		}
	}
}

int get_a_key(struct FIFO32* fifo){
	io_cli();
	if (fifo32_status(fifo) == 0) {
		io_sti();
		task_sleep(task_now());
	}
	else{
		io_sti();
		return fifo32_get(fifo);
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle32(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 3); 
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle32(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 2);
	}
	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct PAGEMAN32 *pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	struct TASK *task = task_alloc(),*task_now0=task_now();
	int i;
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,cons_fifo,7, 1,0);//
	void *cons_fifo_mouse =  memman_alloc_4k(memman, 4096);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,cons_fifo_mouse,7, 1,0);//
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);//内存分配!!!
	pageman_link_page_32_m(pageman,task->cons_stack,7,0x10,0);//
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &console_task+((int)get_this());
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	task->task_sheet_max=8;//最大图层数量
	task->name="console";
	int p=memman_alloc_4k(memman,4096);
	int pp=memman_alloc_page_32(pageman);
	pageman_link_page_32(pageman,p,pp|7,1);
	for(i=0;i<4096;i++){
		*(char*)(p+i)=0;//清空页表
	}
	//memmam_link_page_32_m(pageman,p,0xfffff000,pp&7,1,1);//顶端页面映射自身
	*(int*)(p+0xfff-3)=pp|7;
	for(i=0xc00;i<=0xfff-4;i++){
		*(char*)(p+i)=*(char*)(0xfffff000+i);//复制高端页表
	}
	for(i=0x00;i<3;i++){
		*(int*)(p+i*4)=*(int*)(0xfffff000+i*4);//复制低端页表12M
	}
	task->tss.cr3=pp&0xfffff000;
	
	task->memman=memman_alloc_4k(memman,sizeof(struct MEMMAN));;//应用程序的内存控制器
	memmam_link_page_32_m(pageman,0x268000,task->memman,7,(sizeof(struct MEMMAN)+0xfff)>>12,0);
	memman_init(task->memman);
	memman_free(task->memman,0x00c00000,0xbfffffff);
	
	
	*((int *) (task->tss.esp + 4)) = (int) sht;
	*((int *) (task->tss.esp + 8)) = memtotal;
	//add_child_task(task_now0,task);//命令行是主进程的子进程
	//add_child_task(task_now(),task);//设置子进程
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	fifo_mouse_init(&task->fifom, 128, cons_fifo_mouse, task);
	return task;
}

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct PAGEMAN32 *pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 1024 * 768 *4);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,buf,7,768+10,0);//
	sheet_setbuf(sht, buf, 1024, 768, -1); /* 不使用透明色 */
	make_window32(buf, 1024, 768, "console", 0);
	make_textbox32(sht, 8, 28, 1024-8*2, 768-28*2, COL8_000000);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20;	/* カーソルあり */
	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	int i;
	task_sleep(task);
	//释放应用程序内存控制器
	for(i=0;i<((int)(sizeof(struct MEMMAN))+0xfff)>>12;i++){
		void* po=pageman_unlink_page_32(pageman,(int)(task->memman)+0x1000*i,1);
		//memman_free_page_32(pageman,po);
	}
	memman_free_4k(memman,task->memman,sizeof(struct MEMMAN));//释放应用程序的内存控制器占用的内存
	//释放栈
	for(i=0;i<(64 * 1024+0xfff)>>12;i++){
		void* po=pageman_unlink_page_32(pageman,(int)(task->cons_stack)+0x1000*i,1);
		//memman_free_page_32(pageman,po);
	}
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	//释放fifo
	for(i=0;i<(128*4+0xfff)>>12;i++){
		void* po=pageman_unlink_page_32(pageman,(int)(task->fifo.buf)+0x1000*i,1);
		//memman_free_page_32(pageman,po);
	}
	memman_free_4k(memman, (int) task->fifo.buf, 128 *4);
	task->flags = 0; /* task_free(task); の代わり */
	return;
}

void close_console(struct SHEET *sht)
{
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i;
	struct TASK *task = sht->task;
	for(i=0;i<(1024*768*4+0xfff)>>12;i++){
		void* po=pageman_unlink_page_32(pageman,(int)(sht->buf)+0x1000*i,1);
		//memman_free_page_32(pageman,po);
	}
	memman_free_4k(memman, (int) sht->buf, 1024*768*4);
	sheet_free(sht);
	close_constask(task);
	return;
}

void top_bar_task(struct SHTCTL *shtctl){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	int i;
	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
		}
	}
}

EFI_SYSTEM_TABLE* get_sys_table_addr(){
	return Systemtable_base;
}
EFI_GUID* get_var_guid(){
	return &varGuid;
}
void* get_this(){
	return gThis;
}