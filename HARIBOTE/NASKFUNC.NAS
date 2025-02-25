; naskfunc
; tab=4

;format "wcoff"				; オブジェクトファイルを作るモーチe
;instrset "i486p"				; 486の命令まで使ぁe��ぁe��ぁe��記述
bits 64						; 32ビットモード用の機械語を作らせる
;file "naskfunc.nas"			; ソースファイル名情報

		global	io_hlt, io_cli, io_sti, io_stihlt
		global	io_in8,  io_in16,  io_in32
		global	io_out8, io_out16, io_out32
		global	io_load_eflags, io_store_eflags
		global	load_gdtr, load_idtr
		global	load_cr0, store_cr0
		global	load_cr3, store_cr3
		global load_cr4, store_cr4
		global	load_tr
		global	asm_inthandler20, asm_inthandler21
		global	asm_inthandler2c, asm_inthandler0c , asm_inthandler34, asm_inthandler0e
		global	asm_inthandler0d, asm_end_app
		global	memtest_sub
		global	farjmp, farcall
		global	asm_hrb_api, start_app
		global sys_nop , asm_sse_strcmp
		global	asm_memset,asm_strcpy,asm_strcmp,asm_memcpy,asm_memcmp,asm_memset
		global	support_apic, io_wrmsr, io_rdmsr, test2, get_lock, rdrand
		global	store_gdt, store_idt,sys_reboot,sys_call, test3, asm_task_switch32
		global asm_x64_cs_init,asm_get_cs,asm_sheet_refreshsub32
		extern	inthandler20, inthandler21
		extern	inthandler2c, inthandler0d
		extern	inthandler0c, inthandler0e , inthandler34
		extern	hrb_api 
		tpm_command equ 0xfed40000      ;tpm命令寄存器地址
		tpm_status equ 0xfed40014       ;tpm状态寄存器地址
; 一个接收指针参数的 vmxon 指令宏定义

;ip_size equ 4
;gpr_size equ 4
;rax equ <eax>
;rbx equ <ebx>
;rcx equ <ecx>
;rdx equ <edx>
;rsi equ <esi>
;rdi equ <edi>
;rsp equ <esp>
;rbp equ <ebp>


ip_size equ 8
gpr_size equ 8


section .text

io_hlt:	; void io_hlt(void);
		hlt
		ret

io_cli:	; void io_cli(void);

		cli
		ret

io_sti:	; void io_sti(void);
	
		sti
		ret

io_stihlt:	; void io_stihlt(void);
		sti
		hlt
		ret

io_in8:	; int io_in8(int port);
		mov		edx,ecx		; port
		xor		eax,eax
		in		al,dx
		ret

io_in16:	; int io_in16(int port);
		mov		edx,ecx		; port
		xor		eax,eax
		in		ax,dx
		ret

io_in32:	; int io_in32(int port);
		mov		edx,ecx		; port
		in		eax,dx
		ret

io_out8:	; void io_out8(int port, int data);
		mov		eax,edx		; data
		mov		edx,ecx	; port
		out		dx,al
		ret

io_out16:	; void io_out16(int port, int data);
		mov		eax,edx		; data
		mov		edx,ecx		; port
		out		dx,ax
		ret

io_out32:	; void io_out32(int port, int data);
		mov		eax,edx		; datas
		mov		edx,ecx		; port
		out		dx,eax
		ret

io_load_eflags:	; int io_load_eflags(void);
		pushfq		; push eflags とぁe��意味
		pop rax
		ret
io_store_eflags:	; void io_store_eflags(int eflags);
		;push eax
		push rcx
		popfq		; pop eflags とぁe��意味
		;pop eax
		ret

load_gdtr:		; void load_gdtr(int limit, int addr);
		;push eax
		push rdx
		sub esp,2
		mov [esp],cx
		lgdt [rsp]
		add rsp,10
		;64位0特权级所有段寄存器清零
		xor eax,eax
		mov ds,eax
		mov es,eax
		mov ss,eax
		mov gs,eax
		mov fs,eax
		;pop eax
		ret

load_idtr:		; void load_idtr(int limit, int addr);
		push rdx
		push cx
		lidt [rsp]
		add rsp,10
		ret

load_cr0:		; int load_cr0(void);
		mov		rax,cr0
		ret

store_cr0:		; void store_cr0(int cr0);
		mov		cr0,rcx
		ret
		
