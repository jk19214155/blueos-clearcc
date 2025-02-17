#include "bootpack.h"
#include "sqlite3.h"



void ___chkstk_ms(void){
	return;
}

void func_idle(){
	printf("call func_idle\r\n");
	return;
}

#include "sqlite3.h"

// 文件描述符结构体，用于管理文件信息
typedef struct {
    sqlite3_file base;          // SQLite 的文件基础结构
    FILE* fat32File;   // 指向实际的文件结构
	char* filename;
	unsigned long long flags;
} Fat32File;

// 自定义的 VFS 名称
#define FAT32_VFS_NAME "fat32_vfs"

//文件删除
int fat32Delete(sqlite3_vfs* fs, const char *zName, int syncDir){
	FILE* f=0;
	FILE_OF_FAT32* t=task_now()->file;
	f=fat32_init(t,t->cache ,0,t->part_base_lba);
	printf("call fat32Delete\r\n");
	printf("filename: %s\n",zName);
	//创建文件
	unsigned long long index=0;
	unsigned int status=fat32_open_file_from_cache(f,zName,&index);
	if(status!=0){
		return SQLITE_OK;//找不到文件返回OK
	}
	else{//把文件大小截断到0
		EFI_STATUS status = f->fwrite(f, NULL, 0, 0);
		fat32_delete_file_from_cache(f,index);
	}
	return SQLITE_OK;
}

//文件检测
int fat32Access(sqlite3_vfs* fs, const char *zName, int flags, int *pResOut){
	printf("call fat32Access file name:%s\r\n",zName);
	FILE* f=0;
	FILE_OF_FAT32* t=task_now()->file;
	f=fat32_init(t,t->cache ,0,t->part_base_lba);
	//创建文件
	unsigned long long index=0;
	unsigned int status=fat32_open_file_from_cache(f,zName,&index);
	if(flags==SQLITE_ACCESS_EXISTS){//文件是否存在
		if(status==0){
			printf("fat32Access SQLITE_ACCESS_EXISTS:ok\r\n");
			*pResOut=1;
			return SQLITE_OK;
		}
		else{
			*pResOut=0;
			printf("fat32Access SQLITE_ACCESS_EXISTS:no\r\n");
			return SQLITE_OK;
		}
	}
	else if(flags==SQLITE_ACCESS_READWRITE){
		printf("fat32Access SQLITE_ACCESS_READWRITE:ok\r\n");
		*pResOut=1;
		return SQLITE_OK;
	}
	else{
		printf("fat32Access other:ok\r\n");
		*pResOut=1;//允许访问
		return SQLITE_OK;
	}
	return SQLITE_OK;
}


// 文件关闭接口
static int fat32Close(sqlite3_file* file) {
	Fat32File* fat32File = (Fat32File*)file;
	printf("call fat32Close: %s\r\n",fat32File->filename);
	if( (fat32File->flags) & SQLITE_OPEN_DELETEONCLOSE){
		printf("call fat32Close->fat32Delete\r\n");
		fat32Delete(fat32File,fat32File->filename,NULL);
	}
    return SQLITE_OK; // 忽略关闭操作
}

// 文件读取接口
static int fat32Read(sqlite3_file* file, void* buffer, int size, sqlite3_int64 offset) {
	printf("call fat32Read\r\n");
    Fat32File* fat32File = (Fat32File*)file;
    EFI_STATUS status = fat32File->fat32File->fread(fat32File->fat32File, buffer, offset, size);
    return SQLITE_OK;
	return (status == EFI_SUCCESS) ? SQLITE_OK : SQLITE_IOERR_READ;
}

// 文件写入接口
static int fat32Write(sqlite3_file* file, const void* buffer, int size, sqlite3_int64 offset) {
	printf("call fat32Write\r\n");
    Fat32File* fat32File = (Fat32File*)file;
    EFI_STATUS status = fat32File->fat32File->fwrite(fat32File->fat32File, (void*)buffer, offset, size);
	return SQLITE_OK;
    return (status == EFI_SUCCESS) ? SQLITE_OK : SQLITE_IOERR_WRITE;
}

