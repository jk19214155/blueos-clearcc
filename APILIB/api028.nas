[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api028.nas"]

		GLOBAL	_api_getevent

[SECTION .text]

_api_getevent:		; //_api_getevent(int* regs);
		push eax
		mov eax,[esp+8];获取regs
		pushad ;eax最先入栈
		push eax
		mov edx,28
		int 0x40
		;交换eax的值
		xor eax,[esp]
		xor [esp],eax
		xor eax,[esp]
		mov [eax+4],ecx
		mov [eax+8],edx
		mov [eax+12],ebx
		mov [eax+16],esi
		mov [eax+20],edi
		mov [eax+24],ebp
		pop ebx
		mov [eax],ebx
		popad
		pop eax
		RET
