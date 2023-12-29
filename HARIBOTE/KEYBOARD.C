/* キーボード関係 */

#include "bootpack.h"

static struct FIFO32 *keyfifo;
static int keydata0;
static char buff[256];
void inthandler21(int *esp)
{
	int data;
	//io_out8(PIC0_OCW2, 0x61);	/* IRQ-01受付完了をPICに通知 */
	*(int*)(0xfec00040)=0;
	*(int*)(0xfee000b0)=0;
	data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, data + keydata0);
	sprintf(buff,"inthandler21 data: %d\n",data);
	com_out_string(0x3f8,buff);
	sprintf(buff,"fifo_sattus: this:%x p:%d q:%d size:%d free:%d task:%x\n",keyfifo,keyfifo->p,keyfifo->q,keyfifo->size,keyfifo->free,keyfifo->task);
	com_out_string(0x3f8,buff);
	return;
}

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)
{
	com_out_string(0x3f8,"wait_KBC_sendready\n");
	/* キーボードコントローラがデータ送信可能になるのを待つ */
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	com_out_string(0x3f8,"wait_KBC_sendready OK!\n");
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	/* 書き込み先のFIFOバッファを記憶 */
	com_out_string(0x3f8,"init_keyboard\n");
	keyfifo = fifo;
	keydata0 = data0;
	/* キーボードコントローラの初期化 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}
