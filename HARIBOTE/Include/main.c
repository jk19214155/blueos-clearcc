#include "Uefi.h"

typedef struct EFI_FILE_PROTOCOL {//文件协议
    unsigned long long _buf;
    unsigned long long (*Open)(struct EFI_FILE_PROTOCOL *This,
                   struct EFI_FILE_PROTOCOL **NewHandle,
                   unsigned short *FileName,
                   unsigned long long OpenMode,
                   unsigned long long Attributes);
    unsigned long long (*Close)(struct EFI_FILE_PROTOCOL *This);
    unsigned long long _buf2;
    unsigned long long (*Read)(struct EFI_FILE_PROTOCOL *This,
                   unsigned long long *BufferSize,
                   void *Buffer);
    unsigned long long (*Write)(struct EFI_FILE_PROTOCOL *This,
                    unsigned long long *BufferSize,
                    void *Buffer);
    unsigned long long _buf3[4];
    unsigned long long (*Flush)(struct EFI_FILE_PROTOCOL *This);
}EFI_FILE_PROTOCOL;


typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL{
    unsigned long long Revision;
    unsigned long long (*OpenVolume)(
         struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
         EFI_FILE_PROTOCOL **Root);
}EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;//文件系统协议

void efi_main(void *ImageHandle __attribute__ ((unused)),EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_FILE_PROTOCOL *root;//文件协议
    //SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"Hello UEFI!\n");//输出hello world
	EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459, 0x11d2, \
                   {0x8e, 0x39, 0x00, 0xa0, \
                   0xc9, 0x69, 0x72, 0x3b}};
	SystemTable->BootServices->LocateProtocol(&sfsp_guid, NULL, (void **)&SFSP);//获取文件系统协议
	SFSP->OpenVolume(SFSP, &root);//打开根目录,获取文件协议
    while (1);
}