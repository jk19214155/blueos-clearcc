void api_putchar(int c);
void api_putstr0(char *s);
void api_putstr1(char *s, int l);
void api_end(void);
int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);
void api_point(int win, int x, int y, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_closewin(int win);
int api_getkey(int mode);
int api_alloctimer(void);
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_freetimer(int timer);
void api_beep(int tone);
int api_fopen(char *fname);
void api_fclose(int fhandle);
void api_fseek(int fhandle, int offset, int mode);
int api_fsize(int fhandle, int mode);
int api_fread(char *buf, int maxsize, int fhandle);
int api_cmdline(char *buf, int maxsize);
int api_getevent(int* regs);
struct SHTCTL *shtctl_init(unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refresh_all(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
void sheet_refreshsub24(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshsub32(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
#define MAX_SHEETS		256
#define SHEET_MOUSE_ENABLE 0x80000000
#define SHEET_USE		1

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
};
struct DISPLAY{
	void (*sheet_refreshsub)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);//局部刷新函数
};
struct SHTCTL {
	void *vram;//图层管理器输出buff
	void *map;//图层排布buff(总体显示情况)
	struct SHEET* sid[256];//id分配情况
	unsigned char* opacity;//map图层透明度
	int xsize, ysize, top;
	int sheets0_size;//公共图层数据数组包含的图层数量
	struct SHEET *sheets[MAX_SHEETS];//本图层控制器控制的图层列表
	struct SHEET *sheets0;//公共图层数据数组
	struct SHEET *vram4sht;//该图层控制器的输出是否接到了一个图层上
	struct DISPLAY func;//操作函数
};

struct button{
	struct SHEET;
	char* name;
	int (*onclick)(struct button* button);
};
