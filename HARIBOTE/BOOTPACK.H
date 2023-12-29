//Thanks to ChatGPT 3.5 from OpenAI for the help
//Thanks to Claude from Slack for the help
typedef unsigned int UINTN;
typedef UINTN EFI_HANDLE;
typedef UINTN EFI_STATUS;

typedef struct {
  unsigned int  Signature_low;
  unsigned int  Signature_high;
  unsigned int  Revision;
  unsigned int  HeaderSize;
  unsigned int  CRC32;
  unsigned int  Reserved;
} EFI_TABLE_HEADER;

typedef struct {
	unsigned int g1;
	unsigned short g2;
	unsigned short g3;
	unsigned char g4[8];
}EFI_GUID;

typedef struct {
  EFI_TABLE_HEADER                Hdr;
  //
  // Time Services
  //
  UINTN                    (*GetTime)(void* Time,void* Capabilities);
  UINTN                    (*SetTime)(void* Time);
  UINTN             (*GetWakeupTime)(void* Enabled,void* Pending,void* Time);
  UINTN             (*SetWakeupTime)(void* Enabled,void* Time);
  //
  // Virtual Memory Services
  //
  UINTN     (*SetVirtualAddressMap)(UINTN MemoryMapSize,UINTN DescriptorSize,unsigned int DescriptorVersion,void* VirtualMap);
  UINTN             (*ConvertPointer)(UINTN DebugDisposition,void** address);
  //
  // Variable Services
  //
  EFI_STATUS                (*GetVariable)(short* VariableName,EFI_GUID* VendorGuid,unsigned int* Attributes,UINTN* DataSize,void* Data);
  EFI_STATUS      (*GetNextVariableName)(UINTN* VariableNameSize,short* VariableName,EFI_GUID VendorGuid);
  EFI_STATUS                (*SetVariable)(short* VariableName,EFI_GUID* VendorGuid,unsigned int Attributes,UINTN DataSize,void* Data);
  //
  // Miscellaneous Services
  //
  EFI_STATUS    (*GetNextHighMonotonicCount)();
  EFI_STATUS                (*ResetSystem)();
  //
  // UEFI 2.0 Capsule Services
  //
  EFI_STATUS              (*UpdateCapsule)();
  EFI_STATUS  (*QueryCapsuleCapabilities)();
  //
  // Miscellaneous UEFI 2.0 Service
  //
  EFI_STATUS         (*QueryVariableInfo)();
} EFI_RUNTIME_SERVICES;