load_cr3:		; int load_cr3(void);
		mov		rax,cr3
		ret

store_cr3:		; void store_cr3(int cr0);
		mov		cr3,rcx
		ret

load_tr:		; void load_tr(int tr);
		ltr		cx
		ret
load_cr4:
		mov cr4,rcx
		ret
store_cr4:
		mov rax,cr4
		ret
		
asm_inthandler20:
		push rax
		
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rax,rsp
		mov 	rcx,rax
		mov		ax,ss
		mov		ds,ax
		mov		es,ax
		call	inthandler20
		
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		
		pop rax
		db 0x48
		iret

asm_inthandler21:
		push rax
		
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rax,rsp
		mov		rcx,rax
		mov		ax,ss
		mov		ds,ax
		mov		es,ax
		call	inthandler21
		
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rcx
		pop rax
		
		mov ds,eax
		pop rax
		mov es,eax
		pop rax
		db 0x48
		iret

asm_inthandler2c:
		push rax
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rcx,rsp
		xor		rax,rax
		mov		ds,ax
		mov		es,ax
		call	inthandler2c
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		pop rax
		db 0x48
		iret

asm_inthandler34:
		push rax
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rcx,rsp
		mov		ax,ss
		mov		ds,ax
		mov		es,ax
		call	inthandler34
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		pop rax
		db 0x48
		iret

asm_inthandler0c:
		push rax
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rcx,rsp
		mov		ax,ss
		mov		ds,ax
		mov		es,ax
		call	inthandler0c
		cmp		eax,0
		jne		asm_end_app
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		pop rax
		add		esp,4			; int 0x0c でも、これが忁e��e
		db 0x48
		iret

asm_inthandler0d:
		push rax
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rcx,rsp
		mov		ax,ss
		mov		ds,ax
		mov		es,ax
		call	inthandler0d
		cmp		eax,0			; ここだけ違ぁe
		jne		asm_end_app	; ここだけ違ぁe
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		pop rax
		add		esp,4			; int 0x0d では、これが忁e��e
		db 0x48
		iret

asm_inthandler0e:
		sti
		push rax
		mov eax,es
		push rax
		mov eax,ds
		push rax
		
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		
		mov		rcx,rsp
		mov		ax,ss
		mov		ds,ax
		mov		es,ax
		mov 	rdx,cr2 ;保存现场
		push 	rax
		call	inthandler0e
		cmp		eax,0
		jne		asm_end_app
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		pop rax
		add		esp,4
		db 0x48
		iret
memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		push	rdi						; �e�ebx, esi, edi も使ぁe��ぁe�eで�e�e
		push	rsi
		push	rbx
		mov		esi,0xaa55aa55			; pat0 = 0xaa55aa55;
		mov		edi,0x55aa55aa			; pat1 = 0x55aa55aa;
		mov		eax,[rsp+12+4]			; i = start;
mts_loop:
		mov		ebx,eax
		add		ebx,0xffc				; p = i + 0xffc;
		mov		edx,[ebx]				; old = *p;
		mov		[ebx],esi				; *p = pat0;
		xor		dword [ebx],0xffffffff	; *p ^= 0xffffffff;
		cmp		edi,[ebx]				; if (*p != pat1) goto fin;
		jne		mts_fin
		xor		dword [ebx],0xffffffff	; *p ^= 0xffffffff;
		cmp		esi,[ebx]				; if (*p != pat0) goto fin;
		jne		mts_fin
		mov		[ebx],edx				; *p = old;
		add		eax,0x1000				; i += 0x1000;
		cmp		eax,[rsp+12+8]			; if (i <= end) goto mts_loop;
		jbe		mts_loop
		pop		rbx
		pop		rsi
		pop		rdi
		ret
mts_fin:
		mov		[ebx],edx				; *p = old;
		pop		rbx
		pop		rsi
		pop		rdi
		ret

farjmp:		; void farjmp(int eip, int cs);
		jmp		far	[rsp+ip_size]				; eip, cs
		ret

farcall:		; void farcall(int eip, int cs);
		call	far	[rsp+ip_size]				; eip, cs
		ret

