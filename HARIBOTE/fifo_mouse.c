/* FIFOライブラリ */

#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

void fifo_mouse_init(struct _FIFOMOUSE *fifo, int size, MOUSESTATUS *buf, struct TASK *task)
/* FIFOバッファの初期化 */
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* 空き */
	fifo->flags = 0;
	fifo->p = 0; /* 書き込み位置 */
	fifo->q = 0; /* 読み込み位置 */
	fifo->task = task; /* データが入ったときに起こすタスク */
	return;
}

MOUSESTATUS* fifo_mouse_put(struct _FIFOMOUSE *fifo, UINTN x,UINTN y ,UINTN btn)
/* FIFOへデータを送り込んで蓄える */
{
	MOUSESTATUS* data_point;
	if (fifo->free == 0) {
		/* 空きがなくてあふれた */
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	data_point=(fifo->buf)+(fifo->p);
	fifo->buf[fifo->p].x = x;
	fifo->buf[fifo->p].y = y;
	fifo->buf[fifo->p].btn = btn;
	fifo->p++;
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	fifo->free--;
	return data_point;
}

MOUSESTATUS* fifo_mouse_get(struct _FIFOMOUSE *fifo)
/* FIFOからデータを一つとってくる */
{
	MOUSESTATUS* data_point;
	if (fifo->free == fifo->size) {
		/* バッファが空っぽのときは、とりあえず-1が返される */
		return -1;
	}
	data_point = (fifo->buf)+(fifo->q);
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data_point;
}

int fifo_mouse_status(struct _FIFOMOUSE *fifo)
/* どのくらいデータが溜まっているかを報告する */
{
	return fifo->size - fifo->free;
}
