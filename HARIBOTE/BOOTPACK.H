//Thanks to ChatGPT 3.5 from OpenAI for the help
//Thanks to Claude from Slack for the help
#include<Uefi.h>

#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long
int dmg_write(void* buff,int lba28,int block_number,int divice);
EFI_SYSTEM_TABLE* get_sys_table_addr();
EFI_GUID* get_var_guid();
void test3();

//EFI分区的代码
#define gUidPartTypeEFI {0xC12A7328,0xF81F,0x11D2,{0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}
//数据分区的代码
#define gUidPartTypeBaseData {0xEBD0A0A2,0xB9E5,0x4433,{0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}}
//EFI分区表的表项
#pragma pack(1)
typedef struct{
	EFI_GUID type;//分区的类型id
	EFI_GUID guid;//分区自身的唯一id
	unsigned int start_lba_low;//分区开始的lba号码
	unsigned int start_lba_high;
	unsigned int end_lba_low;//分区结束（包含）的lba号码
	unsigned int end_lba_high;
	unsigned int status_low;//分区状态
	unsigned int status_high;//分区状态
	short name[36];//人类可读的UTF-16编码格式
}GPT_ITEM;
//阻止结构体内部对齐
#pragma pack(1)
typedef struct{
	unsigned char BS_jmpBOOT[3];
	unsigned char BS_OEMName[8];
	unsigned short BPB_BytesPerSec; //每扇区字节数（Bytes/Sector）
	unsigned char BPB_SecPerClus; //每簇扇区数（Sector/Cluster）
	unsigned short BPB_ResvdSecCnt; //Boot记录占用多少扇区,也就是FAT之前有多少扇区
	unsigned char BPB_NumFATs; //共有多少FAT表
	unsigned short BPB_RootEntCnt; //根目录区文件最大数
	unsigned short BPB_TotSec16; //	扇区总数
	unsigned char BPB_Media; //介质描述符
	unsigned short BPB_FATSz16;//每个FAT表所占扇区数
	unsigned short BPB_SecPerTrk;//每磁道扇区数（Sector/track）
	unsigned short BPB_NumHeads;//磁头数（面数）
	unsigned int BPB_HiddSec;//隐藏扇区数
	unsigned int BPB_TotSec32;//如果BPB_TotSec16=0,则由这里给出扇区数
	unsigned int BPB_FATSz32;//每个FAT表所占扇区数
	unsigned short BS_BootSig;
	unsigned short Var;
	unsigned int BPB_Root;//文件根目录的起始簇号
	//剩下的部分不使用，因此不解析
}FAT32_HEADER;
//FAT32_FILE_INFO是32字节结构
#pragma pack(1)
typedef struct {
	unsigned char name[8];
	unsigned char ext[3];
	unsigned char type; //1:只读 2：隐藏 4：系统 8：卷标 0x10：子目录 0x20：归档
	unsigned char res1;
	unsigned char creat_time_ms;
	unsigned short create_time;
	unsigned short create_data;
	unsigned short last_found;
	unsigned short start_high;
	unsigned short change_time;
	unsigned short change_data;
	unsigned short start_low;
	unsigned int file_size;
}FAT32_FILE_INFO;
#pragma pack(1)
typedef struct{//设备信息
	unsigned int divice_index;//设备编号
}BLUEOS_STORAGE_INFO;
#pragma pack(1)
typedef struct{
	unsigned short name[64];//文件名
	unsigned int device;//所属设备
	unsigned int size_low;
	unsigned int size_high;
}BLUEOS_FILE_INFO;
//FAT32_LONG_NAME是32字节结构
#pragma pack(1)
typedef struct {
	char type;
	unsigned short name1[5];
	unsigned char type2;//0x0f
	unsigned char res1;//保留
	unsigned char chk;//校验码
	unsigned short name2[6];
	unsigned char mode;
	unsigned short start;//通常是0
	unsigned short name3[2];
}FAT32_LONG_NAME;
/* asmhead.nas */
#pragma pack(1)
struct BOOTINFO { /* 0x0ff0-0x0fff */
	char cyls; /* 引导扇区读取了磁盘的哪一部分 */
	char leds; /* ブート時のキーボードのLEDの状態 */
	char vmode; /* 视频模式下使用了多少位色彩深度 */
	char reserve;
	short scrnx, scrny; /* 画面解像度 */
	void *vram;
	int mode;//显示模式
	/*
	*1. 调色板
	*2. 24色全彩色
	*3. 32色全彩色
	*/
};
#define ADR_BOOTINFO	0x00000ff0
#define ADR_DISKIMG		0x00100000
#pragma pack(1)
struct EBOOTINFO{
	int cpu_info;
	int cpu_num;
};
#pragma pack(1)
struct MEMINFO {
	unsigned int base_addr_low;
	unsigned int base_addr_high;
	unsigned int length_low;
	unsigned int length_high;
	unsigned int type;
	unsigned int index;
	unsigned int undefind1;
	unsigned int undifind2;
};
#define ADR_MEMINFO		0x0026a000
#define ADR_PAGEMAN 	0x0026f000
/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
int io_out8(int port, int data);
int io_in16(int port);
int io_out16(int port, int data);
unsigned long long io_load_eflags(void);
int io_store_eflags(unsigned long long eflags);
int load_gdtr(unsigned long long limit, unsigned long long addr);
int load_idtr(unsigned long long limit, unsigned long long addr);
unsigned long long load_cr0(void);
unsigned long long store_cr0(unsigned long long cr0);
unsigned long long  load_cr3(void);
unsigned long long  store_cr3(unsigned long long cr3);
void load_tr(unsigned long long tr);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler2c(void);
void asm_inthandler0e(void);
void asm_inthandler34(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(unsigned long long eip, unsigned long long cs, unsigned long long esp, unsigned long long ds, unsigned long long *tss_esp0);
void asm_end_app(void);
int support_apic(void);
void io_wrmsr(unsigned long long ecx,unsigned long long edx,unsigned long long eax);
void io_rdmsr(unsigned long long ecx,unsigned long long* edx,unsigned long long* eax);
void test2(int eax,int ebx);
void store_gdt(void* addr);
void store_idt(void* addr);
void sys_reboot(void);
void sys_call(void);
int rdrand();
unsigned int  asm_sse_strcmp(char* s,char* t,unsigned int longer);
void asm_task_switch32(unsigned int *esp_s,unsigned int* esp_t);
void asm_console_task_start_64();
unsigned long long asm_get_cs();
unsigned long long store_cr4();
int load_cr4(unsigned long long cr4);
void asm_memcpy(unsigned long long rdi,unsigned long long rsi,unsigned long long rcx);
//void memset(char* s,int num,int longer);
//void strcpy(char* s,char* t,int longer);
//int strcmp(char* s,char* t,int longer);

/* fifo.c */
#pragma pack(1)
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* fifo_mouse.c */
#pragma pack(1)
typedef struct _MOUSESTATUS{
	UINTN x,y,btn;
	struct SHEET* sht;
}MOUSESTATUS;
#pragma pack(1)
typedef struct _FIFOMOUSE {
	MOUSESTATUS *buf;
	int p, q, size, free, flags;
	struct TASK *task;
}FIFOMOUSE;

void fifo_mouse_init(struct _FIFOMOUSE *fifo, int size, MOUSESTATUS *buf, struct TASK *task);
MOUSESTATUS* fifo_mouse_put(struct _FIFOMOUSE *fifo, UINTN x,UINTN y ,UINTN btn);
MOUSESTATUS* fifo_mouse_get(struct _FIFOMOUSE *fifo);
int fifo_mouse_status(struct _FIFOMOUSE *fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void boxfill32(unsigned int *vram, int xsize, unsigned int c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void init_screen32(int *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont32(int *vram, int xsize, int x, int y, int c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void putfonts32_asc(int *vram, int xsize, int x, int y, int c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void init_mouse_cursor32(int *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);
void putblock8_32(int *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, int *buf, int bxsize);
#define COL8_000000		0
#define COL8_FF0000		0x00FF0000
#define COL8_00FF00		0x0000ff00
#define COL8_FFFF00		0x00ffff00
#define COL8_0000FF		0x000000ff
#define COL8_FF00FF		0x00ff00ff
#define COL8_00FFFF		0x0000ffff
#define COL8_FFFFFF		0x00ffffff
#define COL8_C6C6C6		0x00c6c6c6
#define COL8_840000		0x00840000
#define COL8_008400		0x00008400
#define COL8_848400		0x00848400
#define COL8_000084		0x00000084
#define COL8_840084		0x00840084
#define COL8_008484		0x00008484
#define COL8_848484		0x00848484

/* dsctbl.c */
#pragma pack(1)
struct SEGMENT_DESCRIPTOR {
			short limit_low, base_low;
			char base_mid, access_right;
			char limit_high, base_high;
};
#pragma pack(1)
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
	unsigned int offset_64;
	unsigned int res;
};
void init_gdtidt(UINTN main_this);
void asm_x64_cs_init(unsigned long long cs_sel);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, unsigned long long base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, unsigned long long offset, int selector, int ar);
#define ADR_IDT			0x0026f000
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT			0x0082
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e
#define AR_CODE64_ER	0x209a

/* int.c */
void init_pic(void);
void io_write_io_apic(unsigned char  index,unsigned int data);
void init_apic(void* addr);
void io_ipi_message(void *apic_base,unsigned int high,unsigned int low);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

/* mouse.c */
#pragma pack(1)
struct MOUSE_DEC {
	unsigned char buf[4], phase;
	int x, y, btn,wheel;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
int mouse_decode_wheel(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c */
#define SYS_CE3_ADDRESS 0x00268000
#define SYS_CE3_ADDRESS_full 0x00269000
#define MEMMAN_FREES		4090	/* これで約32KB */
#define MEMMAN_ADDR			0x003c0000
#define MEMMAN_CORE_ADDR	0x0026f014//指针位置
#pragma pack(1)
struct FREEINFO {	/* あき情報 */
	unsigned long long addr, size;
};
#pragma pack(1)
struct MEMMAN {		/* メモリ管理 */
	unsigned long long frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
#pragma pack(1)
struct PAGEMAN32 {
	unsigned int free_page_num;
	unsigned int total_page_num;
	unsigned char* mem_map_base; 
};

struct inthandler0e_handling_info{
	unsigned long long cr2;
	unsigned long long cr3;
	struct TASK* task;
};

struct inthandler0e_handling_function{
	unsigned long long handling_number;
	struct handling{
		unsigned long long handling_index;
		long long (*function)(struct inthandler0e_handling_info* p);
	}handling[0];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned long long memman_total(struct MEMMAN *man);
unsigned long long memman_alloc(struct MEMMAN *man, unsigned long long size);
int memman_free(struct MEMMAN *man, unsigned long long addr, unsigned long long size);
unsigned long long memman_alloc_4k(struct MEMMAN *man, unsigned long long size);
int memman_free_4k(struct MEMMAN *man, unsigned long long addr, unsigned long long size);
void init_page(struct PAGEMAN32 *man);
unsigned int memmam_link_page_32_m(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address,unsigned int physical_address,int page_num,int mode);
unsigned int memman_link_page_32(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address,unsigned int physical_address,int mode);
unsigned int memman_alloc_page_32(struct PAGEMAN32 *man);
unsigned int memman_free_page_32(struct PAGEMAN32 *man,unsigned int physical_address);
unsigned int pageman_link_page_32(struct PAGEMAN32 *man,unsigned int linear_address,unsigned int physical_address,int mode);
unsigned int pageman_link_page_32_m(struct PAGEMAN32 *man,unsigned int linear_address,unsigned int physical_address,int page_num,int mode);
unsigned int pageman_unlink_page_32_m(struct PAGEMAN32 *man,unsigned int linear_address,int page_num,int mode);
unsigned pageman_unlink_page_32(struct PAGEMAN32 *man,unsigned int linear_address,int mode);
unsigned long long memman_link_page_64(struct PAGEMAN32* pageman,unsigned long long cr3_address,unsigned long long linear_address,unsigned long long physical_address,unsigned int mode);
unsigned int memman_link_page_64_m(struct PAGEMAN32 *man,unsigned long long cr3_address,unsigned long long linear_address,unsigned long long physical_address,int page_num,int mode);
int inthandler0e(unsigned long long cr2,unsigned long long* esp);
unsigned int get_physical_by_linear_32(unsigned int linear_address);
unsigned long long pageman_get_physical_address(unsigned long long cr3_address,unsigned long long linear_address);
void pageman_set(struct PAGEMAN32* pageman);
void* pageman_get();
unsigned long long memman_alloc_page_64_4k(struct PAGEMAN32 *man);
unsigned long long memman_alloc_page_64_4m(struct PAGEMAN32 *man);
void memman_free_page_64_4k(struct PAGEMAN32 *man,unsigned long long addr);
void memman_free_page_64_4m(struct PAGEMAN32 *man,unsigned long long addr);
/* sheet.c */
#define MAX_SHEETS		256
#define SHEET_MOUSE_ENABLE 0x80000000
#pragma pack(1)
struct SHEET {
	union{
		void *buf;
		unsigned int *buf32;
	};
	void* buf4clear;//透明通透的结果
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	int sid;//图层在图层控制器中的id
	/*
	flags:0x10 应用程序图层 0x01图层有效,0x02,锁定的图层,0x04 图层通透性开启,0x80000000 鼠标通透性开启
	*/
	struct SHTCTL *ctl;//属于哪一个图层管理器
	struct SHTCTL *ctl_from;//本图层是否由另一个图层控制器生成?
	struct TASK *task;//属于哪一个任务
	int red_size;//图层红色部分大小bit
	int grenn_size;//
	int yellow_size;//
	unsigned int colordept;
	unsigned int* (*mouse_down)(struct SHEET* self,int x,int y);//鼠标按下事件
	unsigned int* (*mouse_up)(struct SHEET* self,int x,int y);//鼠标抬起事件
	unsigned int* (*mouse_move)(struct SHEET* self,int x1,int y1,int x2,int y2);//鼠标移动事件
	unsigned int* (*get_sign)(struct SHEET* self);//获取鼠标焦点
	unsigned int* (*lost_sign)(struct SHEET* self);//失去鼠标焦点
	struct VIEW_FUNC (*func);
};
#pragma pack(1)
struct DISPLAY{
	void (*sheet_refreshsub)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);//局部刷新函数
};
#pragma pack(1)
struct SHTCTL {
	void *vram;//图层管理器输出buff
	void *map;//图层排布buff(总体显示情况)
	struct SHEET* sid[256];//id分配情况
	unsigned char* opacity;//map图层透明度
	int xsize, ysize, top;
	int sheets0_size;//公共图层数据数组包含的图层数量
	struct SHEET *sheets[MAX_SHEETS];//本图层控制器控制的图层列表
	struct View *sheets0;//公共图层数据数组
	struct SHEET *vram4sht;//该图层控制器的输出是否接到了一个图层上
	struct DISPLAY func;//操作函数
	unsigned int colordept;//色彩深度
};
#pragma pack(1)
struct SHEET_REFRESH_PACK_32{
	unsigned long long from_vram_addr;
	unsigned long long to_vram_addr;
	unsigned long long from_xsize;
	unsigned long long to_xsize;
	unsigned long long x_width;//刷新窗口宽度
	unsigned long long y_width;//刷新窗口高度
	unsigned long long to_sid_map;
	unsigned long long from_sid;
	unsigned long long from_sid_ext;
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman,struct PAGEMAN32 *pageman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refresh_all(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
void sheet_refreshsub24(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshsub32(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);

/*view.c*/
#pragma pack(1)
struct View {//由SHEET类派生而来 type：1
	struct SHEET sht;
	struct View* top;//上层控件
	struct View* next;//下一个控件
	struct View* child;//子控件
	int background_color;//背景色
};
void view_init(struct View* view);
void view_setfunc(struct View* self,struct VIEW_FUNC* func);
void view_flush(struct View* self,int x0,int y0,int x1,int y1);//控制器刷新函数
#pragma pack(1)
struct VIEW_FUNC{
	void* (*mouse_down)(struct View* self,int x,int y);//鼠标按下事件
	void* (*mouse_up)(struct View* self,int x,int y);//鼠标抬起事件
	void* (*mouse_move)(struct View* self,int x1,int y1,int x2,int y2);//鼠标移动事件
	void* (*get_sign)(struct View* self);//获取鼠标焦点
	void* (*lost_sign)(struct View* self);//失去鼠标焦点
};
#pragma pack(1)
struct TextView{
	struct View view;
	char* text;//文本内容
	int color;
	int point;//要显示的位置
	int ix,iy;//文本框输入焦点位置
};
void textview_init(struct TextView* self,char* text);
void textview_settext(struct TextView* self,char* text);
void textview_flush(struct TextView* self);
struct Button{
	struct View view;
	char* text;//显示的文本
	int text_color;//文本颜色
};
void button_init(struct Button self);
void button_flush(struct Button* self);
//void listview_init(struct ListView self,struct List list);
//void listview_additem(struct ListView self,struct Item item);
struct List{//列表类
	int index;//当前选中的目录项目
	int num;//目录项数量
	int item_size;//目录项指针大小
	void* list;//目录项列表
};
struct ListView{//列表基础类
	struct View view;
	struct List list;
};
void list_init(struct List self,int item_size);
struct Item{//选项基础类
	struct ListView* listview;
	struct Item* next;
	struct Item* up;
	char* text;//选项文本
	char* info;//选项描述
};
void item_init(struct Item self);
void item_flush();
struct MenuView{//菜单控件
	struct ListView listview;
};
struct task_abort{//应用程序终止列表
	struct task_abort* next;
	struct TASK* task;//要终止的任务
};

/* timer.c */
#define MAX_TIMER		500
#pragma pack(1)
struct TIMER {
	struct TIMER *next;
	unsigned int timeout, timeout64;
	/*flags2
	0:timeout是否被解释为高精度定时器计数值
	*/
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
#pragma pack(1)
struct TIMERCTL {
	unsigned int count, count64, next, next64;
	unsigned int timer_type;//计数器类型
	/*关于计数器类型
	1. 采用固定时钟源
	*/
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
	unsigned int fps;//本定时器一秒产生多少次中断,系统组件需要根据这个值评估自身申请的定时器timeout的大小,填写近似值也被允许
	EFI_STATUS (*timer_second2count)(unsigned int *time64,unsigned int *time);//这个函数将标准定点时间秒转换为计数值 定点格式为 time64.time
	EFI_STATUS (*timer_msecond2count)(unsigned int *time64,unsigned int *time);//这个函数将标准定点时间毫秒转换为计数值
};
extern struct TIMERCTL timerctl[8];
void init_pit(void);
void init_hpet_timer(void);
unsigned int timer_get_fps(unsigned int index);
struct TIMER *timer_alloc(unsigned int index);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(unsigned int index,struct TIMER *timer, unsigned int timeout,unsigned int timeout64);
void inthandler20(int *esp);
void inthandler34(int *esp);
int timer_cancel(unsigned int index,struct TIMER *timer);
void timer_cancelall(unsigned int index,struct FIFO32 *fifo);
EFI_STATUS timer_wait(unsigned int time_of_ms,unsigned int data);
/*taskmsg.c*/
#pragma pack(1)
typedef struct _TASK_MSG{
	unsigned int type;//消息类型
	void* name;//消息名称
	unsigned int name_len;//名称长度
	void* body;//消息正文
	unsigned int body_len;//正文长度
	unsigned int sender_pid;//发送者进程id
	unsigned int c;//接收者进程id
	struct _TASK_MSG* next;//下一个消息
}TASK_MSG;
#pragma pack(1)
typedef struct _TASK_MSG_MUSK{//消息来源
	unsigned int pid;
	struct _TASK_MSG_MUSK* next;
}TASK_MSG_MUSK;
#pragma pack(1)
typedef struct _TASK_MSG_BOX{
	unsigned int fifo_sel;//当有消息时向fifo发送哪个编号
	unsigned int num;//消息数量
	struct _TASK_MSG* msg;
	unsigned int flag;//0：使能消息进入 1：musk列表类型（0：阻止列表 1：允许列表）
	struct _TASK_MSG_MUSK* musk;
}TASK_MSG_BOX;

/* mtask.c */
#define MAX_TASKS		1000	/* 最大タスク数 */
#define TASK_GDT0		20		/* TSSをGDTの何番から割り当てるのか */
#define MAX_TASKS_LV	100
#define MAX_TASKLEVELS	10
#pragma pack(1)
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
struct TSS64{
	unsigned int res1;
	unsigned long long rsp0;
	unsigned long long rsp1;
	unsigned long long rsp2;
	unsigned long long res2;
	unsigned long long ist[7];
	unsigned long long res3;
	unsigned int iomap;
	//剩下的部分是TSS段外数据
	unsigned long long rsp;
	unsigned long long rip;
	unsigned long long rbx;
	unsigned long long rsi;
	unsigned long long rdi;
	unsigned long long rbp;
	unsigned long long cr3;
	unsigned long long rflages;
	unsigned long long ldtr;
};
//flags_a 0:任务是0否1是第一次运行 1:任务发生资源连锁是1否报错
#pragma pack(1)
struct TASK {
	/*flages_a
	0:read only任务首次启动
	1:任务是否要求快速切换
	2:read only任务的上一次切换是否是抢占切换 置位情况下 当前任务是被中断事件抢占而放弃处理器 否则任务是主动放弃处理器
	3:任务待摧毁标志 置位情况下切换到该任务 会立即摧毁这个任务
	*/
	unsigned long long sel, flags,flags_a; /* selはGDTの番号のこと */
	int level, priority;
	struct FIFO32 fifo;
	FIFOMOUSE fifom;
	struct _TASK_MSG_BOX msg_box;
	struct TSS64 tss;
	struct SEGMENT_DESCRIPTOR ldt[4];
	struct CONSOLE *cons;
	unsigned long long int ds_base, cons_stack;
	struct FILEHANDLE *fhandle;//打开的文件列表
	int *fat;
	char *cmdline;
	unsigned char langmode, langbyte1;
	struct SHEET *task_sheet[16];//图层列表
	int task_sheet_max;
	struct TASK* brother_task;//兄弟任务
	struct TASK* child_task;//子任务
	struct TASK* father_task;//父任务
	int index;//进程识别码
	struct MEMMAN* memman;//应用程序的内存管理器
	struct TASKCTL* taskctl;//应用程序的多线程管理器
	struct TIMERCTL* timerctl;//应用程序定时器管理器
	struct SHTCTL* shtctl;//应用程序图层管理器
	int id_high;//任务唯一id值高位
	int id_low;//任务唯一id值低位
	unsigned long long mem_use;//内存使用量
	unsigned long long root_dir_addr;//文件根目录
	char *name;//进程名称
	unsigned int fifo32_mouse_event;//鼠标事件发生时向FIFO传入的数据的base
	UINTN mouse_x;//鼠标位置
	UINTN mouse_y;//鼠标位置
	UINTN mouse_btn;//鼠标按键状态
	struct SHEET* sheet_mouse_on;//鼠标位于那个图层之上
};
#pragma pack(1)
struct TASKLEVEL {
	int running; /* 当前级别运行的任务数量 */
	int now; /* 现在正在运行的任务编号 */
	struct TASK *tasks[MAX_TASKS_LV];//当前级别任务数组
};
#pragma pack(1)
struct TASKCTL {
	int now_lv; /* 现在运行中的任务级别 */
	char lv_change; /* 下一次要切换到的任务级别 */
	int id_high;//下一个任务id高位
	int id_low;//下一个任务id低位
	struct TASKLEVEL level[MAX_TASKLEVELS];//任务级别数组
	struct TASK tasks0[MAX_TASKS];//所有任务池
	struct TSS64 tss;//正在运行的任务的tss
	
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASKCTL *task_ctl_now(void);
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
unsigned int task_get_esp0();
/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);
void make_window32(unsigned int *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht32(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox32(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle32(unsigned int *buf, int xsize, char *title, char act);
void change_wtitle32(struct SHEET *sht, char act);

/* console.c */
/*extend TASK*/
struct CONSOLE {
	struct SHEET *sht;
	struct TASK* task;
	int cur_x, cur_y, cur_c;//命令行输出坐标和光标颜色
	int font_x,font_y;//命令行输出行数和字数
	int font_x_max,font_y_max;//命令行的总x和总y
	struct TIMER *timer;
	int mode;/*命令行模式 0：普通文本模式*/
	char* message;/*message为cmd中的内容，根据mode进行相应解析*/
	int language;/*命令行所用的语言*/
	void* cons_stdout_mode;/*标准输出绑定情况*/
	void* cons_stderr_mode;/*标准错误绑定情况*/
};
struct FILEHANDLE {
	char *buf;
	long long size;
	long long pos;
};
typedef struct _SYSTEMTABLE_FILE{
	
}SYSTEMTABLE_FILE;
typedef struct _SYSTEMTABLE{
	EFI_STATUS (*stdout)(char* s);
	EFI_STATUS (*stderr)(char* s);
	EFI_STATUS (*stdin)(char* s);
	struct _SYSTEMTABLE_FILE *file;
}SYSTEMTABLE;
void console_init(struct CONSOLE* cons,struct SHTCTL* shtctl,unsigned int cons_x,unsigned int cons_y);
void __cdecl console_task(struct SHEET *sheet, int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_exit(struct CONSOLE *cons, int *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_langmode(struct CONSOLE *cons, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);
void cmd_rdrand(struct CONSOLE *cons, int memtotal);
void cons_set_system_task(struct TASK* task);
/* file.c */
#pragma pack(1)
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};
struct vfs_FILEINFO{
	int type;;/*
	0:存在标志
	1:系统保留文件(不可被侦测，不可通过普通手段修改)
	2:目录标志
	*/
	char* name;//名称
	int make_time;//创建时间
	int change_time;//更改世界
	int size_of_page;//文件页数量
	int node;//文件识别码
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
struct FILEINFO *file_full_search(char *name, struct FILEINFO *finfo, unsigned char type_have ,unsigned char type_havnt ,int max);
char *file_loadfile2(int clustno, int *psize, int *fat);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

/* bootpack.c */

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);

/*dav.c*/
struct DAVCTL{
	int num;//设备数量
	struct DAV* dav_list
};
struct DAV{
	int type;//0 无效设备 1 字符设备 2 块设备
	int index;//索引号
	void* fun;//设备驱动程序地址
	char name[32];//设备名称
	char bus;//总线号
	char device;//设备号
	char function;//功能号
};
void davctl_init(struct DAVCTL* ctl,int num);
/*desktop.c*/
struct SHEET* desktop_task(struct SHEET* sheet);
struct TASK *desktop_start();
/*system.c*/
struct TASK* system_start();
void system_task_abort(struct task_abort* task);
/* acpi.c */
#pragma pack(1)
struct ACPI_RSDP {
	char Signature[8];
	unsigned char Checksum;
	char OEMID[6];
	unsigned char Revision;
	unsigned int RsdtAddress;
	unsigned int Length;
	unsigned int XsdtAddress[2];
	unsigned char ExtendedChecksum;
	unsigned char Reserved[3];
};
#pragma pack(1)
struct ACPISDTHeader {//SDT表头部
  char Signature[4];
  unsigned int Length;
  unsigned char Revision;
  unsigned char Checksum;
  char OEMID[6];
  char OEMTableID[8];
  unsigned int OEMRevision;
  unsigned int CreatorID;
  unsigned int CreatorRevision;
};
#pragma pack(1)
struct ACPI_RSDT {
	struct ACPISDTHeader header;
	unsigned int Entry;
};
#pragma pack(1)
typedef struct
{
  unsigned char AddressSpace;
  unsigned char BitWidth;
  unsigned char BitOffset;
  unsigned char AccessSize;
  unsigned int Address[2];
} GenericAddressStructure;
#pragma pack(1)
struct ACPI_FADT//FADT表的格式
{
    struct   ACPISDTHeader h;
    unsigned int FirmwareCtrl;
    unsigned int Dsdt;//DSDT表格的位置
 
    // field used in ACPI 1.0; no longer in use, for compatibility only
    unsigned char  Reserved;
 
    unsigned char  PreferredPowerManagementProfile;//标识OME?
    unsigned short SCI_Interrupt;//开关机按钮的中断号
    unsigned int SMI_CommandPort;//启用ACPI或者禁用ACPI的端口号
    unsigned char  AcpiEnable;//把这个字段里的内容发送到偏移47字段指示的端口上就可以启用ACPI
    unsigned char  AcpiDisable;// 把这个字段里的内容发送到偏移47字段指示的端口上就可以禁用ACPI
    unsigned char  S4BIOS_REQ;
    unsigned char  PSTATE_Control;
    unsigned int PM1aEventBlock;// 事件寄存器1a (包括状态寄存器1a和使能寄存器1a)
    unsigned int PM1bEventBlock;//事件寄存器1b (包括状态寄存器1b和使能寄存器1b)
    unsigned int PM1aControlBlock;//控制寄存器1a
    unsigned int PM1bControlBlock;//控制寄存器1b
    unsigned int PM2ControlBlock;
    unsigned int PMTimerBlock;
    unsigned int GPE0Block;
    unsigned int GPE1Block;
    unsigned char  PM1EventLength;//事件寄存器1的大小
    unsigned char  PM1ControlLength;//事件寄存器1的长度?
    unsigned char  PM2ControlLength;//事件寄存器2的长度?
    unsigned char  PMTimerLength;  
    unsigned char  GPE0Length;
    unsigned char  GPE1Length;
    unsigned char  GPE1Base;
    unsigned char  CStateControl;
    unsigned short WorstC2Latency;
    unsigned short WorstC3Latency;
    unsigned short FlushSize;
    unsigned short FlushStride;
    unsigned char  DutyOffset;
    unsigned char  DutyWidth;
    unsigned char  DayAlarm;
    unsigned char  MonthAlarm;
    unsigned char  Century;
 
    // reserved in ACPI 1.0; used since ACPI 2.0+
    unsigned short BootArchitectureFlags;
 
    unsigned char  Reserved2;
    unsigned int Flags;
 
    // 12 byte structure; see below for details
    GenericAddressStructure ResetReg;//计算机重启端口
 
    unsigned char  ResetValue;//重启命令，写到上面端口可用重启计算机
    unsigned char  Reserved3[3];
 
    // 64bit pointers - Available on ACPI 2.0+
    unsigned int                X_FirmwareControl[2];
    unsigned int                X_Dsdt[2];
 
    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
};

void init_acpi(void);
int acpi_shutdown(void);
/*pci.c*/
#define PCI_CONFIG_PORT 0xCF8
#define PCI_DATA_PORT 0xCFC

#pragma pack(1)
typedef struct ahci_header_regs {

  unsigned int cap; 
  unsigned int ghc;
  unsigned int is;
  unsigned int pi;
  unsigned int vs;  

  unsigned int ccc_ctl; 
  unsigned int ccc_ports;
  unsigned int em_loc;  
  unsigned int em_ctl;

  unsigned int cap2; // 0x24
  unsigned int bohc; // 0x28
}AHCI_HEADER_REGS;

#pragma pack(1)
typedef struct ahci_abi_regs {
  unsigned int clb; //HBA_RPxCLB命令队列地址
  unsigned int clbu;
  unsigned int fb; //HBA_RPxFB 草稿纸FIS地址
  unsigned int fbu;
  unsigned int HBA_RPxIS; //HBA_RPxIS
  unsigned int HBA_RPxIE;
  unsigned int HBA_RPxCMD;
  unsigned int res;
  unsigned int HBA_RPxTFD;
  unsigned int HBA_RPxSIG;
  unsigned int HBA_RPxSSTS;
  unsigned int HBA_RPxSCTL;
  unsigned int HBA_RPxSERR;
  unsigned int HBA_RPxSACT;
  unsigned int HBA_RPxCI;
  unsigned int HBA_RPxSNTF;
  unsigned int HBA_RPxFBS;

} AHCI_ABI_REGS;
/*
#define HBA_RPBASE (0x40)
#define HBA_RPSIZE (0x80 >> 2)
#define HBA_RPxCLB 0
#define HBA_RPxFB 2
#define HBA_RPxIS 4
#define HBA_RPxIE 5
#define HBA_RPxCMD 6
#define HBA_RPxTFD 8
#define HBA_RPxSIG 9
#define HBA_RPxSSTS 10
#define HBA_RPxSCTL 11
#define HBA_RPxSERR 12
#define HBA_RPxSACT 13
#define HBA_RPxCI 14
#define HBA_RPxSNTF 15
#define HBA_RPxFBS 16
*/
#pragma pack(1)
typedef struct ahci_regs{
	union{
		AHCI_HEADER_REGS i;
		char space_char[0x100];
	}header;
	union{
		AHCI_ABI_REGS i;
		char space_char[0x80];
	}port[32];
}AHCI_REGS;
#pragma pack(1)
typedef struct pci_device{//设备信息编号
	int bus;
	int device;
	int function;
	int vendor_id;
	int device_id;
	int class_code;
	int subclass_code;
}PCI_DEV;

typedef struct {
    unsigned short GeneralConfiguration;            // 0
    unsigned short NumberOfCylinders;               // 1
    unsigned short Reserved1;                       // 2
    unsigned short NumberOfHeads;                   // 3
    unsigned short UnformattedBytesPerTrack;        // 4
    unsigned short UnformattedBytesPerSector;       // 5
    unsigned short SectorsPerTrack;                 // 6
    unsigned short VendorUnique1[3];                // 7-9
    char  SerialNumber[20];                // 10-19
    unsigned short BufferType;                      // 20
    unsigned short BufferSectorSize;                // 21
    unsigned short NumberOfECCBytes;                // 22
    char  FirmwareRevision[8];             // 23-26
    char  ModelNumber[40];                 // 27-46
    unsigned short MaximumSectorsPerInterrupt;      // 47
    unsigned short Reserved2;                       // 48
    unsigned short Capabilities1;                   // 49
    unsigned short Capabilities2;                   // 50
    unsigned short PIODataTransferCycleTiming;      // 51
    unsigned short Reserved3;                       // 52
    unsigned short FieldValidity;                   // 53
    unsigned short CurrentNumberOfCylinders;        // 54
    unsigned short CurrentNumberOfHeads;            // 55
    unsigned short CurrentSectorsPerTrack;          // 56
    unsigned int CurrentSectorCapacity;           // 57-58
    unsigned short MultipleSectorSetting;           // 59
    unsigned int TotalAddressableSectors;         // 60-61
    unsigned short SingleWordDMA;                   // 62
    unsigned short MultiWordDMA;                    // 63
    unsigned short AdvancedPIOModes;                // 64
    unsigned short MinimumMWXferCycleTime;          // 65
    unsigned short RecommendedMWXferCycleTime;      // 66
    unsigned short MinimumPIOCycleTimeNoFlow;       // 67
    unsigned short MinimumPIOCycleTimeIORDY;        // 68
    unsigned short Reserved4[2];                    // 69-70
    unsigned short Reserved5[4];                    // 71-74
    unsigned short QueueDepth;                      // 75
    unsigned short Reserved6[4];                    // 76-79
    unsigned short MajorVersionNumber;              // 80
    unsigned short MinorVersionNumber;              // 81
    unsigned short CommandSetSupported1;            // 82
    unsigned short CommandSetSupported2;            // 83
    unsigned short CommandSetSupported3;            // 84
    unsigned short CommandSetEnabled1;              // 85
    unsigned short CommandSetEnabled2;              // 86
    unsigned short CommandSetDefault;               // 87
    unsigned short UltraDMAModes;                   // 88
    unsigned short Reserved7[167];                  // 89-255
} IDENTIFY_DEVICE_DATA;

#pragma pack(1)
typedef struct ahci_device{
	PCI_DEV pci_dev;
	unsigned long long fis_base[32];
	unsigned long long fis_page_base[2];
	unsigned  long long clb_base[32];
	unsigned  long long clb_page_base[8];
	AHCI_REGS* ahci_config_space_address;
	IDENTIFY_DEVICE_DATA* dev_info[32];
}AHCI_DEV;
#define ahci_command_header$flags$a (1<<5)
#define ahci_command_header$flags$w (1<<6)
#define ahci_command_header$flags$p (1<<7)
#define ahci_command_header$flags$r (1<<8)
#define ahci_command_header$flags$b (1<<9)
#define ahci_command_header$flags$c (1<<10)

#pragma pack(1)
typedef struct _ahci_command_header{//AHCI 设备的寄存器描述结构 每个描述一个设备
	unsigned short flags;//各种标志 0-4:CFL(DWORD) 5:A (ATAPI) 6:W (write) 7:P (enable ) 8:R ready 9:B (busy) 10:C (BSY auto clear) 11:Reserved 12-15:PMP
	unsigned short prdtl;//AHCI_PRDT 描述表长度
	unsigned int prdbc;//传输数据量(read only)
	unsigned int command_table_address_32;//描述表地址 _ahci_sata_fis 挂载于此
	unsigned int command_table_address_64;//描述表的64位版本地址
	unsigned int undifind[4];//未使用
}AHCI_COMMAND_HEADER;
#pragma pack(1)
typedef struct _ahci_cfis_0x27{
	char type;//必须填写为0x27
	char flag;
	char command;//命令
	char features;
	short lba15_0;//逻辑扇区地址
	char lba23_16;
	char device;
	short lba39_24;
	char lba47_40;
	char features2;
	short count;
	short reserved;
	unsigned int aux;
	/*接下来是一些补充信息*/
	unsigned int* prdbc_addr;
}AHCI_CFIS_0X27;
#pragma pack(1)
typedef struct _ahci_prdt{//数据块描述结构 描述设备和系统传输数据的地址
	unsigned int dba;//数据块地址
	unsigned int dbau;//数据块地址的64位版本
	unsigned int reserved;
	unsigned int dbc;//数据块大小
}AHCI_PRDT;
#pragma pack(1)
typedef struct _ahci_sata_fis{//SATA FIS的结构
	union{//CFIS结构
		AHCI_CFIS_0X27 ahci_cfis_0x27;
		char space[0x40];
	}cfis;
	union{//ACMD结构
		char space[0x40];
	}acmd;
	AHCI_PRDT prdt[0];//动态大小 
}AHCI_SATA_FIS;


#define ATA_IDENTIFY_DEVICE 0xec
#define ATA_READ_DMA 0xc8
#define ATA_READ_DMA_EX 0x25
#define ATA_WRITE_DMA 0xca
#define ATA_WRITE_DMA_EX 0x35
#define SCSI_IDENTIFY_DEVICE 0xa1
#define SCSI_READ_DMA 0xa8
#define SCSI_READ_DMA_EX 0x88
#define SCSI_WRITE_DMA 0xaa
#define SCSI_WRITE_DMA_EX 0x8a
#define SCSI_READ_CAPACITY 0x9e
unsigned int pcie_read_config(PCI_DEV* dev,int reg_offset); 
void pcie_write_config(PCI_DEV* dev,int reg_offset,unsigned int config);
unsigned int pcie_get_rcba();
typedef struct block_dev_function{
	EFI_STATUS (*data_move)(void* id,void* buff,unsigned long long lba,unsigned long long size,unsigned long long flags);
}BLOCK_DEV_FUNCTION;
//unsigned int read_pci_config8(struct pci_device pci_device, unsigned char reg_offset);
//unsigned int read_pci_config32(struct pci_device pci_device, unsigned char reg_offset);
/*bmp.c*/
void write_bmp(int* buff, int xsize, int ysize, char* buff2);
/*bmp2.obj*/
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);//env:图像数据保存与工作空间;info:8字节探测结果;size:文件大小;fp:文件内容指针
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);
/*dmg.obj*/
void dmg_init();
int dmg_read(char* buff,int lba28,int block_number,int divice);


void* get_this();
/*drvier.c*/
// 设备操作函数
typedef struct {
	unsigned int index;//偏移量
	int (*read)(char* buff,unsigned int lba28,unsigned int block_number,int divice);
	int (*write)(void* buff,unsigned int lba28,unsigned int block_number,int divice);
}vdisk;


// 存储设备抽象结构体
struct _StorageDeviceOperations;
typedef struct _StorageDevice{
    char* name; // 设备名称
    unsigned int size; // 设备大小
    unsigned free;//设备空闲空间
	int status;//设备状态
	int rand;//校验值，当设备被挂载时会填写
    struct _StorageDeviceOperations* driver; // 设备驱动程序
	
} StorageDevice;

typedef struct _StorageDeviceOperations{
    // 打开设备，返回0表示成功，返回-1表示失败
    int (*open)(StorageDevice* dev,int* user_index,char* name,int mode);//返回int user_index

    // 从设备中读取数据，返回实际读取的字节数，返回-1表示失败
    int (*read)(StorageDevice* dev,int user_index, void* buf, unsigned int* count);

    // 将数据写入设备，返回实际写入的字节数，返回-1表示失败
    int (*write)(StorageDevice* dev,int user_index, void* buf, unsigned int* count);

    // 关闭设备，返回0表示成功，返回-1表示失败
    int (*close)(StorageDevice* dev);

    // 定位设备的读写指针，返回指针位置，返回-1表示失败
    unsigned int (*seek)(StorageDevice* dev, int user_index,unsigned int* offset);
} StorageDeviceOperations;
void device_init();
unsigned int device_add(char* name,unsigned int size,unsigned int free,StorageDeviceOperations* driver);
unsigned int device_remove(unsigned int index);
unsigned int device_open(unsigned int device_index,int* rand,int* user_index,char* name,int mode);
unsigned int device_read(unsigned int device_index,int rand,int user_index,char* buff,unsigned int* count);
unsigned int device_write(unsigned int device_index,int rand,int user_index,char* buff,unsigned int* count);
unsigned int device_seek(unsigned int device_index,int rand,int user_index,int* count);
/*dmg.c*/
void task_disk();
struct TASK* start_task_disk();
unsigned int _dmg_open(StorageDevice* dev,int* user_index,char* name,int mode);
unsigned int _dmg_read(StorageDevice* dev,int user_index, void* buf, unsigned int* count);
unsigned int _dmg_write(StorageDevice* dev,int user_index, void* buf, unsigned int* count);
unsigned int _dmg_seek(StorageDevice* dev, int user_index,unsigned int* offset);
unsigned int _dmg_close(StorageDevice* dev);
#pragma pack(1)
/*ahci.c*/
/*0x80大小*/
#pragma pack(1)
typedef struct _AHCI_BAR6_PORT_TABLE{
	unsigned int PxCLB;
	unsigned int PxCLBU;
	unsigned int PxFB;
	unsigned int PxFBU;
	unsigned int PxIS;
	unsigned int PxIE;
	unsigned int PxCMD;
	unsigned int RES;
	unsigned int PxTFD;
	unsigned int PxSIG;
	unsigned int PxSSTS;
	unsigned int PxSCTL;
	unsigned int PxSERR;
	unsigned int PxSACT;
	unsigned int PxCI;
	unsigned int PxSNTF;
	unsigned int PxFBS;
	unsigned int PxDEVSLP;
	unsigned int RES2[10];
	unsigned int PxVS[4];
}AHCI_BAR6_PORT_TABLE;
/*0x40大小*/
#pragma pack(1)
typedef struct _AHCI_BAR6_TABLE{
	unsigned int cap;
	unsigned int ghc;
	unsigned int is;
	unsigned int pi;
	unsigned int vs;
	unsigned int ccc_ctl;
	unsigned int ccc_ports;
	unsigned int em_loc;
	unsigned int em_ctl;
	unsigned int cap2;
	unsigned int bohc;
	unsigned int UnUse[5];
}AHCI_BAR6_TABLE;

typedef struct _AHCI_TABLE{
	AHCI_DEV ahci_dev[8];
	int number;
}AHCI_TABLE;
typedef struct _ahci_command{
	unsigned long long id;
	unsigned long long lba;
	void* buff;
	unsigned long long block_count;
	void (*func)(struct _ahci_command* cmd);//回调函数
}AHCI_COMMAND;
typedef struct _ahci_command_list{
	AHCI_COMMAND* list[128];
}AHCI_COMMAND_LIST;
AHCI_TABLE* ahci_init_all();
EFI_STATUS ahci_init(AHCI_DEV* ahci_dev);
AHCI_SATA_FIS* ahci_make_fis(AHCI_SATA_FIS* fis,void* buff,unsigned long long lba,unsigned long long count,unsigned long long command,unsigned long long flag);
void ahci_fis_write_prdt(AHCI_SATA_FIS* fis,unsigned int index,unsigned long long address,unsigned long long count);
AHCI_SATA_FIS* ahci_fis_send(AHCI_DEV* dev,unsigned int ahci_abi_regs_index,AHCI_SATA_FIS* fis,unsigned long long prdtl,unsigned int flags);
typedef struct _ahci_IDENTIFY_DEVICE_info{
    unsigned short general_config;                // Word 0
    unsigned short num_cylinders;                 // Word 1
    unsigned short specific_config;               // Word 2
    unsigned short num_heads;                     // Word 3
    unsigned short retired[2];                    // Words 4-5
    unsigned short num_sectors_per_track;         // Word 6
    unsigned short vendor_unique[3];              // Words 7-9
    char serial_number[20];                       // Words 10-19
    unsigned short retired2[2];                   // Words 20-21
    unsigned short obsolete;                      // Word 22
    char firmware_revision[8];                    // Words 23-26
    char model_number[40];                        // Words 27-46
    unsigned short max_sectors_per_irq;           // Word 47
    unsigned short reserved;                      // Word 48
    unsigned short capabilities[2];               // Words 49-50
    unsigned short obsolete2[2];                  // Words 51-52
    unsigned short field_validity;                // Word 53
    unsigned short current_cylinders;             // Word 54
    unsigned short current_heads;                 // Word 55
    unsigned short current_sectors_per_track;     // Word 56
    unsigned int current_capacity_sectors;        // Words 57-58
    unsigned short current_sectors_per_irq;       // Word 59
    unsigned int total_addressable_sectors;       // Words 60-61
    unsigned short obsolete3;                     // Word 62
    unsigned short multiword_dma_support;         // Word 63
    unsigned short pio_modes_supported;           // Word 64
    unsigned short min_multiword_dma_cycle_time;  // Word 65
    unsigned short rec_multiword_dma_cycle_time;  // Word 66
    unsigned short min_pio_cycle_time;            // Word 67
    unsigned short min_pio_cycle_time_iordy;      // Word 68
    unsigned short reserved2[6];                  // Words 69-74
    unsigned short queue_depth;                   // Word 75
    unsigned short sata_capabilities;             // Word 76
    unsigned short sata_reserved;                 // Word 77
    unsigned short sata_feature_supported;        // Word 78
    unsigned short sata_feature_enabled;          // Word 79
    unsigned short major_version;                 // Word 80
    unsigned short minor_version;                 // Word 81
    unsigned short command_set_supported[3];      // Words 82-84
    unsigned short command_set_active[2];         // Words 85-86
    unsigned short ultra_dma_modes;               // Word 88
    unsigned short time_for_security_erase;       // Word 89
    unsigned short time_for_enhanced_security_erase; // Word 90
    unsigned short current_apm_level;             // Word 91
    unsigned short master_password_id;            // Word 92
    unsigned short hardware_reset_result;         // Word 93
    unsigned short acoustic_management;           // Word 94
    unsigned short stream_min_req_size;           // Word 95
    unsigned short streaming_transfer_time_dma;   // Word 96
    unsigned short streaming_access_latency;      // Word 97
    unsigned int streaming_performance_granularity; // Words 98-99
    unsigned long long max_lba_48bit;             // Words 100-103
    unsigned short streaming_transfer_time_pio;   // Word 104
    unsigned short reserved3;                     // Word 105
    unsigned short physical_logical_sector_size;  // Word 106
    unsigned short inter_seek_delay;              // Word 107
    unsigned short world_wide_name[4];            // Words 108-111
    unsigned short reserved4[4];                  // Words 112-115
    unsigned short reserved5;                     // Word 116
    unsigned int logical_sector_size;             // Words 117-118
    unsigned short command_set_support_ext;       // Word 119
    unsigned short command_set_active_ext;        // Word 120
    unsigned short reserved6[6];                  // Words 121-126
    unsigned short removable_media_status;        // Word 127
    unsigned short security_status;               // Word 128
    unsigned short vendor_specific[31];           // Words 129-159
    unsigned short cfa_power_mode;                // Word 160
    unsigned short reserved7[15];                 // Words 161-175
    char current_media_serial_number[60];         // Words 176-205
    unsigned short reserved8[49];                 // Words 206-254
    unsigned short integrity_word;                // Word 255
}ahci_identify_info;

/* com.c */
void com_out_string(unsigned int port,char* str);
/*sse.c*/
void init_sse42();
/*amtask.c*/
typedef struct _atask{
	struct TASK task;
	
}ATASK;
typedef struct _ataskctl{
	struct TASKCTL taskctl;
}ATASKCTL;
/*vmx.c*/

typedef struct _blueos_vmcs{
	void* vmcs;//VMCS字段
	void* ept;//扩展页表
} BLUEOS_VMCS;
/*cache.c*/

#define CACHE_BLOCK_IDLE  0
#define CACHE_BLOCK_ENABLE 1
#define CACHE_BLOCK_ACTIVE 2
#define CACHE_BLOCK_DIRTY 3

typedef struct {
    unsigned long long lba;
    unsigned int size; // Size in bytes
    char *buffer;
    unsigned long long valid; // 判断是否空闲
	unsigned long long flags;/*
	0: CACHE_BLOCK_DIRTY状态下禁止写回磁盘
	1.：此缓存块禁止被释放
	2：CACHE_BLOCK_ENABLE状态下不从磁盘读取内容而直接变为CACHE_BLOCK_ACTIVE
	*/
} CACHE_BLOCK;

typedef struct {
    CACHE_BLOCK* cache_blocks_point;
    char *sbuffer;//源变址
	char *dbuffer;//目的变址
    unsigned long long size; // Size in bytes
    unsigned long long valid; // 判断是否空闲
} REQUEST;


typedef struct {
    CACHE_BLOCK* cache_blocks_point;
	unsigned long long flags;
    unsigned long long valid; // 判断是否空闲
} SYNC_REQUEST;
#define CACHE_SIZE (4 * 1024 * 1024) // Example cache block size: 4 MB
#define MAX_CACHES 128       // Maximum number of cache blocks
#define MAX_REQUESTS 128      // Maximum number of requests
#define MAX_SYNC_REQUESTS 32 // Maximum number of sync requests

#define SYNC_REQUEST_WRITE 1
#define SYNC_REQUEST_READ 0

typedef struct {
	EFI_STATUS (*read)(void *this, void *buff, unsigned long long offset, unsigned long long size);
	EFI_STATUS (*write)(void *this, void *buff, unsigned long long offset, unsigned long long size);
	EFI_STATUS (*sync)(void *this);
	EFI_STATUS (*flush)(void* this);
    CACHE_BLOCK cache_blocks[MAX_CACHES];
    CACHE_BLOCK* cache_blocks_point[MAX_CACHES];
    REQUEST requests[MAX_REQUESTS];
    REQUEST* requests_point[MAX_REQUESTS];
    SYNC_REQUEST sync_requests[MAX_SYNC_REQUESTS];
    SYNC_REQUEST* sync_requests_point[MAX_SYNC_REQUESTS];
    unsigned int block_count;
    unsigned int request_count;
    unsigned int sync_request_count;
	union{
   		 PCI_DEV* pci_dev;
		 void* dev;
	};
    unsigned long long device_id;
} CACHE_TABLE;
void cache_init(CACHE_TABLE** this);
/*fat32.c*/
typedef struct _FILE_OF_FAT32{
	EFI_STATUS (*fread)(struct _FILE* this,void* buff,unsigned long long seek,unsigned long long size); \
	EFI_STATUS (*fwrite)(struct _FILE* this,void* buff,unsigned long long seek,unsigned long long size); \
	EFI_STATUS (*fsize)(struct _FILE* this,unsigned long long* size); \
	EFI_STATUS (*fopen)(struct _FILE* this,unsigned long long index); \
	EFI_STATUS (*fclose)(struct _FILE* this);

	unsigned long long node_id;//当前文件的起始FAT编号
	unsigned long long device_id;
	unsigned long long part_base_lba;
	unsigned long long part_end_lba;
	unsigned long long part_id;
	PCI_DEV* pci_dev;
	unsigned long long seek;
	unsigned long long fsizeof;
	unsigned int* fat32_fat;
	unsigned long long size;
	FAT32_HEADER* mbr;
	CACHE_TABLE* cache;
	
}FILE_OF_FAT32;
FILE_OF_FAT32* fat32_init(FILE_OF_FAT32* file_info,CACHE_TABLE* cache_table ,unsigned int device_id,unsigned long long part_base_lba);
EFI_STATUS fat32_read_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size);
/*efi.c*/
#pragma pack(push, 1)

// DOS Header
typedef struct {
    uint16_t e_magic;    // Magic number
    uint16_t e_cblp;     // Bytes on last page of file
    uint16_t e_cp;       // Pages in file
    uint16_t e_crlc;     // Relocations
    uint16_t e_cparhdr;  // Size of header in paragraphs
    uint16_t e_minalloc; // Minimum extra paragraphs needed
    uint16_t e_maxalloc; // Maximum extra paragraphs needed
    uint16_t e_ss;       // Initial (relative) SS value
    uint16_t e_sp;       // Initial SP value
    uint16_t e_csum;     // Checksum
    uint16_t e_ip;       // Initial IP value
    uint16_t e_cs;       // Initial (relative) CS value
    uint16_t e_lfarlc;   // File address of relocation table
    uint16_t e_ovno;     // Overlay number
    uint16_t e_res[4];   // Reserved words
    uint16_t e_oemid;    // OEM identifier (for e_oeminfo)
    uint16_t e_oeminfo;  // OEM information; e_oemid specific
    uint16_t e_res2[10]; // Reserved words
    uint32_t e_lfanew;   // File address of new exe header
} IMAGE_DOS_HEADER;

// PE Header
typedef struct {
    uint32_t Signature;
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

// Optional Header
typedef struct {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    uint64_t DataDirectory[16][2]; // Directory entries (Virtual Address, Size)
} IMAGE_OPTIONAL_HEADER64;

// Section Header
typedef struct {
    uint8_t  Name[8];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct {
	EFI_STATUS (*read)(void* this, void* buff, char* name,unsigned long long *size);
	IMAGE_DOS_HEADER* dosHeader;
	IMAGE_FILE_HEADER* fileHeader;
	IMAGE_SECTION_HEADER** sectionHeader;
	IMAGE_OPTIONAL_HEADER64* optionalHeader;
	void* buff;
}TYPEOF_PE;
#pragma pack(pop)
/*vmx.c */
#define MAX_VM_NAME_LENGTH 64

typedef struct {
    uint32_t vm_id;                       // 虚拟机唯一标识符
    char name[MAX_VM_NAME_LENGTH];        // 虚拟机名称
    uint8_t state;                        // 虚拟机状态（运行、暂停、停止等）
    
    // 内存相关
    uint64_t memory_size;                 // 分配给虚拟机的内存大小
    void* memory_start;                   // 虚拟机内存起始地址
    
    // CPU相关
    uint32_t vcpu_count;                  // 虚拟CPU数量
    uint64_t cr3;                         // CR3寄存器值（页表基址）
    
    // VMCS相关
    void* vmcs_region;                    // VMCS区域指针
    
    // 设备相关
    uint32_t disk_size;                   // 虚拟磁盘大小
    char* disk_image_path;                // 虚拟磁盘镜像路径
    
    // 网络相关
    uint8_t mac_address[6];               // 虚拟机MAC地址
    
    // 性能统计
    uint64_t total_run_time;              // 总运行时间
    uint64_t vm_exit_count;               // VM Exit次数
    
    // 调度相关
    uint32_t priority;                    // 调度优先级
    uint64_t time_slice;                  // 时间片长度
    
    // 错误处理
    uint32_t last_error_code;             // 最后一次错误代码
    char* last_error_message;             // 最后一次错误消息
    
    // 其他可能需要的字段...
    
} VirtualMachineInfo;
#define MAX_VIRTUAL_MACHINES 16

typedef struct {
    VirtualMachineInfo vm_list[MAX_VIRTUAL_MACHINES];  // 虚拟机列表
    uint32_t vm_count;                                 // 当前虚拟机数量
    uint32_t active_vm_id;                             // 当前活动虚拟机ID
    
    // 虚拟机管理函数指针
    EFI_STATUS (*create_vm)(struct VirtualMachineCTL* this, VirtualMachineInfo* vm_info);
    EFI_STATUS (*start_vm)(struct VirtualMachineCTL* this, uint32_t vm_id);
    EFI_STATUS (*stop_vm)(struct VirtualMachineCTL* this, uint32_t vm_id);
    EFI_STATUS (*pause_vm)(struct VirtualMachineCTL* this, uint32_t vm_id);
    EFI_STATUS (*resume_vm)(struct VirtualMachineCTL* this, uint32_t vm_id);
    EFI_STATUS (*delete_vm)(struct VirtualMachineCTL* this, uint32_t vm_id);
    
    VirtualMachineInfo* (*get_vm_info)(struct VirtualMachineCTL* this, uint32_t vm_id);
    EFI_STATUS (*set_vm_memory)(struct VirtualMachineCTL* this, uint32_t vm_id, uint64_t memory_size);
    EFI_STATUS (*set_vm_vcpu_count)(struct VirtualMachineCTL* this, uint32_t vm_id, uint32_t vcpu_count);
    
    // 资源管理
    uint64_t total_memory;        // 总可用内存
    uint64_t allocated_memory;    // 已分配内存
    uint32_t total_vcpu;          // 总可用vCPU数
    uint32_t allocated_vcpu;      // 已分配vCPU数
    
    // 性能监控
    uint64_t total_vm_exits;      // 所有虚拟机的VM Exit总数
    
    // 错误处理
    uint32_t last_error_code;     // 最后一次错误代码
    char* last_error_message;     // 最后一次错误消息
    
} VirtualMachineCTL;

// 初始化VirtualMachineCTL的函数声明
EFI_STATUS init_virtual_machine_ctl(VirtualMachineCTL* vm_ctl);
void vmx_exit_handler();