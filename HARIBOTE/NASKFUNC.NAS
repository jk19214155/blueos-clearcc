; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 繧ｪ繝悶ず繧ｧ繧ｯ繝医ヵ繧｡繧､繝ｫ繧剃ｽ懊ｋ繝｢繝ｼ繝・
[INSTRSET "i486p"]				; 486縺ｮ蜻ｽ莉､縺ｾ縺ｧ菴ｿ縺・◆縺・→縺・≧險倩ｿｰ
[BITS 32]						; 32繝薙ャ繝医Δ繝ｼ繝臥畑縺ｮ讖滓｢ｰ隱槭ｒ菴懊ｉ縺帙ｋ
[FILE "naskfunc.nas"]			; 繧ｽ繝ｼ繧ｹ繝輔ぃ繧､繝ｫ蜷肴ュ蝣ｱ

		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_load_cr0, _store_cr0
		GLOBAL	_load_cr3, _store_cr3
		GLOBAL	_load_tr
		GLOBAL	_asm_inthandler20, _asm_inthandler21
		GLOBAL	_asm_inthandler2c, _asm_inthandler0c, _asm_inthandler0e
		GLOBAL	_asm_inthandler0d, _asm_end_app
		GLOBAL	_memtest_sub
		GLOBAL	_farjmp, _farcall
		GLOBAL	_asm_hrb_api, _start_app
		;GLOBAL	_memset,_strcpy,_strcmp
		global	_support_apic, _io_wrmsr, _io_rdmsr, _test2, _get_lock, _rdrand
		global	_store_gdt, _store_idt,_sys_reboot,_sys_call,_test3
		EXTERN	_inthandler20, _inthandler21
		EXTERN	_inthandler2c, _inthandler0d
		EXTERN	_inthandler0c, _inthandler0e
		EXTERN	_hrb_api

[SECTION .text]

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

