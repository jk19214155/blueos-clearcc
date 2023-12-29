OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
		int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
		mtask.obj window.obj console.obj file.obj tek.obj dav.obj desktop.obj view.obj system.obj acpi.obj bmp.obj bmp2.obj dmg.obj drvier.obj \
		fifo_mouse.obj com.obj vmx.obj ahci.obj pcie.obj sse.obj

TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
NASM     = $(TOOLPATH)nasm.exe
GCC      = $(TOOLPATH)MinGW/bin/gcc.exe -I$(INCPATH) -m32  -nostdlib -fno-builtin -ffreestanding \
			-fno-stack-protector -Os -fno-pic -fno-pie -fno-asynchronous-unwind-tables  \
			-fomit-frame-pointer -O0 -finput-charset=UTF-8 -w -c
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

# デフォルト動作

default :
	$(MAKE) ipl09.bin
	$(MAKE) haribote.sys

# ファイル生成規則

ipl09.bin : ipl09.nas makefile
	$(NASK) ipl09.nas ipl09.bin ipl09.lst

asmhead.bin : asmhead.nas makefile
#	$(NASK) asmhead.nas asmhead.bin asmhead.lst
	$(NASM) asmhead.nas -f bin -o asmhead.bin -l asmhead.lst

hankaku.bin : hankaku.txt makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

haribote.sys : asmhead.bin bootpack.hrb makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys

naskfunc.obj : naskfunc.nas makefile
	$(NASM) naskfunc.nas -f coff -o naskfunc.obj -l naskfunc.lst

# 一般規則

%.gas : %.c bootpack.h makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas makefile
	$(GAS2NASK) $*.gas $*.nas

#%.obj : %.nas makefile bootpack.h
%.obj : %.c makefile bootpack.h
#	$(NASK) $*.nas $*.obj $*.lst
#	$(NASM) -f elf32 $*.nas -o $*.obj  -l $*.lst
	$(GCC) $*.c -o $*.obj -l $*.lst

# コマンド

clean :
	-$(DEL) asmhead.bin
	-$(DEL) hankaku.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb

src_only :
	$(MAKE) clean
	-$(DEL) ipl09.bin
	-$(DEL) haribote.sys
