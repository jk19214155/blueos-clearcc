typedef struct __builtin_ms_va_list {
    char *gp_offset;
    char *fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __builtin_ms_va_list;

typedef struct _EFI_FILE_PROTOCOL{
  UINT64  Revision;
  EFI_STATUS  (*Open)(
                struct _EFI_FILE_PROTOCOL  *This,
                struct _EFI_FILE_PROTOCOL  **NewHandle,
                CHAR16                     *FileName,
                UINT64                     OpenMode,
                UINT64                     Attributes
                );
  EFI_STATUS  (*Close)(
                struct _EFI_FILE_PROTOCOL  *This
                );
  EFI_STATUS  (*Delete)(
                struct _EFI_FILE_PROTOCOL  *This
                );
  EFI_STATUS  (*Read)(
                struct _EFI_FILE_PROTOCOL  *This,
                UINTN                       *BufferSize,
                VOID                        *Buffer
                );
  EFI_STATUS  (*Write)(
                struct _EFI_FILE_PROTOCOL  *This,
                UINTN                       *BufferSize,
                VOID                        *Buffer
                );
  EFI_STATUS  (*GetPosition)(
                struct _EFI_FILE_PROTOCOL  *This,
                UINT64                     *Position
                );
  EFI_STATUS  (*SetPosition)(
                struct _EFI_FILE_PROTOCOL  *This,
                UINT64                     Position
                );
  EFI_STATUS  (*GetInfo)(
                struct _EFI_FILE_PROTOCOL  *This,
                EFI_GUID                   *InformationType,
                UINTN                      *BufferSize,
                VOID                       *Buffer
                );
  EFI_STATUS  (*SetInfo)(
                struct _EFI_FILE_PROTOCOL  *This,
                EFI_GUID                   *InformationType,
                UINTN                      BufferSize,
                VOID                       *Buffer
                );
  EFI_STATUS  (*Flush)(
                struct _EFI_FILE_PROTOCOL  *This
                );
} EFI_FILE_PROTOCOL;

typedef struct _EFI_FILE_INFO {
  UINT64    Size;
  UINT64    FileSize;
  UINT64    PhysicalSize;
  EFI_TIME  CreateTime;
  EFI_TIME  LastAccessTime;
  EFI_TIME  ModificationTime;
  UINT64    Attribute;
  CHAR16    FileName[1];
} EFI_FILE_INFO;

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL{
    unsigned long long Revision;
    unsigned long long (*OpenVolume)(
         struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
         EFI_FILE_PROTOCOL **Root);
}EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#include "bootpack.h"
#define EFI_FILE_MODE_READ 0x01
#define EFI_FILE_MODE_WRITE 0x02
#define EFI_FILE_MODE_CREATE 0x8000000000000000
#define ADDR_CR3 0x3f001000
#define EFI_SCAN_CODE_DOWN_ARROW 0x501
#define EFI_SCAN_CODE_UP_ARROW 0x500

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;//文件系统协议
EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;//图形协议
EFI_BOOT_SERVICES* BS;//系统服务
short text_buff[64];
UINT32 mode_list[64];
EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459, 0x11d2, \
                   {0x8e, 0x39, 0x00, 0xa0, \
                   0xc9, 0x69, 0x72, 0x3b}};
	EFI_GUID gEfiSimpleFileSystemProtocolGuid={0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};//简单文件系统协议
	EFI_GUID gEfiFileInfoGuid={0x09576e92,0x6d3f,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};//文件信息获取函数
	EFI_GUID gEfiPartTypeNtfsGuid={0x0FC63DAF, 0x8483, 0x4772, {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x62, 0xC5, 0x8F}};//ntfs文件系统类型
	EFI_GUID gEfiFileSystemInfoGuid={0x09576E93, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};//文件系统信息获取函数
	EFI_GUID gEfiGraphicsOutputProtocolGuid={0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};
	EFI_GUID gEfiEdidActiveProtocolGuid={0x1c0c34f6, 0xd380, 0x41fa, {0xa0, 0xf2, 0x5e, 0x4f, 0x94, 0x22, 0x2e, 0x9}};
	EFI_GUID gEfiEdidDiscoveredProtocolGuid={0x1c0c34f5, 0xd380, 0x41fa, {0xa0, 0xf2, 0x5e, 0x4f, 0x94, 0x22, 0x2e, 0x9}};
	EFI_GUID varGuid = {0xb105cc01, 0x2018, 0x0401, {0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xef, 0x00}};
	EFI_GUID gEfiLoadFileProtocolGuid={0x56ec3091,0x954c,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x1e,0xf9,0x71}};
	EFI_GUID gEfiTcgProtocolGuid=EFI_TCG2_PROTOCOL_GUID;
	EFI_GUID gEfiSimpleTextInputProtocolGuid = {
	  0x387477c1, 0x69c7, 0x11d2,
	  { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }
	};
	EFI_GUID dpttp_guid = {0x8b843e20, 0x8132, 0x4852,
                      {0x90, 0xcc, 0x55, 0x1a,
                       0x4e, 0x4a, 0x7f, 0x1c}};
	EFI_GUID dpp_guid = {0x09576e91, 0x6d3f, 0x11d2,
                            {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
	EFI_GUID gEfiLoadImageProtocol={0x52856efe,0x9f2d,0x4f2a,{0xaa,0x55,0x1f,0x67,0xf2,0x7f,0x87,0x41}};
	EFI_GUID gEfiDevicePathUtilitiesProtocolGuid={0x379be4e,0xd706,0x437d,{0xb0,0x37,0xed,0xb8,0x2f,0xb7,0x72,0xa4}};
	 EFI_GUID dpftp_guid = {0x5c99a21, 0xc70f, 0x4ad2,
                      {0x8a, 0x5f, 0x35, 0xdf,
                       0x33, 0x43, 0xf5, 0x1e}};
	//EFI_GUID gEfiBlockIoProtocolGuid={0xa77b2472,0xe282,0x4e9f,{0xa26a,0x79,0x64,0x19,0x3e,0xf0,0x03,0x67}};
	UINT32 buffer_size=256*1024;
	short unsigned int *blueos_index_path=(short unsigned int*)L"\\blueos";//校验文件路径
	short unsigned int *blueos_core_path=(short unsigned int*)L".\\system\\bootpack.sys";//核心文件路径
	short unsigned int *blueos_background=(short unsigned int*)L"\\system\\background";//背景文件路径
	short unsigned int *blueos_file_load_list=(short unsigned int*)L"\\system\\bootlist";//加载列表路径
EFI_STATUS efi_main(EFI_HANDLE gImageHandle,EFI_SYSTEM_TABLE *SystemTable)
{
	UINTN i;
	void* p_cr3=(void*)ADDR_CR3;
	EFI_BOOT_SERVICES* gBS=SystemTable->BootServices;//系统服务列表
	BS=gBS;
	EFI_FILE_PROTOCOL* file = NULL , *root;
	EFI_FILE_SYSTEM_INFO* fileInfo;//单个文件系统协议信息
	UINTN fs_num=0;//文件系统数量
	EFI_HANDLE* fsHandles = NULL;//所有文件系统协议句柄
	int boot_device=-1;//启动设备的标识
	EFI_STATUS status;
    //SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"Hello UEFI! blueos4bootx64\r\n");//输出hello world
	
	
	//切换显示模式
	// 获取Graphics Output协议
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
	status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&GraphicsOutput);
	if (EFI_ERROR(status)) {
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"Fail from GraphicsOutputProtocol:init\r\n");
		for(;;);
	}
	UINT32 mode_sel=1;//画面模式选择
	int mode_num=0;
	for(;;){//准备切换显示模式
		//清除屏幕，准备打印分辨率信息
		mode_num=0;
		SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"Please select the resolution you want to set from the list of supported options\r\n");
		// 遍历所有支持的分辨率
		UINT32 mode_set=-1;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo_set;
		for (UINT32 Mode = 0,index=0; Mode < GraphicsOutput->Mode->MaxMode; Mode++) {
			EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo;
			UINTN SizeOfInfo;

			// 获取该分辨率的信息
			status = GraphicsOutput->QueryMode(GraphicsOutput, Mode, &SizeOfInfo, &ModeInfo);
			if (EFI_ERROR(status)) {
				SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"Fail from GraphicsOutputProtocol:QueryMode\r\n");
				continue;
			}

			// 只处理32位和24位色模式
			if (ModeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor || ModeInfo->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
				// 输出该分辨率的信息
				index++;
				if(index==mode_sel){
					SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"[x]");
				}
				else{
					SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"[ ]");
				}
				sprintf(text_buff,(short unsigned int*)L"%d:Resolution: %dx%d, Format: %d\r\n", index, (int)ModeInfo->HorizontalResolution,(int) ModeInfo->VerticalResolution,(int) ModeInfo->PixelFormat);
				SystemTable->ConOut->OutputString(SystemTable->ConOut, text_buff);
				//SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"\r\n");
				mode_set=Mode;
				ModeInfo_set=ModeInfo;
				mode_list[++mode_num]=Mode;//获取mode编号，后面选择时会用到
				continue;
			}
		}
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"2018-2023 blueos.top All rights reserved.\r\n");
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"2018-2023 weiyufei@blueos.top All rights reserved.\r\n");
		for(;;){
			EFI_INPUT_KEY Key;
			status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);//获取按键
			if (Key.UnicodeChar == L's') {
				if(mode_sel<mode_num){//没有向下超界限
					mode_sel++;
					break;
				}
			}
			if (Key.UnicodeChar == L'w') {
				if(mode_sel>1){//没有向下超界限
					mode_sel--;
					break;
				}
			}
			if(Key.UnicodeChar == L' '){
				GraphicsOutput->SetMode(GraphicsOutput,mode_list[mode_sel]);
				//SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"mode change success\r\n");
				// 获取该分辨率的信息
				EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo;
				int SizeOfInfo;
				status = GraphicsOutput->QueryMode(GraphicsOutput, mode_list[mode_sel], &SizeOfInfo, &ModeInfo);
				//SystemTable->RuntimeServices->SetVariable("xsize",&varGuid,EFI_VARIABLE_RUNTIME_ACCESS|EFI_VARIABLE_BOOTSERVICE_ACCESS,4,&(ModeInfo->HorizontalResolution));
				//SystemTable->RuntimeServices->SetVariable("ysize",&varGuid,EFI_VARIABLE_RUNTIME_ACCESS|EFI_VARIABLE_BOOTSERVICE_ACCESS,4,&(ModeInfo->VerticalResolution));
				//SystemTableRuntimeServices->SetVariable("vram",&varGuid,EFI_VARIABLE_RUNTIME_ACCESS|EFI_VARIABLE_BOOTSERVICE_ACCESS,8,&((unsigned long long)0xe0000000));
				//*(char*)(0xff0)=0;
				//*(char*)(0xff0+1)=0;
				//*(char*)(0xff0+2)=32;
				//*(short*)(0xff0+4)=(short)ModeInfo->HorizontalResolution;//横向分辨率
				//*(short*)(0xff0+6)=(short)ModeInfo->VerticalResolution;//纵向分辨率
				//*(unsigned int*)(0xff0+8)=(unsigned int)0xe0000000;
				SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut,0,0);
				goto next;
			}
		}
	}
