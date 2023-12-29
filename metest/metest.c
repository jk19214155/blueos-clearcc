#include "apilib.h"
#include <stdio.h>

void boxfill32(unsigned int *vram, int xsize, unsigned int c, int x0, int y0, int x1, int y1)
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;
	}
	return;
}

void putfont32(int *vram, int xsize, int x, int y, int c, char *font)
{
	int i;
	int *p;
	char d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

/*void putfonts32_asc(int *vram, int xsize, int x, int y, int c, unsigned char *s)
{
	extern char hankaku[4096];
	struct TASK *task = task_now();
	char *nihongo = (char *) *((int *) 0x0fe8), *font;
	int k, t;

	if (task->langmode == 0) {
		for (; *s != 0x00; s++) {
			putfont32(vram, xsize, x, y, c, hankaku + *s * 16);
			x += 8;
		}
	}
	if (task->langmode == 1) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
					task->langbyte1 = *s;
				} else {
					putfont32(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			} else {
				if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) {
					k = (task->langbyte1 - 0x81) * 2;
				} else {
					k = (task->langbyte1 - 0xe0) * 2 + 62;
				}
				if (0x40 <= *s && *s <= 0x7e) {
					t = *s - 0x40;
				} else if (0x80 <= *s && *s <= 0x9e) {
					t = *s - 0x80 + 63;
				} else {
					t = *s - 0x9f;
					k++;
				}
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont32(vram, xsize, x - 8, y, c, font     );	// ���� 
				putfont32(vram, xsize, x    , y, c, font + 16);	// �Ұ�� 
			}
			x += 8;
		}
	}
	if (task->langmode == 2) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if (0x81 <= *s && *s <= 0xfe) {
					task->langbyte1 = *s;
				} else {
					putfont32(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			} else {
				k = task->langbyte1 - 0xa1;
				t = *s - 0xa1;
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont32(vram, xsize, x - 8, y, c, font     );	// ���� 
				putfont32(vram, xsize, x    , y, c, font + 16);	// �Ұ�� 
			}
			x += 8;
		}
	}
	return;
}*/


/*Button��ť��*/
struct button{
	struct SHEET;
	char* name;
	int status;
	int (*onclick)(struct button* button);
};
struct button* button_alloc(int x,int y,char* name){
	char* p=api_malloc(sizeof(struct button));
	p->buf32=api_malloc(x*y*4);
	button_paint(p);
	return p;
}
#define BUTTON_BG_COLOR 0x00848484
#define BUTTON_FRAME_COLOR 0x00ffffff
void button_paint(struct button* this) {
    int xsize = this->sheet->xsize; 
    int ysize = this->sheet->ysize;
    unsigned int *buf = this->sheet->buf;
    
    // ���Ʊ���ɫ���
    boxfill32(buf, xsize, BUTTON_BG_COLOR, 0, 0, xsize - 1, ysize - 1);
    
    // ���Ʊ߿�(��ѡ)
    if (this->state == BUTTON_STATE_ACTIVE) {    // �״̬�Ż��Ʊ߿�
        // �����ⲿ�����(��ɫ)
        boxfill32(buf, xsize, BUTTON_FRAME_COLOR, 0, 0, xsize - 1, ysize - 1);    
        
        // �ڲ�����(����ɫ) 
        boxfill32(buf, xsize, BUTTON_BG_COLOR, 1, 1, xsize - 2, ysize - 2);        
    }  
    
    // �����ı�
    //putfonts32_asc(buf, xsize, 24, (xsize - strlen(this->name) * 8) / 2,  BUTTON_TEXT_COLOR, this->name);  
    
    // �״̬΢���ı���ɫ
    if (this->state == 0) {
       // putfonts32_asc(buf, xsize, 24, (xsize - strlen(this->name) * 8) / 2, BUTTON_TEXT_COLOR_ACTIVE, this->name); 
    }
} 

void HariMain(void)
{
	int i;
	char buff[50];
	int regs[10];
	struct SHEET* sht_base,sht_win,sht
	api_initmalloc();
	struct SHTCTL *shtctl=shtctl_init(base_buff, 256, 128);//ͼ�������
	struct SHEET* sheet0=api_malloc(sizeof(struct SHEET [16]));//����ͼ��
	for(i=0;i<16;i++){
		sheet0[i].flags=0;//����ͼ����Ч
	}
	shtctl->sheets0=sheet0;//����ͼ������
	shtctl->sheets0_size=16;//����ͼ�������С
	
	int win=api_openwin(base_buff, 256, 128, -1, "metest");
	struct button button0=button_alloc(50,25,"test");
	//sheet_updown(sht_base,0);
	sheet_updown(button0,1);
	api_refreshwin(win, 0, 0, sht_base->bxsize,sht_base->bysize);
	for(;;){
		api_getevent(regs);
		if(regs[0]==0){
			if(regs[2]=='\r' || regs[2]=='\n' || regs[2]=='q'){
				api_end();
			}
		}
		else if(regs[0]==1){
			
		}
	}
	api_end();
}
