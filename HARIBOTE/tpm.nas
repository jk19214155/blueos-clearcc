[INSTRSET "i486p"]
[FORMAT "WCOFF"]
[BITS 32]
[FILE "tpm.nas"]

tpm_command equ 0xfed40020      ;TPM命令寄存器地址
tpm_status equ 0xfed4001c       ;TPM状态寄存器地址
tpm_flush equ 0xfed40024		;TPM数据缓冲

[SECTION .text]
global tpm_init

tpm_init:
    ;初始化TPM
    mov eax, 0x6f000
    mov dword [tpm_command], eax
    jmp tpm_wait_for_status

    ;生成RSA密钥对
    mov eax, 0x00010001          ;指定密钥参数（这里使用2048位RSA密钥）
    mov ebx, 0x00000000
    mov ecx, 0x00000001			;这个寄存器指定密钥包的权限控制，其值为 0x00000001，表示密钥包可以被所有用户读取和使用。
    mov edx, 0x00000001			这个寄存器用于指定密钥包的使用控制，其值为 0x00000001，表示密钥包可以被所有用户使用
    mov esi, 0x00000000
    mov edi, 0x00000800			;这个寄存器指定密钥包的大小，其值为 0x00000800，表示密钥包的大小为 2048 字节。
    mov ebp, 0x00000000
    mov dword [tpm_command], 0x00000047 ;发送TPM_CreateWrapKey命令
    jmp tpm_wait_for_status



    ;检查操作是否成功
    mov eax, dword [tpm_command]
    and eax, 0xff
    cmp eax, 0
    jne .error

    ;从TPM中获取密钥
    mov eax, 0x00000001
    mov dword [tpm_command], 0x0000007f ;发送TPM_Load命令
    jmp tpm_wait_for_status


    ;检查操作是否成功
    mov eax, dword [tpm_command]
    and eax, 0xff
    cmp eax, 0
    jne .error

    ;将密钥保存在变量中
    mov ebx, dword [tpm_command + 0x04] ;获取密钥句柄
    mov ecx, dword [tpm_command + 0x08] ;获取密钥长度
    mov esi, dword [tpm_command + 0x0c] ;获取密钥数据

    ;在这里使用密钥进行加密、解密等操作

    ;释放密钥
    mov eax, 0x00000001
    mov dword [tpm_command], 0x0000005b ;发送TPM_FlushContext命令
    jmp tpm_wait_for_status
	
	;是否操作成功
	mov eax, dword [tpm_command]
	and eax, 0xff
	cmp eax, 0
	jne .error


tpm_wait_for_status:
        ;等待TPM状态就绪
        mov eax, dword [tpm_status]
        and eax, 0x01800000
        cmp eax, 0x00800000
        jne tpm_wait_for_status



;使用TPM私钥加密数据
;密钥句柄存储在 ebx 中
;待加密数据长度存储在 ecx 中
;待加密数据存储在 esi 中
;加密结果存储在 edi 中

mov eax, ebx                  ;将密钥句柄存入eax中
shl eax, 8                    ;将eax左移8位，将命令码移入高8位
or eax, 0x00000200            ;设置加密模式为OAEP
mov dword [tpm_command], eax  ;将命令写入TPM命令寄存器

mov dword [tpm_command + 0x04], ecx  ;设置待加密数据长度
mov dword [tpm_command + 0x08], esi  ;设置待加密数据

jmp tpm_wait_for_status        ;等待TPM操作完成

mov eax, dword [tpm_command]   ;获取TPM返回值
and eax, 0xff
cmp eax, 0                     ;检查是否操作成功
jne .error

mov edi, dword [tpm_command + 0x0c] ;获取加密结果


;使用TPM公钥解密数据
;密钥句柄存储在 ebx 中
;待解密数据长度存储在 ecx 中
;待解密数据存储在 esi 中
;解密结果存储在 edi 中

mov eax, ebx                  ;将密钥句柄存入eax中
shl eax, 8                    ;将eax左移8位，将命令码移入高8位
or eax, 0x00000400            ;设置解密模式为OAEP
mov dword [tpm_command], eax  ;将命令写入TPM命令寄存器

mov dword [tpm_command + 0x04], ecx  ;设置待解密数据长度
mov dword [tpm_command + 0x08], esi  ;设置待解密数据

jmp tpm_wait_for_status        ;等待TPM操作完成

mov eax, dword [tpm_command]   ;获取TPM返回值
and eax, 0xff
cmp eax, 0                     ;检查是否操作成功
jne .error

