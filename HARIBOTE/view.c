#include "bootpack.h"
#include <stdio.h>
#include <string.h>
void view_init(struct View* view){
	((struct SHEET*)view)->func=0;//没有虚函数列表
	return;
}
void view_setfunc(struct View* self,struct VIEW_FUNC* func){
	((struct SHEET*)self)->func=func;
	return;
}
void textview_init(struct TextView* self,char* text){
	view_init(self);
	self->text=0;
	return;
}
void textview_settext(struct TextView* self,char* text){
	self->text=text;
	return;
}
void textview_flush(struct TextView* self){
	int x=((struct SHEET*)self)->bxsize;//文本框宽度
	int y=((struct SHEET*)self)->bysize;//文本框高度
	int color=((struct View*)self)->background_color;//取颜色
	int color_t=self->color;//文本颜色
	int buf=((struct SHEET*)self)->buf;
	boxfill8(buf, x, color, 0, 0, x-1, y-1);//画上背景
	putfonts8_asc(buf, x, 1, 1, color_t, self->text);//画上文字
	return;
}
void view_flush(struct View* self,int x0,int y0,int x1,int y1){
	sheet_refresh(self,x0,y0,x1,y1);//调用sheet类的刷新方法
	return;
}