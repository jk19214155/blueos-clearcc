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

struct SHTCTL *shtctl_init(unsigned char *vram, int xsize, int ysize)
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) api_malloc(sizeof (struct SHTCTL));//mem_alloc!!!
	if (ctl == 0) {
		goto err;
	}
	ctl->map = (unsigned char *) api_malloc( xsize * ysize * 4);
	for(i=0;i<xsize * ysize;i++){
		((int*)ctl->map)[i]=0;
	}
	if (ctl->map == 0) {
		//memman_free_4k(memman, (int) ctl, sizeof (struct SHTCTL));
		goto err;
	}
	for(i=0;i<256;i++){
		ctl->sid[i]=0;//所有??id都可用
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; /* シートは一枚もない */
	ctl->vram4sht=0;//??控制器的?出不????
	//(ctl->func).sheet_refreshsub=sheet_refreshsub24;//24位彩色
	(ctl->func).sheet_refreshsub=sheet_refreshsub32;//32位彩色
	//for (i = 0; i < MAX_SHEETS; i++) {
		//ctl->sheets0[i].flags = 0; 
		//ctl->sheets0[i].ctl = ctl; 
	//}
err:
	return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
	struct SHEET *sht;
	int i;
	for (i = 0; i < ctl->sheets0_size; i++) {
		sht = &ctl->sheets0[i];
		if (sht->flags == 0) {
			sht->flags = SHEET_USE; /* 使用中マーク */
			sht->height = -1; /* 非表示中 */
			sht->task = 0;	/* 自動で閉じる機能を使わない */
			sht->ctl=ctl;//??的管理器是自己
			sht->ctl_from=0;//??不由控制器生成
			//sht->alpha_map=0;//没有透明度图层
			return sht;
		}
	}
	return 0;	/* 全てのシートが使用中だった */
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)//map局部刷新函数指定?始h0
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
	unsigned char *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	//数据修正
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= ctl->top; h++) {//从h0高度?始由下到上刷新
		sht = ctl->sheets[h];//?取本次要刷新的??
		//sid = sht - ctl->sheets0; //指?相??得id
		sid=sht->sid;
		buf = sht->buf;//??数据信息
		//?算刷新坐?的算法
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		if (sht->col_inv == -1) {//没有使用透明色
			if ((sht->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) {//地址4字???使用高速算法
				bx1 = (bx1 - bx0) / 4; //横坐?除4
				sid4 = sid | sid << 8 | sid << 16 | sid << 24;//将8位id?展到32位
				for (by = by0; by < by1; by++) {//行数是循?目?
					vy = sht->vy0 + by;
					vx = sht->vx0 + bx0;
					p = (int *) &map[vy * ctl->xsize + vx];//?算地址
					for (bx = 0; bx < bx1; bx++) {//横坐?是循?目?
						p[bx] = sid4;
					}
				}
			} else {//地址不??使用普通算法
				for (by = by0; by < by1; by++) {
					vy = sht->vy0 + by;
					for (bx = bx0; bx < bx1; bx++) {
						vx = sht->vx0 + bx;
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		} else {//有透明色使用一般算法
			for (by = by0; by < by1; by++) {
				vy = sht->vy0 + by;
				for (bx = bx0; bx < bx1; bx++) {
					vx = sht->vx0 + bx;
					if (buf[by * sht->bxsize + bx] != sht->col_inv) {
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		}
	}
	return;
}

void sheet_refreshmap32(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)//map局部刷新函数指定?始h0
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
	unsigned int *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	//数据修正
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= ctl->top; h++) {//从h0高度?始由下到上刷新
		sht = ctl->sheets[h];//?取本次要刷新的??
		//sid = sht - ctl->sheets0; //指?相??得id
		sid=sht->sid;
		buf = sht->buf;//??数据信息
		//?算刷新坐?的算法
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		//一般算法(32位?色?示不支持高速算法)
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if (buf[by * sht->bxsize + bx] != sht->col_inv) {//不是透明色
					if((buf[by * sht->bxsize + bx]&0xff000000)==0){//没有透明元素
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		}
	}
	return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)//屏幕局部刷新函数，指定h0和h1(刷新上界和刷新下界)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, bx2, sid4, i, i1, *p, *q, *r;
	unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
	struct SHEET *sht;
	//数据修正
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		//sid = sht - ctl->sheets0;
		sid=sht->sid;
		//?算刷新坐?的算法
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		if ((sht->vx0 & 3) == 0) {
			/* 4バイト型 */
			i  = (bx0 + 3) / 4; /* bx0を4で割ったもの（端数切り上げ） */
			i1 =  bx1      / 4; /* bx1を4で割ったもの（端数切り捨て） */
			i1 = i1 - i;
			sid4 = sid | sid << 8 | sid << 16 | sid << 24;
			for (by = by0; by < by1; by++) {
				vy = sht->vy0 + by;
				for (bx = bx0; bx < bx1 && (bx & 3) != 0; bx++) {	/* 前の端数を1バイトずつ */
					vx = sht->vx0 + bx;
					if (map[vy * ctl->xsize + vx] == sid) {
						vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
					}
				}
				vx = sht->vx0 + bx;
				p = (int *) &map[vy * ctl->xsize + vx];
				q = (int *) &vram[vy * ctl->xsize + vx];
				r = (int *) &buf[by * sht->bxsize + bx];
				for (i = 0; i < i1; i++) {//4的倍数的部分
					if (p[i] == sid4) {
						q[i] = r[i];
					} else {
						bx2 = bx + i * 4;
						vx = sht->vx0 + bx2;
						if (map[vy * ctl->xsize + vx + 0] == sid) {
							vram[vy * ctl->xsize + vx + 0] = buf[by * sht->bxsize + bx2 + 0];
						}
						if (map[vy * ctl->xsize + vx + 1] == sid) {
							vram[vy * ctl->xsize + vx + 1] = buf[by * sht->bxsize + bx2 + 1];
						}
						if (map[vy * ctl->xsize + vx + 2] == sid) {
							vram[vy * ctl->xsize + vx + 2] = buf[by * sht->bxsize + bx2 + 2];
						}
						if (map[vy * ctl->xsize + vx + 3] == sid) {
							vram[vy * ctl->xsize + vx + 3] = buf[by * sht->bxsize + bx2 + 3];
						}
					}
				}
				for (bx += i1 * 4; bx < bx1; bx++) {				/* 後ろの端数を1バイトずつ */
					vx = sht->vx0 + bx;
					if (map[vy * ctl->xsize + vx] == sid) {
						vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
					}
				}
			}
		} else {
			/* 1バイト型 */
			for (by = by0; by < by1; by++) {
				vy = sht->vy0 + by;
				for (bx = bx0; bx < bx1; bx++) {
					vx = sht->vx0 + bx;
					if (map[vy * ctl->xsize + vx] == sid) {
						vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
					}
				}
			}
		}
	}
	return;
}

void sheet_refreshsub24(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)//屏幕局部刷新函数，指定h0和h1(刷新上界和刷新下界)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, bx2, sid4, i, i1, *p, *q, *r;
	unsigned int *buf;
	unsigned char *vram = ctl->vram;
	unsigned int *map = ctl->map, sid;
	struct SHEET *sht;
	union{
		int int32;
		unsigned char char8[4];
	}color;
	//数据修正
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;//使用原始图层
		//sid = sht - ctl->sheets0;
		sid=sht->sid;
		//?算刷新坐?的算法
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		//没有快速算法
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				color.int32= buf[by * sht->bxsize + bx];
				unsigned char a=color.char8[3];//通透性数据
				/*
				两个条件满足其一即可描绘:
				1.本像素属于map指示的图层(本图层)
				2.本像素是透明像素且高于map指示的图层(透明叠加层)
				*/
				if (map[vy * ctl->xsize + vx] == sid ||( a!=0 && (sht->height >= (ctl->sid[map[vy * ctl->xsize + vx]]->height)))) {
					unsigned int r=color.char8[0];
					unsigned int g=color.char8[1];
					unsigned int b=color.char8[2];
					unsigned int r0=vram[(vy * ctl->xsize + vx)*3+0];
					unsigned int g0=vram[(vy * ctl->xsize + vx)*3+1];
					unsigned int b0=vram[(vy * ctl->xsize + vx)*3+2];
					r=((unsigned int)r*(255-a)+r0*a);
					g=((unsigned int)g*(255-a)+g0*a);
					b=((unsigned int)b*(255-a)+b0*a);
					vram[(vy * ctl->xsize + vx)*3+0]= r;
					vram[(vy * ctl->xsize + vx)*3+1]= g;
					vram[(vy * ctl->xsize + vx)*3+2]= b;
				}
			}
		}
	}
	return;
}

