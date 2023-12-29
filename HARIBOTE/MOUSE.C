/* マウス関係 */

#include "bootpack.h"

static struct FIFO32 *mousefifo;
static int mousedata0;
static char buff[256];
void inthandler2c(int *esp)
/* PS/2マウスからの割り込み */
{
	int data;
	//io_out8(PIC1_OCW2, 0x64);	/* IRQ-12受付完了をPIC1に通知 */
	//io_out8(PIC0_OCW2, 0x62);	/* IRQ-02受付完了をPIC0に通知 */
	*(int*)(0xfec00040)=0;
	*(int*)(0xfee000b0)=0;
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data + mousedata0);
	sprintf(buff,"inthandler2c data: %d\n",data);
	com_out_string(0x3f8,buff);
	sprintf(buff,"fifo_sattus: this:%x p:%d q:%d size:%d free:%d task:%x\n",mousefifo,mousefifo->p,mousefifo->q,mousefifo->size,mousefifo->free,mousefifo->task);
	com_out_string(0x3f8,buff);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
#define MOUSECMD_ENABLE_WHEEL	0xe8 
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
	/* 書き込み先のFIFOバッファを記憶 */
	sprintf(buff,"enable_mouse fifo: %x\n",fifo);
	com_out_string(0x3f8,buff);
	mousefifo = fifo;
	mousedata0 = data0;
	/* ?用鼠??? */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE_WHEEL); 
	/* ?用鼠?移? */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	/* うまくいくとACK(0xfa)が送信されてくる */
	mdec->phase = 0; /* マウスの0xfaを待っている段階 */
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* マウスの0xfaを待っている段階 */
		com_out_string(0x3f8,"mouse_decode 0\n");
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/* マウスの1バイト目を待っている段階 */
		com_out_string(0x3f8,"mouse_decode 1\n");
		if ((dat & 0xc8) == 0x08) {
			/* 正しい1バイト目だった */
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/* マウスの2バイト目を待っている段階 */
		com_out_string(0x3f8,"mouse_decode 2\n");
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/* マウスの3バイト目を待っている段階 */
		com_out_string(0x3f8,"mouse_decode 3\n");
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; /* マウスではy方向の符号が画面と反対 */
		return 1;
	}
	return -1; /* ここに来ることはないはず */
}

int mouse_decode_wheel(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* マウスの0xfaを待っている段階 */
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/* マウスの1バイト目を待っている段階 */
		if ((dat & 0xc8) == 0x08) {
			/* 正しい1バイト目だった */
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/* マウスの2バイト目を待っている段階 */
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	//原初拦截
	if (mdec->phase == 3) {
		/* マウスの2バイト目を待っている段階 */
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; /* マウスではy方向の符号が画面と反対 */
		mdec->wheel=0;
		return 0;
	}
	if (mdec->phase == 4) {
		/* マウスの3バイト目を待っている段階 */
		mdec->buf[3] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; /* マウスではy方向の符号が画面と反対 */
		mdec->wheel=mdec->buf[3];
		return 1;
	}
	return -1; /* ここに来ることはないはず */
}
