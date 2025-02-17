#include "bootpack.h"
#include "sqlite3.h"
int *fat32xClose(sqlite3_file*){
	
}
int fat32xRead(sqlite3_file* file, void* buff, int iAmt, sqlite3_int64 iOfst){
	FILE_OF_FAT32* file_fat32=NULL;
	file_fat32->fread(file_fat32,buff,iOfst,iAmt);
}
int fat32xWrite(sqlite3_file* file, const void* buff, int iAmt, sqlite3_int64 iOfst){
	FILE_OF_FAT32* file_fat32=NULL;
	file_fat32->fwrite(file_fat32,buff,iOfst,iAmt);
}
int fat32xTruncate(sqlite3_file* file, sqlite3_int64 size){
	FILE_OF_FAT32* file_fat32=NULL;
	file_fat32->fwrite(file_fat32,NULL,0,0);
}
int fat32xSync(sqlite3_file*, int flags){
	return;
}

int fat32xFileSize(sqlite3_file*, sqlite3_int64 *pSize){
	
}
int fat32xLock(sqlite3_file*, int){
	
}
int fat32xUnlock(sqlite3_file*, int){
	
}
int fat32xCheckReservedLock(sqlite3_file*, int *pResOut){
	
}
int fat32xFileControl(sqlite3_file*, int op, void *pArg){
	
}
int fat32xSectorSize(sqlite3_file* file){
	
}
int fat32xDeviceCharacteristics(sqlite3_file* file){
	
}