int fat32Truncate(sqlite3_file* file, sqlite3_int64 size){
	printf("call fat32Truncate\r\n");
	Fat32File* fat32File = (Fat32File*)file;
	FILE_OF_FAT32* file2=fat32File->fat32File;
	EFI_STATUS status = file2->top.fwrite(file2, NULL, size, 0);
	return SQLITE_OK;
    return (status == EFI_SUCCESS) ? SQLITE_OK : SQLITE_IOERR_WRITE;
}

// 获取文件大小
static int fat32FileSize(sqlite3_file* file, sqlite3_int64* pSize) {
	printf("call fat32FileSize\r\n");
    Fat32File* fat32File = (Fat32File*)file;
	FILE_OF_FAT32* file2=fat32File->fat32File;
    *pSize = (sqlite3_int64)(file2->size);
    return SQLITE_OK;

}

int fat32CheckReservedLock(sqlite3_file*, int *pResOut){
	printf("call fat32CheckReservedLock\r\n");
	*pResOut=0;//没有锁
	return SQLITE_OK;
}

// 文件控制接口（忽略所有请求）
static int fat32FileControl(sqlite3_file* file, int op, void* pArg) {
	printf("call fat32FileControl\r\n");
    return SQLITE_OK;
}

// 文件锁接口（无锁支持，直接返回成功）
static int fat32Lock(sqlite3_file* file, int lockType) {
	printf("call fat32Lock\r\n");
    return SQLITE_OK;
}

static int fat32Unlock(sqlite3_file* file, int lockType) {
	printf("call fat32Unlock\r\n");
    return SQLITE_OK;
}

// 文件检查点接口（忽略同步操作）
static int fat32Sync(sqlite3_file* file, int flags) {
	printf("call fat32Sync\r\n");
    return SQLITE_OK;
}

int fat32SectorSize(sqlite3_file *pFile) {
	printf("call fat32SectorSize\r\n");
	FILE_OF_FAT32* file=((Fat32File*)pFile)->fat32File;
	int size=file->mbr->BPB_SecPerClus*512;
    return size;  // 4K 扇区大小（适配 FAT32）
}

int fat32DeviceCharacteristics(sqlite3_file *pFile) {
	printf("call fat32DeviceCharacteristics\r\n");
    return SQLITE_IOCAP_ATOMIC4K | SQLITE_IOCAP_SAFE_APPEND;
}


// 自定义 VFS 的方法表
static sqlite3_io_methods fat32IoMethods = {
    1,                          // 版本号
    fat32Close,                 // xClose
    fat32Read,                  // xRead
    fat32Write,                 // xWrite
	fat32Truncate,				//xTruncate
    fat32Sync,                  // xSync
	fat32FileSize,              // xFileSize
    fat32Lock,                  // xLock
    fat32Unlock,                // xUnlock
    fat32CheckReservedLock,		//int (*xCheckReservedLock)(sqlite3_file*, int *pResOut);
	fat32FileControl,           // xFileControl
    fat32SectorSize,                       // xSectorSize
    fat32DeviceCharacteristics                        // xDeviceCharacteristics
};



// 自定义的 xOpen 接口
static int fat32Open(sqlite3_vfs* vfs, const char* zName, sqlite3_file* file, int flags, int* pOutFlags) {
	printf("call fat32Open: %s\r\n",zName);
    Fat32File* fat32File = (Fat32File*)file;
	char* temp;
	FILE* f=0;
	FILE_OF_FAT32* t=task_now()->file;
	f=fat32_init(t,t->cache ,0,t->part_base_lba);
	//创建文件
	unsigned long long index=0;
	unsigned int status=fat32_open_file_from_cache(f,zName,&index);
	if(status!=0){//找不到文件
		if(flags & SQLITE_OPEN_CREATE ){
			fat32_create_file_from_cache(f,zName);
			fat32File->fat32File = f;
			fat32File->filename=zName;
			fat32File->flags=(unsigned int)flags;
			file->pMethods = &fat32IoMethods;
			if (pOutFlags) {
				*pOutFlags = flags;
			}
			return SQLITE_OK;
		}
		else{
			file->pMethods=NULL;
			return SQLITE_CANTOPEN;
		}
	}
    else{//找到了文件
		if((flags&SQLITE_OPEN_CREATE)&&(flags&SQLITE_OPEN_EXCLUSIVE) ){
			file->pMethods=NULL;
			return SQLITE_CANTOPEN;
		}
		else{
			fat32File->fat32File = f;
			fat32File->filename=zName;
			fat32File->flags=(unsigned int)flags;
			file->pMethods = &fat32IoMethods;
			if (pOutFlags) {
				*pOutFlags = flags;
			}
			return SQLITE_OK;
		}
	}
}

