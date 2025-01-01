#include "bootpack.h"

// INFO structure
typedef struct {
	
    void* entry_point;    // 程序的入口地址
    void* stack_pointer;  // 初始栈指针
    unsigned int bitness; // 程序的位数（如64）
} INFO;

EFI_SYSTEM_TABLE blone_efi_system_table;
EFI_RUNTIME_SERVICES  blone_efi_runtime_services;
EFI_BOOT_SERVICES blone_efi_boot_service;

EFI_SYSTEM_TABLE* get_system_table(){
    blone_efi_system_table.ConIn=0;
    blone_efi_system_table.ConOut=0;
    blone_efi_system_table.StdErr=0;
    blone_efi_system_table.RuntimeServices=&blone_efi_runtime_services;
    blone_efi_system_table.BootServices=&blone_efi_boot_service;
    return &blone_efi_system_table;
}

EFI_STATUS efi_find(TYPEOF_PE* this,char* name,IMAGE_SECTION_HEADER** buff){
	int i;
	IMAGE_SECTION_HEADER* sectionHeader = this->sectionHeader;
    // Load sections
    for (i = 0; i < this->fileHeader->NumberOfSections; i++) {
        if (asm_sse_strcmp((char*)sectionHeader[i].Name, name, 8) == 0) {
            asm_memcpy(buff, (char*)this->buff + sectionHeader[i].PointerToRawData, sectionHeader[i].SizeOfRawData);
        } 
    }
	if(i < this->fileHeader->NumberOfSections){
		*buff=sectionHeader+i;
		return EFI_SUCCESS;
	}
	else{
		return EFI_LOAD_ERROR;
	}
}

// efi_load function
EFI_STATUS efi_load(TYPEOF_PE* this, void* buff, char* name,unsigned long long* size) {

    // Section Headers
    IMAGE_SECTION_HEADER* sectionHeader = this->sectionHeader;
	int i;
    // Load sections
    for (i = 0; i < this->fileHeader->NumberOfSections; i++) {
        if (asm_sse_strcmp((char*)sectionHeader[i].Name, name, 8) == 0) {
            // Code section
			if(*size<sectionHeader[i].SizeOfRawData){
				*size=sectionHeader[i].SizeOfRawData;
				return EFI_LOAD_ERROR;
			}
            asm_memcpy(buff, (char*)this->buff + sectionHeader[i].PointerToRawData, sectionHeader[i].SizeOfRawData);
			*size=sectionHeader[i].SizeOfRawData;
        } 
    }
	if(i < this->fileHeader->NumberOfSections){
		return EFI_SUCCESS;
	}
	else{
		return EFI_LOAD_ERROR;
	}
}


EFI_STATUS efi_init(TYPEOF_PE* this,char* image){
	this->buff=image;
	
	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)image;
    if (dosHeader->e_magic != 0x5A4D) { // "MZ"
        return EFI_LOAD_ERROR;
    }
	else{
		this->dosHeader=dosHeader;
	}
    // PE Header
    IMAGE_FILE_HEADER* fileHeader = (IMAGE_FILE_HEADER*)(image + dosHeader->e_lfanew + 4);
    if (*(uint32_t*)(image + dosHeader->e_lfanew) != 0x00004550) { // "PE\0\0"
        return EFI_LOAD_ERROR;
    }
	else{
		this->fileHeader=fileHeader;
	}
    // Optional Header
    IMAGE_OPTIONAL_HEADER64* optionalHeader = (IMAGE_OPTIONAL_HEADER64*)((uint8_t*)fileHeader + sizeof(IMAGE_FILE_HEADER));
    if (optionalHeader->Magic != 0x20B) { // PE32+ magic number
        return EFI_LOAD_ERROR;
    }
	else{
		this->optionalHeader=optionalHeader;
	}
    // Section Headers
    IMAGE_SECTION_HEADER* sectionHeader = (IMAGE_SECTION_HEADER*)((uint8_t*)optionalHeader + fileHeader->SizeOfOptionalHeader);
	this->sectionHeader=sectionHeader;
	this->read=efi_load;
    return EFI_SUCCESS;
}