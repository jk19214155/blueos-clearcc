; haribote-os boot asm
; TAB=4

;[INSTRSET "i486p"]

; bootpack的启动
		bits 32
start:
		cli
		mov ebx,0x280000
		LGDT [GDTR0+ebx]
		jmp .next
.next:
		mov 	EBP,ESP
		MOV		EBX,bootpack+0x280000
		MOV		ECX,[EBX+16];数据段大小
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要传送的数据
		MOV		ESI,[EBX+20]	; 数据开始部分
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; esp初始值
		CALL	memcpy
		
		xor eax,eax
		mov cr4,eax
		
skip:
		MOV		ESP,[EBX+12]	; esp初期値
		push  	dword[ebp+4]		;传入参数
		push 	dword bootpack+0x280000
		mov eax,0x28001b+bootpack
		call	eax
		;jmp far 0x10:0x1b
		ret
memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 引き算した結果が0でなければmemcpyへ
		RET
		ALIGNB	16
GDT0:
		RESB	8				; ヌルセレクタ
		DW		0xffff,0x0000,0x9200,0x00cf	; 読み書き可能セグメント32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 実行可能セグメント32bit（bootpack用）
		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0+0x280000
		ALIGNB	16
bootpack:
