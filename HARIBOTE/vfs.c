#include "bootpack.h"

typedef struct _VFS{
    EFI_STATUS (*mount)(struct _VFS* this,unsigned long long device_id,unsigned long long part_base_lba,unsigned long long part_end_lba,unsigned long long part_id);
    EFI_STATUS (*umount)(struct _VFS* this);
    EFI_STATUS (*open)(struct _VFS* this,unsigned long long index);
    EFI_STATUS (*close)(struct _VFS* this);
    EFI_STATUS (*read)(struct _VFS* this,void* buff,unsigned long long seek,unsigned long long size);
    EFI_STATUS (*write)(struct _VFS* this,void* buff,unsigned long long seek,unsigned long long size);
}VFS;