asm_hrb_api:
		sti
		mov eax,es
		push rax
		mov eax,ds
		push rax
		

		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push rsp; 保存�eためのpush
		

		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push rsp; hrb_apiにわたすため�epush
		mov		ax,ss
		mov		ds,ax		; os用のセグメントをdsとesにも�eれる
		mov		es,ax
		call	hrb_api
		cmp		eax,0		; eaxぁeでなければアプリ終亁e�e琁e
		jne		asm_end_app
		add		esp,32
		
		pop rsp
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		
		pop rax
		mov ds,eax
		pop rax
		mov es,eax
		db 0x48
		iret
asm_end_app:
;	eaxはtss.esp0の番地
		mov		rsp,[rax]
		mov		dword [eax+4],0
		pop rsp
		pop rbp
		pop rdi
		pop rsi
		pop rbx
		pop rdx
		pop rcx
		pop rax
		ret					; cmd_appへ帰めe

start_app:		; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
		push rax
		push rcx
		push rdx
		push rbx
		push rsi
		push rdi
		push rbp
		push rsp		; 32ビットレジスタを�e部保存しておく
		
		mov		rbp,[rsp+ip_size+gpr_size*8]	; tss.esp0の番地
		mov		[rbp  ],rsp		; os用のespを保孁e
		mov		[rbp+8],ss		; os用のssを保孁e
		mov		es,r9d
		mov		ds,r9d
		mov		fs,r9d
		mov		gs,r9d
		;db 0x0f,0x35;sys_ret
;	以下�eretfでアプリに行かせるためのスタチe��調整
		or		eax,3			; アプリ用のセグメント番号に3をorする
		or		ebx,3			; アプリ用のセグメント番号に3をorする
		push	r9				; アプリのss
		push	r8				; アプリのesp
		push	rdx				; アプリのcs
		push	rcx				; アプリのeip
		retf
;	アプリが終亁e��てもここには来なぁe

; int strcmp(const char *s1, const char *s2)
asm_strcmp:
    mov rsi, rcx  ; s1 -> rsi
    mov rdi, rdx  ; s2 -> rdi
    cld           ; 确保DF标志位为0，正向扫描
    mov al, 0     ; 设置终止字符
    repe cmpsb    ; 比较直到发现不同的字节或遇到'\0'
    movzx eax, byte [rsi - 1]
    movzx edx, byte [rdi - 1]
    sub eax, edx  ; 计算差值
    ret

; char* strcpy(char *dest, const char *src)
asm_strcpy:
    mov rdi, rdx  ; src -> rdi
	push rcx
    mov rcx, -1   ; 计数器初始化为 -1
    xor al, al    ; 终止字符 '\0'
    cld
    repne scasb   ; 计算 src 的长度，包括 '\0'
    not rcx       ; 计算实际长度（包含 '\0'）
    mov rsi, rdx  ; src -> rsi
	pop rdi		  ; dest -> rdi
    mov rcx, rcx  ; 设置复制的字节数
    rep movsb     ; 复制整个字符串
    mov rax, rcx  ; 返回 dest
    ret

; int memcmp(const void *s1, const void *s2, size_t n)
asm_memcmp:
    mov rsi, rcx  ; s1 -> rsi
    mov rdi, rdx  ; s2 -> rdi
    mov rcx, r8   ; n -> rcx
    cld
    repe cmpsb    ; 比较 n 字节
    jne .diff
    xor eax, eax  ; 若相等，返回0
    ret
.diff:
    movzx eax, byte [rsi - 1]
    movzx edx, byte [rdi - 1]
    sub eax, edx  ; 计算差值
    ret

; void* memcpy(void *dest, const void *src, size_t n)
asm_memcpy:
    mov rsi, rdx  ; src -> rsi
    mov rdi, rcx  ; dest -> rdi
    mov rcx, r8   ; n -> rcx
    cld
    rep movsb     ; 复制 n 字节
    mov rax, rcx  ; 返回 dest
    ret
	
asm_memset:	;void memset(char* s,int num,int longer)
	push rdi
	push rax
	push rcx
	mov rdi,[rsp+ip_size]
	mov rax,[rsp+ip_size+gpr_size]
	mov rcx,[rsp+ip_size+gpr_size*2]
	rep stosb
	pop rcx
	pop rax
	pop rdi
	ret
support_apic: ;int support_apic(void):
	push rdx
	mov eax,1
	;db 0x0f,0xa2 ;cpuid 
	cpuid
	bt edx,9
	setc al
	movzx rax,al
	pop rdx
	ret
