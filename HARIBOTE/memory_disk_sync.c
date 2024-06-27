#include"bootpack.h"

typedef struct _disk_info{
	unsigned int offset_align_mask;//设备的对齐性 用于指定对齐大小 
	unsigned int size_align_mask;//设备的对齐性 用于指定对齐大小 
	void (*read)(void* p,unsigned long long offset,unsigned long long size);
	void (*write)(void* p,unsigned long long offset,unsigned long long size);
}DISK_INFO; 

typedef struct _mem_disk_sync{
	void* physical_address;
	unsigned long long size;
	DISK_INFO* disk_info;
	void* p;//数据结构用于描述同步信息
	/*
	0:未使用
	1:已同步 暂时无需进行操作
	2:已修改 需要同步至设备
	3:已失效 需要重新从设备同步
	*/
	unsigned int status;
}MEM_DISK_SYNC; 

typedef struct _mem_disk_sync_ctl{
	MEM_DISK_SYNC sync_item[4096];
}MEM_DISK_SYNC_CTL; 

MEM_DISK_SYNC_CTL* ctl;

void mem_disk_sync_init(struct PAGEMAN32* pageman){
	struct TASK* task=task_now();
	unsigned long long cr3=load_cr3();
	ctl = (struct SHTCTL *) memman_alloc_4k(task->memman, sizeof (MEM_DISK_SYNC_CTL));//mem_alloc!!!
	memman_link_page_64_m(pageman,cr3,ctl,0x07,(sizeof (MEM_DISK_SYNC_CTL)+0xfff)>>12,0);
}
void mem_disk_sync_init_item(MEM_DISK_SYNC* item,void* physical_address,unsigned long long size,DISK_INFO* disk_info,void* p){
	
	item->physical_address=physical_address;
	item->size=size;
	item->disk_info=disk_info;
	item->p=p;
	item->status=1;//因为不知道链接区域的内容有效性 暂时设置为已同步
	return;
}
void mem_disk_sync_to_disk(MEM_DISK_SYNC* item,unsigned long long offset,unsigned long long size,struct PAGEMAN32* pageman){
	unsigned start=offset&item->disk_info->offset_align_mask;
	unsigned end=(offset+size+~(item->disk_info->size_align_mask))&item->disk_info->size_align_mask;
	if(start!=offset){
		void* temp=memman_alloc_page_64_4k(pageman);
		item->disk_info->read(item->p,start,item->disk_info->size_align_mask);//读一个最小字段
		item->disk_info->read(item->p,start,item->disk_info->size_align_mask);//写回
	}
	//item->write(item->p,);
}