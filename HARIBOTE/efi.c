#include "bootpack.h"


typedef unsigned int EFI_STATUS;
#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 1

#pragma pack(push, 1)

#define memcpy asm_memcpy

// DOS Header
typedef struct {
    uint16_t e_magic;    // Magic number
    uint16_t e_cblp;     // Bytes on last page of file
    uint16_t e_cp;       // Pages in file
    uint16_t e_crlc;     // Relocations
    uint16_t e_cparhdr;  // Size of header in paragraphs
    uint16_t e_minalloc; // Minimum extra paragraphs needed
    uint16_t e_maxalloc; // Maximum extra paragraphs needed
    uint16_t e_ss;       // Initial (relative) SS value
    uint16_t e_sp;       // Initial SP value
    uint16_t e_csum;     // Checksum
    uint16_t e_ip;       // Initial IP value
    uint16_t e_cs;       // Initial (relative) CS value
    uint16_t e_lfarlc;   // File address of relocation table
    uint16_t e_ovno;     // Overlay number
    uint16_t e_res[4];   // Reserved words
    uint16_t e_oemid;    // OEM identifier (for e_oeminfo)
    uint16_t e_oeminfo;  // OEM information; e_oemid specific
    uint16_t e_res2[10]; // Reserved words
    uint32_t e_lfanew;   // File address of new exe header
} IMAGE_DOS_HEADER;

// PE Header
typedef struct {
    uint32_t Signature;
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

// Optional Header
typedef struct {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    uint64_t DataDirectory[16][2]; // Directory entries (Virtual Address, Size)
} IMAGE_OPTIONAL_HEADER64;

// Section Header
typedef struct {
    uint8_t  Name[8];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

#pragma pack(pop)

// INFO structure
typedef struct {
    void* entry_point;    // 程序的入口地址
    void* stack_pointer;  // 初始栈指针
    unsigned int bitness; // 程序的位数（如64）
} INFO;

// efi_load function
EFI_STATUS efi_load(void* code, void* data, INFO* info, void* img) {
    uint8_t* image = (uint8_t*)img;

    // DOS Header
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)image;
    if (dosHeader->e_magic != 0x5A4D) { // "MZ"
        return EFI_LOAD_ERROR;
    }

    // PE Header
    IMAGE_FILE_HEADER* fileHeader = (IMAGE_FILE_HEADER*)(image + dosHeader->e_lfanew + 4);
    if (*(uint32_t*)(image + dosHeader->e_lfanew) != 0x00004550) { // "PE\0\0"
        return EFI_LOAD_ERROR;
    }

    // Optional Header
    IMAGE_OPTIONAL_HEADER64* optionalHeader = (IMAGE_OPTIONAL_HEADER64*)((uint8_t*)fileHeader + sizeof(IMAGE_FILE_HEADER));
    if (optionalHeader->Magic != 0x20B) { // PE32+ magic number
        return EFI_LOAD_ERROR;
    }

    // Section Headers
    IMAGE_SECTION_HEADER* sectionHeader = (IMAGE_SECTION_HEADER*)((uint8_t*)optionalHeader + fileHeader->SizeOfOptionalHeader);

    // Load sections
    for (int i = 0; i < fileHeader->NumberOfSections; i++) {
        if (strncmp((char*)sectionHeader[i].Name, ".text", 8) == 0) {
            // Code section
            asm_memcpy(code, image + sectionHeader[i].PointerToRawData, sectionHeader[i].SizeOfRawData);
        } else if (strncmp((char*)sectionHeader[i].Name, ".data", 8) == 0) {
            // Data section
            asm_memcpy(data, image + sectionHeader[i].PointerToRawData, sectionHeader[i].SizeOfRawData);
        }
    }

    // Fill INFO structure
    info->entry_point = (void*)(optionalHeader->ImageBase + optionalHeader->AddressOfEntryPoint);
    info->bitness = 64;
    // Initial stack pointer can be set based on specific requirements, here we assume NULL
    info->stack_pointer = NULL;

    return EFI_SUCCESS;
}