mov edi, dword [tpm_command + 0x0c] ;获取加密结果


; PCR选择器（用于选择要更新的PCR）
PCR_SELECTION struct
    size    DWORD   ?
    pcrMask BYTE    24 dup(?)
PCR_SELECTION ends

; PCR值（存储PCR的值）
PCRVALUE struct
    size    DWORD   ?
    pcr     BYTE    20 dup(?)
PCRVALUE ends

; 获取PCR的值
getPCRValue proc pcrNum:DWORD, pcrValue:PTR PCRVALUE
    LOCAL pcrSel : PCR_SELECTION
    mov pcrSel.size, SIZEOF PCR_SELECTION
    mov pcrSel.pcrMask[pcrNum], 1
    mov eax, TPM_ORD_PcrRead
    mov ebx, [hTPM];这里是TPM句柄
    mov ecx, pcrSel
    mov edx, pcrValue
    call sendTPMCommand
    ret
getPCRValue endp

; 比较PCR值
comparePCR proc pcrNum:DWORD, expectedValue:PTR BYTE
    LOCAL pcrValue : PCRVALUE
    invoke getPCRValue, pcrNum, ADDR pcrValue
    cmp BYTE PTR pcrValue.pcr, expectedValue
    ret
comparePCR endp




; Inputs:
; [esi] = message to encrypt
; [edi] = public key exponent (big-endian)
; [ebp] = public key modulus (big-endian)
; [ecx] = size of modulus in bytes
; [ebx] = size of exponent in bytes
;
; Outputs:
; [esi] = encrypted message
;
; Clobbers:
; eax, ecx, edx, esi, edi

    ; Save registers
    push ebp
    push ebx
    push edi

    ; Initialize TPM command header
    mov eax, 0x00010096          ; TPM_RSA_Encrypt command
    mov edx, ecx                ; TPM expects size of modulus in edx
    mov cl, 0x0a                ; TPM expects exponent size in cl
    mov ebx, 0                  ; Reserved parameter
    mov [hTPM_Cmd], eax         ; Store command code in TPM command buffer
    mov [hTPM_Cmd + 4], ebx     ; Store reserved parameter in TPM command buffer
    mov [hTPM_Cmd + 8], edx     ; Store size of modulus in TPM command buffer
    mov [hTPM_Cmd + 12], ecx    ; Store size of exponent in TPM command buffer
    mov [hTPM_Cmd + 16], ebp    ; Store modulus in TPM command buffer
    mov [hTPM_Cmd + 16 + ecx], edi ; Store exponent in TPM command buffer
    mov ecx, 16 + ecx + ebx     ; Calculate size of TPM command buffer

    ; Send TPM command to device
    mov eax, 0x80               ; TPM_IOPORT_WRITE command
    mov edx, TPM_DATA_REG       ; TPM data register port
    mov esi, hTPM_Cmd           ; Address of TPM command buffer
    out dx, al                  ; Send command to device
    rep outsb                   ; Send TPM command buffer to device

    ; Receive response from device
    mov eax, 0x80               ; TPM_IOPORT_READ command
    mov edx, TPM_STS_REG        ; TPM status register port
    xor ecx, ecx                ; Clear response buffer index
    xor eax, eax                ; Clear response buffer
.wait_for_response:
    in al, dx                   ; Wait for TPM status to become ready
    and al, TPM_STS_READY
    jz .wait_for_response
    in al, dx                   ; Check if TPM command succeeded
    test al, TPM_STS_ERROR_MASK
    jnz .tpm_error              ; Jump to error handling code on error
.read_response:
    in al, dx                   ; Read response data
    stosb                       ; Store data in response buffer
    loop .read_response         ; Repeat for remaining response bytes

    ; Check response buffer for TPM return code
    mov eax, hTPM_Response      ; Address of TPM response buffer
    cmp byte [eax], TPM_RC_SUCCESS
    jne .tpm_error              ; Jump to error handling code on error

    ; Copy encrypted message from response buffer to message buffer
    mov eax, hTPM_Response + 10 ; Address of encrypted message in response buffer
    mov esi, [hTPM_MsgSize]     ; Size of message to encrypt
    rep movsb                   ; Copy encrypted message to message buffer

    ; Restore registers and return
    pop edi
    pop ebx
    pop ebp
    ret

.tpm_error:
    ; Handle TPM error
    ; ...

    ; Restore registers and return
    pop

