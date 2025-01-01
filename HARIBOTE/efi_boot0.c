#include <Uefi.h>

// EFI启动入口
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *File;
    EFI_FILE_INFO *FileInfo;
    UINTN BufferSize;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    UINTN Index;

    // 初始化EFI库
    EFI_BOOT_SERVICES *BootServices = SystemTable->BootServices;
    EFI_FILE_PROTOCOL *BootFile = SystemTable->BootServices->HandleProtocol;

    // 获取系统中的所有句柄
    Status = BootServices->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Unable to locate handle buffer\n");
        return Status;
    }

    // 遍历所有句柄，检查每个分区
    for (Index = 0; Index < HandleCount; Index++) {
        // 获取当前分区的 Simple File System 协议
        Status = BootServices->HandleProtocol(HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&SimpleFileSystem);
        if (EFI_ERROR(Status)) {
            continue; // 如果该分区不支持 Simple File System 协议，跳过
        }

        // 通过Simple File System协议打开根目录
        Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
        if (EFI_ERROR(Status)) {
            continue; // 如果打开根目录失败，跳过
        }

        // 尝试打开文件 ".\\system\\bootpack.sys"
        Status = Root->Open(Root, &File, L"\\system\\bootpack.sys", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
        if (EFI_ERROR(Status)) {
            // 如果文件不存在或无法打开，跳过
            Root->Close(Root);
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

        // 文件读取成功，执行EFI镜像
        EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
        Status = BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);
        if (EFI_ERROR(Status)) {
            FreePool(Buffer);
            Root->Close(Root);
            File->Close(File);
            return Status;
        }

        // 创建一个新的映像并执行
        EFI_MEMORY_TYPE MemoryType = EfiLoaderData;
        EFI_PHYSICAL_ADDRESS ImageBase = (EFI_PHYSICAL_ADDRESS)Buffer;

        Status = BootServices->ExitBootServices(ImageHandle, LoadedImage->ExitDataSize);
        if (EFI_ERROR(Status)) {
            FreePool(Buffer);
            Root->Close(Root);
            File->Close(File);
            return Status;
        }

        // 执行EFI镜像
        Status = ((EFI_STATUS (*)(EFI_HANDLE, EFI_SYSTEM_TABLE *))ImageBase)(ImageHandle, SystemTable);

        // 关闭文件和根目录
        FreePool(Buffer);
        Root->Close(Root);
        File->Close(File);

        return Status;
    }

    // 如果没有找到文件
    Print(L"bootpack.sys not found.\n");
    return EFI_NOT_FOUND;
}
