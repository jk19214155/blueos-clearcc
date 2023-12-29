#include "bootpack.h"



unsigned int _read_file(char* buff,unsigned int* size,unsigned int fat32_addr,unsigned part_base_lba,FAT32_HEADER* mbr,unsigned int start_lba_low,unsigned int start_lba_high);
typedef struct{
	int lba;//当前访问的lba信息
	int status;
}dmg_info;
int lock;
int disk_running=0;
struct TASK * task_disk_p;
void dmg_init(){
	lock=0;
	return;
}
StorageDeviceOperations dmg_StorageDeviceOperations;
struct TASK* start_task_disk(){//磁盘访问守护程序
	if(disk_running!=0){
		return task_disk_p;
	}
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct PAGEMAN32 *pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	struct TASK *task = task_alloc(),*task_now0=task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);//图层控制器
	int i;
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);//内存分配!!!
	memmam_link_page_32_m(pageman,0x268000,cons_fifo,7, 1,0);//
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);//内存分配!!!
	pageman_link_page_32_m(pageman,task->cons_stack,7,0x10,0);//
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = ((int) &task_disk)+((int)get_this());
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	task->task_sheet_max=8;//最大图层数量
	task->tss.cr3=0x268000;
	
	task->memman=memman;
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	
	disk_running=1;//运行中
	task_disk_p=task;
	return task;
}
dmg_info* info;
int dmg_info_num;
void task_disk(){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	struct MEMMAN *memman = task_now()->memman;
	EFI_GUID part_type_uid=gUidPartTypeBaseData;//数据段
	EFI_GUID part_own_uid={0x7a57c1e4,0xea59,0x49c0,{0xad,0x61,0x97,0x6f,0x85,0x30,0x85,0xa3}};//启动盘唯一GUID
	int i;
	void* fat32_addr;//启动区的fat地址
	unsigned int part_base_lba=0;//启动区开始lba
	unsigned int part_endl_lba=0;//启动区结束lba
	dmg_read(0x00100000,0,1440*2,0);//读入启动扇区
	//找到启动磁盘
	unsigned int disk_data_base=0x100000;//数据加载的基地址
	unsigned int disk_part_base=disk_data_base+0x400;
	for(i=0;i<16;i++){
		GPT_ITEM* p=((GPT_ITEM*)disk_part_base)+i;
		if(p->guid.g1==part_own_uid.g1){
			if(p->guid.g2==part_own_uid.g2){
				if(p->guid.g3==part_own_uid.g3){
					int j;
					for(j=0;j<8;j++){
						if(p->guid.g4[j]!=part_own_uid.g4[j]){
							break;
						}
					}
					if(j==8){//通过了全部测验
						part_base_lba=p->start_lba_low;
						part_endl_lba=p->end_lba_low;
						break;
					}
				}
			}
		}
	}
	if(i<16){//找到了设备，解析fat
		FAT32_HEADER* mbr=memman_alloc_4k(memman,4096);//分配一个内存
		pageman_link_page_32_m(pageman,mbr,7,1,0);
		dmg_read(mbr,part_base_lba,1,0);//先读取1个扇区的内存进行分析
		unsigned int BPB_TotSec32=mbr->BPB_TotSec32;//fat大小
		*(unsigned int*)(0x0026f024)=mbr;
		unsigned int BPB_ResvdSecCnt=mbr->BPB_ResvdSecCnt;//fat前保留的扇区数
		//获取FAT大小
		unsigned int BPB_FATSz32=mbr->BPB_FATSz32;
		fat32_addr=memman_alloc_4k(memman,BPB_FATSz32*512);//分配内存
		pageman_link_page_32_m(pageman,fat32_addr,7,(BPB_FATSz32+3)>>2,0);
		dmg_read(fat32_addr,part_base_lba+mbr->BPB_ResvdSecCnt,BPB_FATSz32,0);
		*(unsigned int*)(0x0026f028)=fat32_addr;
		//接下来获取跟目录信息
		unsigned int root_lba=mbr->BPB_Root;
		void* root_dir_addr=memman_alloc_4k(memman,8192);//4k页就够了
		pageman_link_page_32_m(pageman,root_dir_addr,7,2,0);
		unsigned int size=8192;
		_read_file(root_dir_addr+16,&size,fat32_addr, part_base_lba,mbr, mbr->BPB_Root,0);
		/*接下来获取文件夹里的文件数量*/
		 //int num=_get_file_number(root_dir_addr,8192);
		 //*(int*)(0x0026f038)=num;
		 *(unsigned int*)(0x0026f030)=part_base_lba;
		 *(unsigned int*)(0x0026f034)=part_endl_lba;
		 /*接下来读取背景文件ground.jpg*/
		 //struct SHEET **sht_back=*(unsigned int*)0x0026f03c;
		 //struct FILEINFO * finfo=file_search("ground.jpg",0, 20);
		 //if(finfo!=0){
			//unsigned int size=finfo->size;
			//file_loadfile2(finfo->clustno,&size, int *fat);
		 //}
	}
	//注册设备
	/*for(i=0;i<dmg_info_num;i++){
		info[i].status=0;
	}*/
	/*运行AHCI初始化函数*/
	ahci_init();
	for(;;){
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			int i = fifo32_get(&task->fifo);
			io_sti();
		}
	}
}
/*
buff:保存的地址
size：读取的大小
fat32_addr： fat表的地址
part_base_lba：本分区前的保留扇区数量
mbr：文件系统的头扇区
start_lba_low：文件的第一个簇的位置
*/
unsigned int _read_file(char* buff,unsigned int* size,unsigned int fat32_addr,unsigned part_base_lba,FAT32_HEADER* mbr,unsigned int start_lba_low,unsigned int start_lba_high){
	unsigned int i;
	for(i=0;;i++){
		//如果多读一个簇就会超过size设定的大小
		if((i+1)*(mbr->BPB_SecPerClus)*512>size){
			size=i*(mbr->BPB_SecPerClus)*512;//设定已读内容数量
			return 0;
		}
		//扇区位置:逻辑分区基地址+保留分区+FAT所占的分区*FAT数量+(簇号-2)*簇大小
		dmg_read(buff+i*512*(mbr->BPB_SecPerClus),part_base_lba+mbr->BPB_ResvdSecCnt+(mbr->BPB_FATSz32)*(mbr->BPB_NumFATs)+(start_lba_low-2)*(mbr->BPB_SecPerClus),mbr->BPB_SecPerClus,0);
		if(*((unsigned int*)fat32_addr+start_lba_low)==0x0fffffff){
			break;
		}
		start_lba_low=((int*)fat32_addr)[start_lba_low];
	}
	return 0;
}
/*
buff:目录文件的内容指针
*/
unsigned int _get_file_number(unsigned char* buff,unsigned int size){
	unsigned int i,n;
	n=0;
	for(i=0;i<(size/32);i++){
		if(buff[32*i+0x11]!=0x0f&&buff[32*i+0x11]!=0x00){
			if(buff[32*i]!=0xe5){
				n++;
			}
		}
	}
	return n;
}
int dmg_read2(char* buff,int lba28,unsigned char block_number,int divice){
	int i,j;
	if(lock&1){
		return -1;
	}
	else{
		lock|=1;
	}
	io_out8(0x1f2,block_number);//写入要读取的数量
	io_out8(0x1f3,(lba28>>0)&0xff);//7-0
	io_out8(0x1f4,(lba28>>8)&0xff);//15-8
	io_out8(0x1f5,(lba28>>16)&0xff);//23-16
	io_out8(0x1f6,(lba28>>24)&0xff|0xe0);//27-24
	io_out8(0x1f7,0x20);//读数据
	for(i=0;i<block_number;i++){
		for(;;){//循环等待硬盘准备就绪,注意每个扇区读取完成后都需要等待就位
			unsigned char data=io_in8(0x1f7);
			*(unsigned char*)0x0026f02c=data;
			data&=0x88;
			if(data==0x08){
				break;
			}
		}
		*(unsigned char*)0x0026f02c=0;
		for(j=0;j<256;j++){
			int data=io_in16(0x1f0);
			buff[i*512+j*2]=data&0xff;
			buff[i*512+j*2+1]=(data>>8)&0xff;
		}
	}
	lock&=0xfffffffe;
	return 0;
}
int dmg_read(char* buff,int lba28,int block_number,int divice){
	for(;;){
		if(block_number<=255){
			dmg_read2(buff,lba28,block_number,divice);
			break;
		}
		else{
			dmg_read2(buff,lba28,255,divice);
			buff=buff+255*512;
			lba28+=255;
			block_number-=255;
		}
	}
	return 0;
}
int dmg_write(void* buff,int lba28,int block_number,int divice){
	int i;
	if(lock&1){
		return -1;
	}
	else{
		lock|=1;
	}
	io_out8(0x1f2,(char)block_number);//写入要读取的数量
	io_out8(0x1f3,(lba28>>0)&0xff);//7-0
	io_out8(0x1f4,(lba28>>8)&0xff);//15-8
	io_out8(0x1f5,(lba28>>16)&0xff);//23-16
	io_out8(0x1f6,(lba28>>24)&0xff|0xe0);//27-24
	io_out8(0x1f7,0x30);//写数据
	for(;;){//循环等待硬盘准备就绪
		char data=io_in8(0x1f7);
		data&=0x88;
		if(data==0x08){
			break;
		}
	}
	for(i=0;i<block_number*256;i++){
		int data=((short*)buff)[i];
		io_out16(0x1f0,data);
	}
	lock&=0xfffffffe;
	return 0;
}
unsigned int _dmg_open(StorageDevice* dev,int* user_index,char* name,int mode){
	return 0;
}
unsigned int _dmg_read(StorageDevice* dev,int user_index, void* buf, unsigned int* count){
	return 0;
}
unsigned int _dmg_write(StorageDevice* dev,int user_index, void* buf, unsigned int* count){
	dmg_write(buf,1,1,0);
	return 0;
}
unsigned int _dmg_close(StorageDevice* dev){
	return 0;
}
unsigned int _dmg_seek(StorageDevice* dev, int user_index,unsigned int* offset){
	int _offset=*offset;
	_offset&=0xfffff000;//4k对齐
	return 0;
}