void sheet_refreshsub32(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)//屏幕局部刷新函数，指定h0和h1(刷新上界和刷新下界)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, bx2, sid4, i, i1, *p, *q, *r;
	unsigned int *buf;
	char *vram = ctl->vram;
	unsigned int *map = ctl->map, sid;
	struct SHEET *sht;
	union{
		int int32;
		char char8[4];
	}color;
	//数据修正
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		//sid = sht - ctl->sheets0;
		sid=sht->sid;
		//?算刷新坐?的算法
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		//没有快速算法
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				color.int32= buf[by * sht->bxsize + bx];
				unsigned char a=color.char8[3];//通透性数据
				/*
				两个条件满足其一即可描绘:
				1.本像素属于map指示的图层(本图层)
				2.本像素是透明像素且高于map指示的图层(透明叠加层)
				*/
				if (map[vy * ctl->xsize + vx] == sid){//底板图层直接复制原始数据
					unsigned int r=color.char8[0];
					unsigned int g=color.char8[1];
					unsigned int b=color.char8[2];
					vram[(vy * ctl->xsize + vx)*4+0]= r;
					vram[(vy * ctl->xsize + vx)*4+1]= g;
					vram[(vy * ctl->xsize + vx)*4+2]= b;
				}
				else if ( a!=0 && (sht->height >= (ctl->sid[map[vy * ctl->xsize + vx]]->height))) {//透明层描绘
					unsigned int r=color.char8[0];
					unsigned int g=color.char8[1];
					unsigned int b=color.char8[2];
					unsigned int r0=vram[(vy * ctl->xsize + vx)*4+0];
					unsigned int g0=vram[(vy * ctl->xsize + vx)*4+1];
					unsigned int b0=vram[(vy * ctl->xsize + vx)*4+2];
					r=(r*(255-a)+r0*a)/255;
					g=(g*(255-a)+g0*a)/255;
					b=(b*(255-a)+b0*a)/255;
					vram[(vy * ctl->xsize + vx)*4+0]= r;
					vram[(vy * ctl->xsize + vx)*4+1]= g;
					vram[(vy * ctl->xsize + vx)*4+2]= b;
					vram[(vy * ctl->xsize + vx)*4+3]=0;
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHEET *sht, int height)//??上移下移函数
{
	struct SHTCTL *ctl = sht->ctl;
	int h, old = sht->height; //?定前的高度
	//void (*sheet_refreshsub32)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)=(ctl->func).sheet_refreshsub;
	//void (*sheet_refreshsub32)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)=sheet_refreshmap32;
	/* 指定が低すぎや高すぎだったら、修正する */
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height; //高度?定
	if (old > height) {	//比以前低
		if (height >= 0) {
			for (h = old; h > height; h--) {//所有数据移?位置
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;//??插入
			sheet_refreshmap32(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);//刷新map
			sheet_refreshsub32(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);//刷新??
		} else {	//进入隐藏状态
			if (ctl->top > old) {
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--; //总高度减少
			sheet_refreshmap32(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
			sheet_refreshsub32(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
			//释放图层管理信息
			sht->ctl->sid[sht->sid]=0;
		}
	} else if (old < height) {	//比以前高
		if (old >= 0) {
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {	//从隐藏到显示
			int i;
			for(i=0;i<256;i++){//是否有空sid可用分配
				if(sht->ctl->sid[i]==0){
					sht->ctl->sid[i]=sht;
					sht->sid=i;//注册sid
					break;
				}
			}
			if(i>=256){//失败，无法进入显示状态
				return;
			}
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++; //?高度增加
		}
		sheet_refreshmap32(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsub32(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
	return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)//??局部?生改?刷新
{
	void (*sheet_refreshsub32)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)=(sht->ctl->func).sheet_refreshsub;
	if (sht->height >= 0) { //?示中？
		sheet_refreshsub32(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
	}
	return;
}
void sheet_refresh_all(struct SHEET *sht, int bx0, int by0, int bx1, int by1)//??局部?生改?刷新
{
	void (*sheet_refreshsub32)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)=(sht->ctl->func).sheet_refreshsub;
	if (sht->height >= 0) { //?示中？
		sheet_refreshmap32(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, 0);
		sheet_refreshsub32(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, 0, sht->height);
	}
	return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0)//??移?到新位置
{
	struct SHTCTL *ctl = sht->ctl;
	void (*sheet_refreshsub32)(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)=(ctl->func).sheet_refreshsub;
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { //?示中刷新画面
		sheet_refreshmap32(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap32(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
		sheet_refreshsub32(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
		sheet_refreshsub32(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
	}
	return;
}

void sheet_free(struct SHEET *sht)//?放??
{
	if (sht->height >= 0) {
		sheet_updown(sht, -1); //???藏状?
	}
	sht->flags = 0; //不使用
	return;
}

void sheet_init4ctl(){
	return;
}

