/* タイマ関係 */

#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl[8];

#define TIMER_FLAGS_ALLOC		1	/* 確保した状態 */
#define TIMER_FLAGS_USING		2	/* タイマ作動中 */

void init_pit(void)
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl[0].count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl[0].timers0[i].flags = 0; /* 未使用 */
	}
	t = timer_alloc(0); /* 一つもらってくる */
	t->timeout = 0xffffffff;
	t->timeout64 = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; /* 一番うしろ */
	timerctl[0].t0 = t; /* 今は番兵しかいないので先頭でもある */
	timerctl[0].next = 0xffffffff; /* 番兵しかいないので番兵の時刻 */
	timerctl[0].next64 = 0xffffffff;
	timerctl[0].fps=100;
	return;
}

void init_hpet_timer(void)
{
	int i;
	struct TIMER *t;
	timerctl[1].count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl[1].timers0[i].flags = 0; /* 未使用 */
	}
	t = timer_alloc(1); /* 一つもらってくる */
	t->timeout = 0xffffffff;
	t->timeout64 = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; /* 一番うしろ */
	timerctl[1].t0 = t; /* 今は番兵しかいないので先頭でもある */
	timerctl[1].next = 0xffffffff; /* 番兵しかいないので番兵の時刻 */
	timerctl[1].next64 = 0xffffffff;
	timerctl[1].fps=1000;
	//??中断
	unsigned int HPET_base_address=0xfed00000;
	*(int*)(HPET_base_address+0x10)=1;//?始?数
	return;
}

unsigned int timer_get_fps(unsigned int index){
	return timerctl[index].fps;
}

struct TIMER *timer_alloc(unsigned int index)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl[index].timers0[i].flags == 0) {
			timerctl[index].timers0[i].flags = TIMER_FLAGS_ALLOC;
			timerctl[index].timers0[i].flags2 = 0;
			return &timerctl[index].timers0[i];
		}
	}
	return 0; /* 見つからなかった */
}

void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /* 未使用 */
	return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(unsigned int index,struct TIMER *timer, unsigned int timeout,unsigned int timeout64)
{
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl[index].count;
	timer->timeout64 = timeout64 + timerctl[index].count64;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl[index].t0;
	if (timer->timeout64 < t->timeout64 || timer->timeout64 == t->timeout64 && timer->timeout <= t->timeout) {
		/* 先頭に入れる場合 */
		timerctl[index].t0 = timer;
		timer->next = t; /* 次はt */
		timerctl[index].next = timer->timeout;
		timerctl[index].next64 = timer->timeout64;
		io_store_eflags(e);
		return;
	}
	/* どこに入れればいいかを探す */
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout64 < t->timeout64 || timer->timeout64 == t->timeout64 && timer->timeout <= t->timeout) {
			/* sとtの間に入れる場合 */
			s->next = timer; /* sの次はtimer */
			timer->next = t; /* timerの次はt */
			io_store_eflags(e);
			return;
		}
	}
}

void inthandler20(int *esp)
{
	struct TIMER *timer;
	char ts = 0;
	unsigned int index=0;
	*(int*)(0xfec00040)=0;
	*(int*)(0xfee000b0)=0;
	timerctl[index].count++;
	if(timerctl[index].count==0){//如果?生?回 ??高置位?增
		timerctl[index].count64++;
	}
	if (timerctl[index].next64 > timerctl[index].count64 ||  timerctl[index].next64 == timerctl[index].count64 && timerctl[index].next > timerctl[index].count) {
		return;
	}
	timer = timerctl[index].t0; /* とりあえず先頭の番地をtimerに代入 */
	for (;;) {
		/* timersのタイマは全て動作中のものなので、flagsを確認しない */
		if(timer->timeout64 == 0xffffffff && timer->timeout == 0xffffffff){
			break;
		}
		if (timer->timeout64 > timerctl[index].count64 || timer->timeout64 == timerctl[index].count64 && timer->timeout > timerctl[index].count) {
			break;
		}
		/* タイムアウト */
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			fifo32_put(timer->fifo, timer->data);
		} else {
			ts = 1; /* task_timerがタイムアウトした */
		}
		timer = timer->next; /* 次のタイマの番地をtimerに代入 */
	}
	timerctl[index].t0 = timer;
	timerctl[index].next = timer->timeout;
	timerctl[index].next64 = timer->timeout64;
	if (ts != 0) {
		task_switch();
	}
	return;
}

void inthandler34(int *esp){
	struct TIMER *timer;
	char ts = 0;
	unsigned int index=1;
	//io_out8(PIC0_OCW2, 0x60);	/* IRQ-00受付完了をPICに通知 */
	*(int*)(0xfec00040)=0;
	*(int*)(0xfee000b0)=0;
	timerctl[index].count++;
	if(timerctl[index].count==0){//如果?生?回 ??高置位?增
		timerctl[index].count64++;
	}
	if (timerctl[index].next64 > timerctl[index].count64 ||  timerctl[index].next64 == timerctl[index].count64 && timerctl[index].next > timerctl[index].count) {
		return;
	}
	timer = timerctl[index].t0; /* とりあえず先頭の番地をtimerに代入 */
	for (;;) {
		/* timersのタイマは全て動作中のものなので、flagsを確認しない */
		if(timer->timeout64 == 0xffffffff && timer->timeout == 0xffffffff){
			break;
		}
		if (timer->timeout64 > timerctl[index].count64 || timer->timeout64 == timerctl[index].count64 && timer->timeout > timerctl[index].count) {
			break;
		}
		/* タイムアウト */
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			fifo32_put(timer->fifo, timer->data);
		} else {
			ts = 1; /* task_timerがタイムアウトした */
		}
		timer = timer->next; /* 次のタイマの番地をtimerに代入 */
	}
	timerctl[index].t0 = timer;
	timerctl[index].next = timer->timeout;
	timerctl[index].next64 = timer->timeout64;
	if (ts != 0) {
		task_switch();
	}
	return;
}

int timer_cancel(unsigned int index,struct TIMER *timer)
{
	int e;
	struct TIMER *t;
	e = io_load_eflags();
	io_cli();	/* 設定中にタイマの状態が変化しないようにするため */
	if (timer->flags == TIMER_FLAGS_USING) {	/* 取り消し処理は必要か？ */
		if (timer == timerctl[index].t0) {
			/* 先頭だった場合の取り消し処理 */
			t = timer->next;
			timerctl[index].t0 = t;
			timerctl[index].next = t->timeout;
			timerctl[index].next64 = t->timeout64;
		} else {
			/* 先頭以外の場合の取り消し処理 */
			/* timerの一つ前を探す */
			t = timerctl[index].t0;
			for (;;) {
				if (t->next == timer) {
					break;
				}
				t = t->next;
			}
			t->next = timer->next; /* 「timerの直前」の次が、「timerの次」を指すようにする */
		}
		timer->flags = TIMER_FLAGS_ALLOC;
		io_store_eflags(e);
		return 1;	/* キャンセル処理成功 */
	}
	io_store_eflags(e);
	return 0; /* キャンセル処理は不要だった */
}

void timer_cancelall(unsigned int index,struct FIFO32 *fifo)
{
	int e, i;
	struct TIMER *t;
	e = io_load_eflags();
	io_cli();	/* 設定中にタイマの状態が変化しないようにするため */
	for (i = 0; i < MAX_TIMER; i++) {
		t = &timerctl[index].timers0[i];
		if (t->flags != 0 && t->flags2 != 0 && t->fifo == fifo) {
			timer_cancel(index,t);
			timer_free(t);
		}
	}
	io_store_eflags(e);
	return;
}
