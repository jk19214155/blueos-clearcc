#include "bootpack.h"
EFI_SYSTEM_TABLE GlobalSystemTable;
EFI_BOOT_SERVICES GlobalBootServer;
EFI_RUNTIME_SERVICES GlobalRunTimeServer;
EFI_STATUS InitBootServices(EFI_BOOT_SERVICES** this){
	if(*this==NULL)
		*this=&GlobalBootServer;
}
EFI_STATUS InitSystemTable(EFI_SYSTEM_TABLE** SystemTable){
	if(*SystemTable==NULL)
		*SystemTable=&GlobalSystemTable;
	InitBootServices(&(*SystemTable)->BootServices);
	
	return EFI_SUCCESS;
}

EFI_STATUS
(EFIAPI BLONE_ALLOCATE_PAGES)(
  IN     EFI_ALLOCATE_TYPE            Type,
  IN     EFI_MEMORY_TYPE              MemoryType,
  IN     UINTN                        Pages,
  IN OUT EFI_PHYSICAL_ADDRESS         *Memory
  ){
	  
	  return EFI_SUCCESS;
  }