next:
	
	
	//制作内存分布图
	for(unsigned int i =0x800000;i<0x900000;i++){
		*(char*)i=1;
	}
	SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"map ok\r\n");
	EFI_MEMORY_DESCRIPTOR* MemoryMap;
	UINTN MapSize=0x200000;//1M内存
	int MapKey=0;
	int DescriptorSize=0;
	int DescriptorVersion=0;
	//获取MemoryMap信息
	//内存分布图警告：https://forum.osdev.org/viewtopic.php?f=8&t=38500
	EFI_STATUS Status = BS->GetMemoryMap(&MapSize, 0x900000, &MapKey, &DescriptorSize, &DescriptorVersion);
	if(EFI_ERROR(Status)){
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"GetMemoryMap err\r\n");
		for(;;);
	}
	else{
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"GetMemoryMap success\r\n");
	}
	sprintf(text_buff,(short unsigned int*)L"DescriptorSize size: %d\r\n",(int)DescriptorSize);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, text_buff);
	sprintf(text_buff,(short unsigned int*)L"MapSize size: %d\r\n",(int)MapSize);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, text_buff);
	sprintf(text_buff,(short unsigned int*)L"MapKey: %d\r\n",(int)MapKey);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, text_buff);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"getmap ok\r\n");
	/*开始填充内存分布图*/
	int n=0;
	for(i=0;i<MapSize;i+=DescriptorSize){
		unsigned int p=(int*)(0x900000+i);
		int m_type=*(int*)p;
		if(m_type==3||m_type==4||m_type==7){
			if(*(int*)(p+12)!=0){//4G以上的内存
				continue;
			}
			int m_start=*(int*)(p+8)>>12;//起始地址
			int m_num=*(int*)(p+0x18);//页面数量
			n+=m_num;
			for(unsigned int j=0x800000+m_start;j<0x800000+m_num;j++){
				*(char*)j=0;
			}
		}
	}
	sprintf(text_buff,(short unsigned int*)L"free pages : %d\r\n",(int)n);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, text_buff);
	//最开始的12M内存禁用
	for(int i=0;i<0xa00;i++){
		*(char*)(0x800000+i)=-1;
	}
	
	


	status = gBS->LocateHandleBuffer(//获取所有简单文件系统协议
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &fs_num,
                  &fsHandles);
	if(((int)status)==0){
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"fsHandles is ready\r\n");
	}
	else{
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"fsHandles is UNKNOW ERROR\r\n");
		//*(int*)(0x200000)=status;
		for(;;);
	}
	//尝试读取根目录下的root.blueos
	void* file_buffer;//准备的缓冲区buff
	//gBS->AllocatePool(EfiLoaderData,4096,&file_buffer);//分配一个页的大小存放文件数据
	// 遍历所有的协议句柄
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
	for (i = 0; i < fs_num; i++) {
		Status = BootServices->HandleProtocol(fsHandles[i], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs);
        if (EFI_ERROR(Status)) {
            continue; // 如果该分区不支持 Simple File System 协议，跳过
        }
		// 打开文件
		status = fs->OpenVolume(fs, &root);
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L" now find roo path\r\n");
		if (EFI_ERROR(status)) {
		 //*(int*)(0x200000)=status;
			SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"Failed to open volume\r\n");
			continue;
		}
		SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"now find the flag file\r\n");
		status = root->Open(root, &file,blueos_core_path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
		//*(int*)(0x200000)=status;
		if(EFI_ERROR(Status)){
			SystemTable->ConOut->OutputString(SystemTable->ConOut, (short unsigned int*)L"final not found\r\n");
			root->Close(root);
			continue;
		}
		// 文件找到后，加载该文件
        // 获取文件大小
        BufferSize = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 256;
        FileInfo = AllocatePool(BufferSize);
        Status = File->GetInfo(File, &gEfiFileInfoGuid, &BufferSize, FileInfo);
        if (EFI_ERROR(Status)) {
            Root->Close(Root);
            File->Close(File);
            continue;
        }

        // 文件大小
        UINTN FileSize = FileInfo->FileSize;

        // 分配内存加载文件
        VOID *Buffer = AllocatePool(FileSize);
        if (Buffer == NULL) {
            Root->Close(Root);
            File->Close(File);
            continue;
        }

        // 读取文件内容到缓冲区
        Status = File->Read(File, &FileSize, Buffer);
        if (EFI_ERROR(Status)) {
            FreePool(Buffer);
            Root->Close(Root);
            File->Close(File);
            continue;
        }

		/* 从内存加载镜像 */
		status = SystemTable->BootServices->LoadImage(TRUE, NULL,
							 NULL, Buffer, FileSize, &image);

		/* 新增(此处开始) */
		if(EFI_ERROR(status)){
			return -1;
		}
		/* 运行image */
		status = SystemTable->BootServices->StartImage(image, NULL, NULL);
		/* 新增(此处结束) */
		
        // 关闭文件和根目录
        FreePool(Buffer);
        Root->Close(Root);
        File->Close(File);

        return Status;
	}
	return EFI_SUCCESS;
}
int strcmp(short* str,short* dst){
	int i=0;
	for(i=0;str[i]==dst[i];i++){
		if(str[i]==0){
			break;
		}
	}
	if(str[i]==0&&dst[i]==0){
		return 0;
	}
	else{
		return -1;
	}
}
typedef char *  va_list;


 #ifdef  __cplusplus
 #define _ADDRESSOF(v)   ( &reinterpret_cast<const char &>(v) )
 #else
 #define _ADDRESSOF(v)   ( &(v) )
 #endif


 #define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )




 #define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
 #define _crt_va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
 #define _crt_va_end(ap)      ( ap = (va_list)0 )


 #define Test_va_start _crt_va_start /* windows stdarg.h */
 #define Test_va_arg _crt_va_arg
 #define Test_va_end _crt_va_end



 #define ZEROPAD    1       /* pad with zero */
 #define SIGN   2       /* unsigned/signed long */
 #define PLUS   4       /* show plus */
 #define SPACE  8       /* space if plus */
 #define LEFT   16      /* left justified */
 #define SPECIAL    32      /* 0x */
 #define LARGE  64      /* use 'ABCDEF' instead of 'abcdef' */
 
 #define NULL 0

 int _div(long* n,unsigned base)
 {
     int __res; 
         __res = ((unsigned long) *n) % (unsigned) base; 
         *n = ((unsigned long) *n) / (unsigned) base; 
         return __res;
 }

