[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api028.nas"]

		GLOBAL	_api_getlang

[SECTION .text]

_api_sysenter:		; int api_getlang(void);
		db 0x0f,0x34 ;sysenter
		RET