io_rdmsr: ;void io_rdmsr(unsigned long long ecx,unsigned long long* edx,unsigned long long* eax)
	;db 0x0f,0x32 ;rdmsr
	push rdx
	rdmsr
	mov [r8],eax
	pop rax
	mov [rax],edx
	ret
io_wrmsr: ;void io_wrmsr(unsigned long long ecx,unsigned long long edx,unsigned long long eax)
	;db 0x0f,0x30 ;wrmsr
	mov rax,r8
	wrmsr
	ret
test2: ;void test(int eax,int ebx)
	push rax
	push rbx
	mov eax,[rsp+ip_size+gpr_size*2]
	mov ebx,[rsp+ip_size+gpr_size*3]
	db 0x0f,0x0b ;ud2
.fin:
	cli
	hlt
	jmp .fin
	pop rbx
	pop rax
	ret
get_lock: ;int get_lock(int* lock,int bit)
	lock bts dword [rcx],edx
	xor rax,rax
	adc eax,eax
	ret
store_gdt: ;void store_gdt(void* addr)
	sgdt [rcx]
	ret
store_idt: ;void store_gdt(void* addr)
	sidt [rcx]
	ret
sys_reboot: ;void sys_reboot(void)
	cli
	mov rax,cr0
	mov dx,0x64
	mov ax,0xfe
	out dx,ax
.fin:
	hlt
	jmp .fin
sys_call:
	db 0x0f,0x35 ;sysexit
test3:
	db 0xf4
.fin:
	jmp .fin
support_rand: ;int support_rand()
	xor eax,eax
	inc eax
	ret
	
	push rbx
	push rcx
	push rdx
	mov eax,1
	db 0x0f,0xa2 ;cpuid
	and ecx,0x40000000
	db 0x0f,0x92,0xc0;setc al
	pop rdx
	pop rcx
	pop rbx
	ret
rdrand: ;int rdrand()
	db 0x0f,0xc7,0xf0 ;rdrand eax
	rdrand eax
	ret
_invlpg:
	ret
tpm_init:
	;初始化tpm
	push rax
    mov eax, 0x6f000
    mov dword [tpm_command], eax
    call tpm_wait_for_status
	pop rax
	ret
tpm_get_a_key:
	
tpm_wait_for_status:
        ;等待tpm状态就绪
        mov eax, dword [tpm_status]
        and eax, 0x01800000
        cmp eax, 0x00800000
        jne tpm_wait_for_status
		ret
sys_nop:
	nop
	ret
;unsigned int asm_sse_strcmp(char* str,char* str,unsigned,int longer)
asm_sse_strcmp:
	push rbp
	mov rbp,rsp
	
	mov rax,rcx
	;mov edx,rdx
	mov rcx,r8
	push rbx
	push rsi
	push rdi
	mov rsi,rax
	mov rdi,rdx
	mov rdx,rcx
	;清零计数器
	xor rbx,rbx 
	clts
	pxor xmm2,xmm2
.lp:
	clts
	;将字符串剩余长度读入
	;lea eax,[edx-ebx]
	mov rax,rdx
	sub rax,rbx
	mov rcx,rax
	movdqu xmm0, [rsi+rbx]
	movdqu xmm1, [rdi+rbx]
	pcmpestri xmm0,xmm1,0x18
	;不是16证明有不一样的部分
	cmp rcx,16
	jne .fin
	
	;检查两个字符串中是否有0 如果有 证明其中一个字符串已经结束
	mov rbx,rcx
	pcmpistri xmm0,xmm2,0x04
	;如果不是16 证明有一个字符是空字符 字符串结束
	cmp rcx,16
	jne .fin2
	
	pcmpistri xmm1,xmm2,0x04
	;如果不是16 证明有一个字符是空字符 字符串结束
	cmp rcx,16
	jne .fin2
	
	add rbx,16
	;最后 比较是否达到了界限值
	cmp rdx,rbx
	jbe .fin2
	
	jmp .lp
.fin2:
	;没有异常
	xor rax,rax
	pop rdi
	pop rsi
	pop rbx
	pop rbp
	ret
.fin:
	lea rax,[rbx+rcx]
	inc rax
	
	pcmpistri xmm0,xmm1,0x18
	;最后判断一次是不是遇到了字符串结尾
	cmp rcx,16
	je .fin2
	
	pop rdi
	pop rsi
	pop rbx
	pop rbp
	ret