#define do_div(n,base) _div(&n,base)/*({ \
    int __res; \
    __res = ((unsigned long) n) % (unsigned) base; \
    n = ((unsigned long) n) / (unsigned) base; \
    __res; })*/




 static inline int isdigit(int ch)
 {
    return (ch >= '0') && (ch <= '9');
 }



 static int skip_atoi(const char **s)
 {
    int i = 0;

    while (isdigit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
 }



 static char *Test_number(char *str, long num, int base, int size, int precision,
    int type)
 {
    char c, sign, tmp[66];
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    int i;

    if (type & LARGE)
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return 0;
    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }
    i = 0;
    if (num == 0)
    {
        tmp[i++] = '0';
    }
    else
    {
        while (num != 0)
        {
            tmp[i++] = digits[do_div(num, base)];
        }
    }

    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type & (ZEROPAD + LEFT)))
        while (size-- > 0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (type & SPECIAL) {
        if (base == 8)
            *str++ = '0';
        else if (base == 16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }
    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;
    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
 }



unsigned int strnlen(char* s,unsigned int n){
	for(unsigned int i=0;;i++){
		if(s[i]==0){
			if(i<n){
				return i;
			}
			else{
				return n;
			}
		}
	}
}


 int Test_vsprintf(char *buf, const char *fmt, va_list args)
 {
    int len;
    unsigned long num;
    int i, base;
    char *str;
    const char *s;

    int flags;      /* flags to Test_number() */

    int field_width;    /* width of output field */
    int precision;      /* min. # of digits for integers; max
                   Test_number of chars for from string */
    int qualifier;      /* 'h', 'l', or 'L' for integer fields */

    for (str = buf; *fmt; ++fmt) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        /* process flags */
        flags = 0;
          repeat:
        ++fmt;      /* this also skips first '%' */
        switch (*fmt) {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        /* get field width */
        field_width = -1;
        if (isdigit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*') {
            ++fmt;
            /* it's the next argument */
            field_width = Test_va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        /* get the precision */
        precision = -1;
        if (*fmt == '.') {
            ++fmt;
            if (isdigit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*') {
                ++fmt;
                /* it's the next argument */
                precision = Test_va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        /* default base */
        base = 10;

        switch (*fmt) {
        case 'c':
            if (!(flags & LEFT))
                while (--field_width > 0)
                    *str++ = ' ';
            *str++ = (unsigned char)Test_va_arg(args, int);
            while (--field_width > 0)
                *str++ = ' ';
            continue;

        case 's':
            s = Test_va_arg(args, char *);
            len = strnlen(s, precision);

            if (!(flags & LEFT))
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; ++i)
                *str++ = *s++;
            while (len < field_width--)
                *str++ = ' ';
            continue;

        case 'p':
            if (field_width == -1) {
                field_width = 2 * sizeof(void *);
                flags |= ZEROPAD;
            }
            str = Test_number(str,
                     (unsigned long)Test_va_arg(args, void *), 16,
                     field_width, precision, flags);
            continue;

        case 'n':
            if (qualifier == 'l') {
                long *ip = Test_va_arg(args, long *);
                *ip = (str - buf);
            } else {
                int *ip = Test_va_arg(args, int *);
                *ip = (str - buf);
            }
            continue;

        case '%':
            *str++ = '%';
            continue;

            /* integer Test_number formats - set up the flags and "break" */
        case 'o':
            base = 8;
            break;

        case 'X':
            flags |= LARGE;
        case 'x':
            base = 16;
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            break;

        default:
            *str++ = '%';
            if (*fmt)
                *str++ = *fmt;
            else
                --fmt;
            continue;
        }
        if (qualifier == 'l')
            num = Test_va_arg(args, unsigned long);
        else if (qualifier == 'h') {
            num = (unsigned short)Test_va_arg(args, int);
            if (flags & SIGN)
                num = (short)num;
        } else if (flags & SIGN)
            num = Test_va_arg(args, int);
        else
            num = Test_va_arg(args, unsigned int);
        str = Test_number(str, num, base, field_width, precision, flags);
    }
    *str = '\0';
    return str - buf;
 }
 
  int sprintf(short *buf, short *fmt, ...)
{
    //记录fmt对应的地址
    va_list args;
    int val;
    //得到首个%对应的字符地址
    Test_va_start(args, fmt);
    val = Test_vsprintf(buf, fmt, args);
    Test_va_end(args);
    return val;
}




unsigned int strlen(char* s){
	for(unsigned int i=0;;i++){
		if(s[i]==0)
			return i;
	}
}
 
unsigned int alloc_page(unsigned int num){
	unsigned int i=0;
	char* p=0x800000;
	for(i=0;i<100000;i++){
		if(p[i]==0){
			int j;
			for(j=0;j<num;j++){
				if(p[i+j]!=0){
					break;
				}
			}
			if(j<num){//失败
				i+=j;//跳过失败部分
				continue;
			}
			else{
				for(int j=0;j<num;j++){
					p[i+j]=2;
				}
				return i*4096;
			}
		}
	}
	return -1;//失败
}