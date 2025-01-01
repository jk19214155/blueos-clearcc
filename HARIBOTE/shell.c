#include "bootpack.h"
EFI_STATUS eif_main(EFI_HANDLE gImageHandle,EFI_SYSTEM_TABLE* SystemTable){
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello UEFI!\n");
	return EFI_SUCCESS;
}