typedef struct {
  EFI_TABLE_HEADER                    	Hdr;
  short                              	*FirmwareVendor;
  unsigned int                      	FirmwareRevision;
  EFI_HANDLE                          	ConsoleInHandle;
  void      							*ConIn;
  EFI_HANDLE                          	ConsoleOutHandle;
  void     								*ConOut;
  EFI_HANDLE                          	StandardErrorHandle;
  void     								*StdErr;
  EFI_RUNTIME_SERVICES                	*RuntimeServices;
  void                   				*BootServices;
  UINTN                               	NumberOfTableEntries;
  void             						*ConfigurationTable;
} EFI_SYSTEM_TABLE;
int dmg_write(void* buff,int lba28,int block_number,int divice);
EFI_SYSTEM_TABLE* get_sys_table_addr();
EFI_GUID* get_var_guid();
void test3();
//EFI分区的代码
#define gUidPartTypeEFI {0xC12A7328,0xF81F,0x11D2,{0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}
//数据分区的代码
#define gUidPartTypeBaseData {0xEBD0A0A2,0xB9E5,0x4433,{0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}}
//EFI分区表的表项
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
	unsigned char creat_time[10];
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
int io_load_eflags(void);
int io_store_eflags(int eflags);
int load_gdtr(int limit, int addr);
int load_idtr(int limit, int addr);
int load_cr0(void);
int store_cr0(int cr0);
int load_cr3(void);
int store_cr3(int cr3);
void load_tr(int tr);
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
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);
int support_apic(void);
void io_wrmsr(int edx,int eax,int ecx);
void io_rdmsr(int* edx,int* eax,int ecx);
void test2(int eax,int ebx);
void store_gdt(void* addr);
void store_idt(void* addr);
void sys_reboot(void);
void sys_call(void);
int rdrand();
unsigned int  asm_sse_strcmp(char* s,char* t,unsigned int longer);
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
};
void init_gdtidt(UINTN main_this);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
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
	unsigned int addr, size;
};
#pragma pack(1)
struct MEMMAN {		/* メモリ管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
#pragma pack(1)
struct PAGEMAN32 {
	unsigned int free_page_num;
	unsigned int total_page_num;
	unsigned char* mem_map_base; 
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
void init_page(struct PAGEMAN32 *man);
unsigned int memmam_link_page_32_m(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address,unsigned int physical_address,int page_num,int mode);
unsigned int memman_link_page_32(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address,unsigned int physical_address,int mode);
unsigned int memman_alloc_page_32(struct PAGEMAN32 *man);
unsigned int memman_free_page_32(struct PAGEMAN32 *man,unsigned int physical_address);
unsigned int pageman_link_page_32(struct PAGEMAN32 *man,unsigned int linear_address,unsigned int physical_address,int mode);
unsigned int pageman_link_page_32_m(struct PAGEMAN32 *man,unsigned int linear_address,unsigned int physical_address,int page_num,int mode);
unsigned int pageman_unlink_page_32_m(struct PAGEMAN32 *man,unsigned int linear_address,int page_num,int mode);
unsigned pageman_unlink_page_32(struct PAGEMAN32 *man,unsigned int linear_address,int mode);
int inthandler0e(int cr2,int* esp);
unsigned int get_physical_by_linear_32(unsigned int linear_address);
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
void listview_init(struct ListView self,struct List list);
void listview_additem(struct ListView self,struct Item item);
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
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
#pragma pack(1)
struct TIMERCTL {
	unsigned int count, count64, next, next64;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
	unsigned int fps;//本定时器一秒产生多少次中断,系统组件需要根据这个值评估自身申请的定时器timeout的大小,填写近似值也被允许
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
#define TASK_GDT0		3		/* TSSをGDTの何番から割り当てるのか */
#define MAX_TASKS_LV	100
#define MAX_TASKLEVELS	10
#pragma pack(1)
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
#pragma pack(1)
struct TASK {
	int sel, flags; /* selはGDTの番号のこと */
	int level, priority;
	struct FIFO32 fifo;
	FIFOMOUSE fifom;
	struct _TASK_MSG_BOX msg_box;
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[4];
	struct CONSOLE *cons;
	int ds_base, cons_stack;
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
	unsigned int mem_use;//内存使用量
	unsigned int root_dir_addr;//文件根目录
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
struct CONSOLE {
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;//命令行输出坐标和光标颜色
	int font_x,font_y;//命令行输出行数和字数
	int font_x_max,font_y_max;//命令行的总x和总y
	struct TIMER *timer;
	int mode;/*命令行模式 0：普通文本模式*/
	char* message;/*message为cmd中的内容，根据mode进行相应解析*/
	int language;/*命令行所用的语言*/
	int cons_stdout_mode;/*标准输出绑定情况*/
	int cons_stderr_mode;/*标准错误绑定情况*/
};
struct FILEHANDLE {
	char *buf;
	int size;
	int pos;
};
void console_task(struct SHEET *sheet, int memtotal);
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
  unsigned int HBA_RPxTFD;
  unsigned int HBA_RPxSIG;
  unsigned int HBA_RPxSSTS;
  unsigned int HBA_RPxSCTL;
  unsigned int HBA_RPxSERR;
  unsigned int HBA_RPxSACT;
  unsigned int HBA_RPxCI;
  unsigned int HBA_RPxSNTF;
  unsigned int HBA_RPxFBS;
  
  unsigned int vendor[24];  
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
	char control;
	short auxiliary;
	short reserved;
}AHCI_CFIS_0X27;
#pragma pack(1)
typedef struct _ahci_prdt{//数据块描述结构 描述设备和系统传输数据的地址
	unsigned int dba;//数据块地址
	unsigned int dbau;//数据块地址的64位版本
	unsigned reserved;
	unsigned int dbc;//数据块大小
}AHCI_PRDT;
#pragma pack(1)
typedef struct _ahci_sata_fis{//SATA FIS的结构
	union{//CFIS结构
		AHCI_CFIS_0X27 ahci_cfis_0x27;
		char space[0x40];
	}cfis;
	union{//ACMD结构
		char space[0x10];
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
unsigned pcie_get_rcba();
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