;asm_sse_strtest(char* str,char test[16])
_asm_sse_strtest:
	push rbp
	mov rbp,rsp
	mov rax,rcx
	;mov rdx,[rbp+gpr_size*3]
	push rbx
	push rsi
	push rdi
	movdqu xmm1,[edx]
	;清零计数器
	xor rax,rax 
	clts
	pxor xmm2,xmm2
.lp:
	clts
	movdqu xmm0, [esi+eax]
	pcmpistri xmm0,xmm1,0x14
	cmp rcx,16
	;0
	jne .fin
	
	mov rbx,rcx
	pcmpistri xmm0,xmm2,0x04
	;如果是16 证明没有任何一个字符是空字符 字符串没有结束
	cmp rcx,16
	jne .fin2
	
	add rax,16
	jmp .lp
.fin2:
	mov rax,rbx
.fin:
	add rax,rcx
	pop rdi
	pop rsi
	pop rbx
	pop rbp
	ret
;asm_task_switch32(unsigned int *old_esp,unsigned int *new_esp)
asm_task_switch32:
	push rbp
	;mov ecx,[rbp+8]
	;mov edx,[rbp+12]
	call .start
	;恢复环境的代码
	pop rbp
	ret
.start:
	;保存环境的代码
	push rbx
	push rsi
	push rdi
	push rbp
	;cr3
	mov rax,cr3
	push rax
	xor rax,rax
	;flags
	pushfq
	;tr
	push rax
	str [rsp]
	;ldtr
	push rax
	sldt [rsp]
	;64位被调用者寄存器
	push r12
	push r13
	push r14
	push r15
	mov [rcx],rsp
	;加载环境的代码
	mov rsp,[rdx]
	pop r15
	pop r14
	pop r13
	pop r12
	;ldtr
	lldt [rsp]
	pop rax
	;tr
	;ltr [esp]
	pop rax
	;flags
	popfq
	;cr3
	pop rax
	mov cr3,rax
	pop rbp
	pop rdi
	pop rsi
	pop rbx
	ret
bits 32
;asm_task_switch32_2(unsigned int *old_esp,tss32 *new_esp)
asm_task_switch32_2:
	push ebp
	mov ebp,esp
	mov ecx,[ebp+8]
	mov edx,[ebp+12]
	call .start
	;恢复环境的代码
	pop ebp
	ret
.start:
	;保存环境的代码
	push ebx
	push esi
	push edi
	push ebp
	mov eax,cr3
	push eax
	pushfd
	push eax
	mov [ecx],esp
	;加载环境的代码
	mov eax,[edx]
	mov edx,[eax+15*4]
	mov ebp,edx
	mov edx,[eax+16*4]
	mov esi,edx
	mov edx,[eax+17*4]
	mov edi,edx
	mov edx,[eax+18*4]
	mov es,edx
	mov edx,[eax+22*4]
	mov fs,edx
	mov edx,[eax+23*4]
	mov gs,edx
	mov esp,eax
	popad
	retf
	ret
bits 64
;void asm_vmptrld(unsigned int vmcs_point)
global asm_vmptrld
asm_vmptrld:
	vmptrld [rcx]
	ret
;void asm_vmptrst(unsigned int* vmcs_point)
global asm_vmptrst
asm_vmptrst:
	vmptrst [rcx]
	ret
;void asm_vmclear(unsigned int vmcs_point)
global asm_vmclear
asm_vmclear:
	vmclear [rcx]
	ret
;unsigned long long asm_vmread(unsigned int index)
global asm_vmread
asm_vmread:
	vmread rax,rcx
	ret
;void asm_vmread(unsigned int index,void data)
global asm_vmwrite
asm_vmwrite:
	vmwrite rcx,rdx
	ret
global asm_vmxon
asm_vmxon:
	vmxon [rcx]
asm_x64_cs_init:
	db 0x48
	retf
asm_get_cs:
	mov rax,cs
	ret
global asm_vmlaunch
asm_vmlaunch:
	vmlaunch
	ret
global asm_vmresume
asm_vmresume:
	vmresume
	ret	