static int myFullPathname(sqlite3_vfs *pVfs, const char *zName, int nOut, char *zOut) {
	printf("call myFullPathname\r\n");
    if (!zName || !zOut || nOut <= 0) {
        return SQLITE_ERROR;
    }

    // 复制文件名，确保不会超出 nOut
	for(int i=0;;i++){
		zOut[i]=zName[i];
		if(zName[i]==0){
			break;
		}
		if(i>=nOut-1){
			break;
		}
	}
    zOut[nOut - 1] = '\0'; // 保证字符串结尾

    return SQLITE_OK;
}


void myLogCallback(void *pArg, int iErrCode, const char *zMsg) {
    printf("SQLite Log [%d]: %s\n", iErrCode, zMsg);
}

int fat32Randomness(sqlite3_vfs* fs, int nByte, char *zOut){
	printf("call fat32Randomness\r\n");
	int* s=zOut;
	for(;;){
		if(nByte<=0)
			break;
		unsigned int rand=rdrand();
		if(nByte>=4){
			s=rand;
			s++;
			zOut+=4;
			nByte-=4;
		}
		else{
			char* s=&rand;
			for(int i=0;i<nByte;i++){
				zOut[i]=s[i];
			}
			break;
		}
	}
	return SQLITE_OK;
}

int fat32GetLastError(sqlite3_vfs* fs, int n, char *s){
	printf("call fat32GetLastError\r\n");
	printf("err msg: unknow");
	char* q="fs:unknow err\n";
	for(int i=0;;i++){
		if(q[i]==0){
			s[i]=q[i];
			break;
		}
		else{
			s[i]=q[i];
		}
	}
	return SQLITE_OK;
}

// 自定义的 VFS 结构体
struct sqlite3_vfs fat32Vfs = {
    1,                          // 版本号
    sizeof(Fat32File),          // 每个文件对象的大小
    1024,                       // 最大路径长度
    NULL,                       // pNext
    "fat32_vfs",             // zName
    0xffffffffffff19b8,                       // pAppData
    fat32Open,                  // xOpen
    fat32Delete,                       // xDelete
    fat32Access,                       // xAccess
    myFullPathname,             // xFullPathname
    0xffffffffffff49b8,                       // xDlOpen
    0xffffffffffff59b8,                       // xDlError
    0xffffffffffff69b8,                       // xDlSym
    0xffffffffffff79b8,                       // xDlClose
    fat32Randomness,                       // xRandomness
    0xffffffffffff99b8,                       // xSleep
    0xffffffffffffa9b8,                       // xCurrentTime
    fat32GetLastError,                       // xGetLastError
    0xffffffffffffc9b8,                       // xCurrentTimeInt64
    0xffffffffffffd9b8,                       // xSetSystemCall
    0xffffffffffffe9b8,                       // xGetSystemCall
    0xfffffffffffff9b8                        // xNextSystemCall
};

// sqlite3_os_init 实现
int sqlite3_os_init(void) {
	printf("call sqlite3_os_init\r\n");
	//sqlite3_config(SQLITE_CONFIG_LOG, myLogCallback, NULL);
	int rc=sqlite3_vfs_register(&fat32Vfs, 1);
    if (rc != SQLITE_OK) {
		printf("Cannot register vfs");
		return rc;
	}
	return rc;
}

// sqlite3_os_end 实现
int sqlite3_os_end(void) {
	printf("call sqlite3_os_end\r\n");
    return SQLITE_OK;
}
