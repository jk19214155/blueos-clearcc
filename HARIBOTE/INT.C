/* 割り込み関係 */

#include "bootpack.h"
#include <stdio.h>
void io_write_io_apic(unsigned char  index,unsigned int data);
void init_pic(void)
/* PICの初期化 */
{
	io_out8(PIC0_IMR,  0xff  ); /* 全ての割り込みを受け付けない */
	io_out8(PIC1_IMR,  0xff  ); /* 全ての割り込みを受け付けない */

	io_out8(PIC0_ICW1, 0x11  ); /* エッジトリガモード */
	io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7は、INT20-27で受ける */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1はIRQ2にて接続 */
	io_out8(PIC0_ICW4, 0x01  ); /* ノンバッファモード */

	io_out8(PIC1_ICW1, 0x11  ); /* エッジトリガモード */
	io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15は、INT28-2fで受ける */
	io_out8(PIC1_ICW3, 2     ); /* PIC1はIRQ2にて接続 */
	io_out8(PIC1_ICW4, 0x01  ); /* ノンバッファモード */

	io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外は全て禁止 */
	io_out8(PIC1_IMR,  0xff  ); /* 11111111 全ての割り込みを受け付けない */

	return;
}
void init_apic(void* apic_base)
{
	int tmp_low,tmp_high,addr;
	io_out8(PIC0_IMR, 0xff);//禁用8259中断控制器
	io_out8(PIC0_IMR, 0xff);
	//Enabling xAPIC(IA32_APIC_BASE[10]) and 2xAPIC(IA32_APIC_BASE[11])
    io_rdmsr(&tmp_high,&tmp_low,0x1b);
    tmp_low |= (1 << 10);
    io_wrmsr(tmp_high,tmp_low,0x1b);
    //Enabling LAPIC(SVR[8])
    *(int*)(apic_base+0xf0)|=(1<<8);
	io_write_io_apic(0,0x0f00000);
	/*禁用所有??*/
	for(addr=0x10;addr<=0x3e;addr+=2){
		io_write_io_apic(addr,1<<17);
	}
	//首先通?io-apic?置新的??中断
	io_write_io_apic(0x12,0x21);
	io_write_io_apic(0x13,*(char*)(apic_base+0x20)<<24);
	//?置新的??中断
	io_write_io_apic(0x14,0x20);
	io_write_io_apic(0x15,*(char*)(apic_base+0x20)<<24);
	//?置新的鼠?中断
	io_write_io_apic(0x10+12*2,0x2c);
	io_write_io_apic(0x10+12*2+1,*(char*)(apic_base+0x20)<<24);
	return;
}
void io_write_io_apic(unsigned char index,unsigned int data){
	*((char*)0xfec00000)=index;
	*((int*)0xfec00010)=data;
	return;
}