;void asm_sheet_refreshsub32(void* struct SHEET_REFRESH_PACK_32 pack);
asm_sheet_refreshsub32:
	cli
	push rbx
	push rsi
	push rdi
	push r12
	push r13
	push r14
	push r15
	mov rsi,[rcx];源变址
	mov rdi,[rcx+8];目的变址
	mov r8,[rcx+16];源图层宽度
	mov r9,[rcx+24];目的图层宽度
	mov r10,[rcx+32];刷新窗口宽度
	mov r11,[rcx+40];刷新窗口高度
	mov r12,[rcx+48];sid_addr
	mov r13,[rcx+56];sid
	mov [rcx+56+4],r13d;低端32位sid数据复制到高32位
	mov r13,[rcx+56];sid
	mov [rcx+64],r13;sid
	movdqu xmm4,[rcx+56]
	xor rax,rax
	xor rdx,rdx
	
.start:	
	movdqu xmm1,[rsi+rax*4]
	movdqu xmm2,[rdi+rax*4]
	movdqu xmm3,[r12+rax*4];sid序列
	cmpps xmm3,xmm4,0
	pand xmm1,xmm3;清空无效数据
	pandn xmm3,xmm2;清空无效数据
	por xmm1,xmm3;混合
	movdqu [rdi+rax*4],xmm1
	add eax,4
	cmp rax,r10
	jb .start
	je .fin;正好相等
	sub eax,4
.continued:
	;这里说明本行剩下的不足4个像素
	mov ebx,[rsi+rax*4]
	mov r14d,[rdi+rax*4]
	mov r15d,[r12+rax*4]
	cmp r15d,r13d
	cmovne rbx,r14
	mov [rdi+rax*4],ebx
	inc rax
	cmp rax,r10
	jne .continued
.fin:
	xor eax,eax
	inc rdx
	lea rsi,[rsi+r8*4]
	lea rdi,[rdi+r9*4]
	lea r12,[r12+r9*4]
	cmp rdx,r11
	jb .start
	pop r15
	pop r14
	pop r13
	pop r12
	pop rdi
	pop rsi
	pop rbx
	sti
	ret


;void asm_sheet_refreshsub32a(void* struct SHEET_REFRESH_PACK_32 pack);
asm_sheet_refreshsub32a:
	push rbx
	push rsi
	push rdi
	push r12
	push r13
	push r14
	push r15
	mov rsi,[rcx];源变址
	mov rdi,[rcx+8];目的变址
	mov r8,[rcx+16];源图层宽度
	mov r9,[rcx+24];目的图层宽度
	mov r10,[rcx+32];刷新窗口宽度
	mov r11,[rcx+40];刷新窗口高度
	mov r12,[rcx+48];sid_addr
	mov r13,[rcx+56];sid
	movlpd xmm4,[rcx+56]
	pshufd xmm4,xmm4,0;低端32位sid数据复制扩展到整个xmm
	xor rax,rax
	xor rdx,rdx
	
.start:	
	movdqa xmm1,[rsi+rax*4]
	movdqa xmm2,[rdi+rax*4]
	movdqa xmm3,[r12+rax*4];sid序列
	cmpps xmm3,xmm4,0
	pand xmm1,xmm3;清空无效数据
	pandn xmm3,xmm2;清空无效数据
	por xmm1,xmm3;混合
	movdqa [rdi+rax*4],xmm1
	add eax,4
	cmp rax,r10
	jb .start
	je .fin;正好相等
	sub eax,4
.continued:
	;这里说明本行剩下的不足4个像素
	mov ebx,[rsi+rax*4]
	mov r14d,[rdi+rax*4]
	mov r15d,[r12+rax*4]
	cmp r15d,r13d
	cmovne rbx,r14
	mov [rdi+rax*4],ebx
	inc rax
	cmp rax,r10
	jne .continued
.fin:
	xor eax,eax
	inc rdx
	lea rsi,[rsi+r8*4]
	lea rdi,[rdi+r9*4]
	lea r12,[r12+r9*4]
	cmp rdx,r11
	jb .start
	pop r15
	pop r14
	pop r13
	pop r12
	pop rdi
	pop rsi
	pop rbx
	ret

extern console_task
global asm_console_task_start_64
;这个函数的目的是因为x64架构下无法使用栈传递第一个和第二个参数 但是为了任务切换效率无法切换调用者保存寄存器 因此使用这个函数中转
asm_console_task_start_64:
	mov rcx,[rsp+8]
	mov rdx,[rsp+16]
	jmp console_task