_io_in16:	; int io_in16(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

_io_out16:	; void io_out16(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; PUSH EFLAGS 縺ｨ縺・≧諢丞袖
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS 縺ｨ縺・≧諢丞袖
		RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

_load_idtr:		; void load_idtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET

_load_cr0:		; int load_cr0(void);
		MOV		EAX,CR0
		RET

_store_cr0:		; void store_cr0(int cr0);
		MOV		EAX,[ESP+4]
		MOV		CR0,EAX
		nop
		nop
		RET
		
_load_cr3:		; int load_cr3(void);
		MOV		EAX,CR3
		RET

_store_cr3:		; void store_cr3(int cr0);
		MOV		EAX,[ESP+4]
		MOV		CR3,EAX
		RET

_load_tr:		; void load_tr(int tr);
		LTR		[ESP+4]			; tr
		RET

_asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler21:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler21
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler0c:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0c
		CMP		EAX,0
		JNE		_asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; INT 0x0c 縺ｧ繧ゅ√％繧後′蠢・ｦ・
		IRETD

_asm_inthandler0d:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0d
		CMP		EAX,0			; 縺薙％縺縺鷹＆縺・
		JNE		_asm_end_app	; 縺薙％縺縺鷹＆縺・
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; INT 0x0d 縺ｧ縺ｯ縲√％繧後′蠢・ｦ・
		IRETD

_asm_inthandler0e:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		MOV 	eax,cr2 ;菫晏ｭ倡鴫蝨ｺ
		PUSH 	eax
		CALL	_inthandler0e
		CMP		EAX,0
		JNE		_asm_end_app
		ADD 	ESP,4 ;貂・勁譬井ｸｭ逧・r2
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4
		IRETD
_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH	EDI						; ・・BX, ESI, EDI 繧ゆｽｿ縺・◆縺・・縺ｧ・・
		PUSH	ESI
		PUSH	EBX
		MOV		ESI,0xaa55aa55			; pat0 = 0xaa55aa55;
		MOV		EDI,0x55aa55aa			; pat1 = 0x55aa55aa;
		MOV		EAX,[ESP+12+4]			; i = start;
mts_loop:
		MOV		EBX,EAX
		ADD		EBX,0xffc				; p = i + 0xffc;
		MOV		EDX,[EBX]				; old = *p;
		MOV		[EBX],ESI				; *p = pat0;
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		EDI,[EBX]				; if (*p != pat1) goto fin;
		JNE		mts_fin
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		ESI,[EBX]				; if (*p != pat0) goto fin;
		JNE		mts_fin
		MOV		[EBX],EDX				; *p = old;
		ADD		EAX,0x1000				; i += 0x1000;
		CMP		EAX,[ESP+12+8]			; if (i <= end) goto mts_loop;
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX				; *p = old;
		POP		EBX
		POP		ESI
		POP		EDI
		RET

_farjmp:		; void farjmp(int eip, int cs);
		JMP		FAR	[ESP+4]				; eip, cs
		RET

_farcall:		; void farcall(int eip, int cs);
		CALL	FAR	[ESP+4]				; eip, cs
		RET

_asm_hrb_api:
		STI
		PUSH	DS
		PUSH	ES
		PUSHAD		; 菫晏ｭ倥・縺溘ａ縺ｮPUSH
		PUSHAD		; hrb_api縺ｫ繧上◆縺吶◆繧√・PUSH
		MOV		AX,SS
		MOV		DS,AX		; OS逕ｨ縺ｮ繧ｻ繧ｰ繝｡繝ｳ繝医ｒDS縺ｨES縺ｫ繧ょ・繧後ｋ
		MOV		ES,AX
		CALL	_hrb_api
		CMP		EAX,0		; EAX縺・縺ｧ縺ｪ縺代ｌ縺ｰ繧｢繝励Μ邨ゆｺ・・逅・
		JNE		_asm_end_app
		ADD		ESP,32
		POPAD
		POP		ES
		POP		DS
		IRETD
_asm_end_app:
;	EAX縺ｯtss.esp0縺ｮ逡ｪ蝨ｰ
		MOV		ESP,[EAX]
		MOV		DWORD [EAX+4],0
		POPAD
		RET					; cmd_app縺ｸ蟶ｰ繧・

_start_app:		; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
		PUSHAD		; 32繝薙ャ繝医Ξ繧ｸ繧ｹ繧ｿ繧貞・驛ｨ菫晏ｭ倥＠縺ｦ縺翫￥
		MOV		EDX,[ESP+36]	; 繧｢繝励Μ逕ｨ縺ｮEIP
		MOV		EAX,[ESP+40]	; 繧｢繝励Μ逕ｨ縺ｮCS
		MOV		ECX,[ESP+44]	; 繧｢繝励Μ逕ｨ縺ｮESP
		MOV		EBX,[ESP+48]	; 繧｢繝励Μ逕ｨ縺ｮDS/SS
		MOV		EBP,[ESP+52]	; tss.esp0縺ｮ逡ｪ蝨ｰ
		MOV		[EBP  ],ESP		; OS逕ｨ縺ｮESP繧剃ｿ晏ｭ・
		MOV		[EBP+4],SS		; OS逕ｨ縺ｮSS繧剃ｿ晏ｭ・
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX
		;db 0x0f,0x35;sys_ret
;	莉･荳九・RETF縺ｧ繧｢繝励Μ縺ｫ陦後°縺帙ｋ縺溘ａ縺ｮ繧ｹ繧ｿ繝・け隱ｿ謨ｴ
		OR		EAX,3			; 繧｢繝励Μ逕ｨ縺ｮ繧ｻ繧ｰ繝｡繝ｳ繝育分蜿ｷ縺ｫ3繧丹R縺吶ｋ
		OR		EBX,3			; 繧｢繝励Μ逕ｨ縺ｮ繧ｻ繧ｰ繝｡繝ｳ繝育分蜿ｷ縺ｫ3繧丹R縺吶ｋ
		PUSH	EBX				; 繧｢繝励Μ縺ｮSS
		PUSH	ECX				; 繧｢繝励Μ縺ｮESP
		PUSH	EAX				; 繧｢繝励Μ縺ｮCS
		PUSH	EDX				; 繧｢繝励Μ縺ｮEIP
		RETF
;	繧｢繝励Μ縺檎ｵゆｺ・＠縺ｦ繧ゅ％縺薙↓縺ｯ譚･縺ｪ縺・
_strcmp:	;int strcmp(char* s,char* t,int longer);
	push esi;
	push edi;
	push ecx;
	mov esi,[esp+4]
	mov edi,[esp+8]
	mov ecx,[esp+12]
	repne cmpsd
	mov eax,ecx
	pop ecx
	pop edi
	pop esi
	ret
_strcpy:	;void strcpy(char* s,char* t,int longer);
	push esi;
	push edi;
	push ecx;
	mov esi,[esp+4]
	mov edi,[esp+8]
	mov ecx,[esp+12]
	db 0xf3,0xa5;rep movsb
	pop ecx
	pop edi
	pop esi
	ret
_memset:	;void memset(char* s,int num,int longer)
	push edi
	push eax
	push ecx
	mov edi,[esp+4]
	mov eax,[esp+8]
	mov ecx,[esp+12]
	rep stosb
	pop ecx
	pop eax
	pop edi
	ret
_support_apic: ;int support_apic(void):
	push edx
	mov eax,1
	db 0x0f,0xa2 ;cpuid 
	bt edx,9
	setc al
	movzx eax,al
	pop edx
	ret
_io_rdmsr: ;void io_rdmsr(int* edx,int* eax,int ecx)
	push edx
	push ecx
	push eax
	mov ecx,[esp+24]
	db 0x0f,0x32 ;rdmsr
	mov ecx,[esp+16]
	mov [ecx],edx
	mov ecx,[esp+20]
	mov [ecx],eax
	pop eax
	pop ecx
	pop edx
	ret
_io_wrmsr: ;void io_wrmsr(int edx,int eax,int ecx)
	push edx
	push ecx
	push eax
	mov edx,[esp+16]
	mov eax,[esp+20]
	mov ecx,[esp+24]
	db 0x0f,0x30 ;wrmsr
	pop eax
	pop ecx
	pop edx
	ret
_test2: ;void test(int eax,int ebx)
	push eax
	push ebx
	mov eax,[esp+12]
	mov ebx,[esp+16]
	db 0x0f,0x0b ;ud2
.fin:
	cli
	hlt
	jmp .fin
	pop ebx
	pop eax
	ret
_get_lock: ;int get_lock(int* lock,int bit)
	push ebp
	mov ebp,esp
	push ecx
	push edx
	mov ebx,[ebp+8]
	mov ecx,[ebp+12]
	lock bts dword [ebx],ecx
	xor eax,eax
	adc eax,eax
	pop edx
	pop ecx
	mov esp,ebp
	pop ebp
	ret
_store_gdt: ;void store_gdt(void* addr)
	push eax
	mov eax,[esp+8]
	sgdt [eax]
	pop eax
	ret
_store_idt: ;void store_gdt(void* addr)
	push eax
	mov eax,[esp+8]
	sidt [eax]
	pop eax
	ret
_sys_reboot: ;void sys_reboot(void)
	cli
	mov eax,cr0
	mov dx,0x64
	mov ax,0xfe
	out dx,ax
.fin:
	hlt
	jmp .fin
_sys_call:
	db 0x0f,0x35 ;sysexit
_test3:
	db 0xf4
.fin:
	jmp .fin
_support_rand: ;int support_rand()
	
	push ebx
	push ecx
	push edx
	mov eax,1
	db 0x0f,0xa2 ;cpuid
	and ecx,0x400000000
	db 0x0f,0x92,0xc0;setc al
	pop edx
	pop ecx
	pop ebx
	ret
_rdrand: ;int rdrand()
	db 0x0f,0xc7,0xf0 ;rnrand eax
	ret
_invlpg:
