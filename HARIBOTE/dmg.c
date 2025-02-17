#include "bootpack.h"




unsigned int _read_file(char* buff,unsigned int* size,unsigned int fat32_addr,unsigned part_base_lba,FAT32_HEADER* mbr,unsigned int start_lba_low,unsigned int start_lba_high,unsigned int device_id);
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
char ahci_buff[513];
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
	task->tss.rsp = task->cons_stack + 64 * 1024 - 12;
	task->tss.rip = ((int) &task_disk)+((int)get_this());
	task->task_sheet_max=8;//最大图层数量
	task->tss.cr3=0x268000;
	
	task->memman=memman;
	task_start(task);
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	
	disk_running=1;//运行中
	task_disk_p=task;
	return task;
}
dmg_info* info;
int dmg_info_num;

extern AHCI_TABLE* ahci_table_addr;
extern char* text_buff;

void dmg_gpt_get_item(unsigned long long n){
	
}

unsigned int dmg_cmp_guid(char* stra,char* strb){
	int i=sizeof(EFI_GUID);
	if (asm_sse_strcmp(stra,strb,i) == 0){
		return 0;
	}
	else{
		return 1;
	}
}

void task_disk(){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	struct MEMMAN *memman = task_now()->memman;
	EFI_GUID part_type_uid=gUidPartTypeBaseData;//数据段
	EFI_GUID part_own_uid={0x7a57c1e4,0xea59,0x49c0,{0xad,0x61,0x97,0x6f,0x85,0x30,0x85,0xa3}};//启动盘唯一GUID
	EFI_GUID part_own_uid2={0x346065d9,0x295d,0x4107,{0x86,0x25,0x98,0x6d,0xb5,0xef,0xa6,0x40}};
	short command[128];
	int command_point=0;
	int i,j,k,m;
	void* fat32_addr;//启动区的fat地址
	unsigned int part_base_lba=0;//启动区开始lba
	unsigned int part_endl_lba=0;//启动区结束lba
	i=ahci_table_addr->number;
	unsigned int ahci_id,ahci_port;
	unsigned device_id=0;
	AHCI_SATA_FIS* fis=0;
	unsigned long long read_size=(1440*2*512);
	void* read_base=memman_alloc_page_64_4m(NULL);
	for(i=0;i<ahci_table_addr->number;i++){
		fis=ahci_make_fis(fis,read_base,0,read_size/512,0x25,0);
		ahci_fis_write_prdt(fis,0,read_base,read_size-1);
		unsigned int pi=(ahci_table_addr->ahci_dev[i].ahci_config_space_address->header.i.pi);
		for(m=0;m<32;m++){
			if((pi&(1<<m))==0){//没有设备
				continue;
			}
			//cons_putstr0(task_now()->cons," ahci test send ok\n");
			ahci_fis_send(&(ahci_table_addr->ahci_dev[i]),m,fis,1,0x05);
			for(;;){
				unsigned long long prdbc=*(fis->cfis.ahci_cfis_0x27.prdbc_addr);
				if(prdbc==(1440*2*512)){
					break;
				}
			}
			unsigned long long prdbc=*(fis->cfis.ahci_cfis_0x27.prdbc_addr);
			sprintf(text_buff,"prdbc:%ld\n",prdbc);
			cons_putstr0(task_now()->cons,text_buff);
			//找到启动磁盘
			unsigned long long disk_data_base=read_base;//数据加载的基地址
			unsigned long long disk_part_base=disk_data_base+0x400;
			
			for(j=0;j<16;j++){
				GPT_ITEM* p=((GPT_ITEM*)disk_part_base)+j;
				unsigned long long s=p->guid.Data1;
				//sprintf(text_buff,"s: %ld %ld\n",&(p->guid.Data1),sizeof(p->guid.Data1));
				//cons_putstr0(task_now()->cons,text_buff);
				if(p->guid.Data1==part_own_uid.Data1){
					if(p->guid.Data2==part_own_uid.Data2){
						if(p->guid.Data3==part_own_uid.Data3){
							for(k=0;k<8;k++){
								if(p->guid.Data4[k]!=part_own_uid.Data4[k]){
									break;
								}
							}
							if(k==8){//通过了全部测验
								cons_putstr0(task_now()->cons," ahci cmp ok\n");
								ahci_id=i;
								ahci_port=m;
								device_id=(1<<16)|((i&0xff)<<8)|(m&0xff);
								part_base_lba=p->start_lba_low;
								part_endl_lba=p->end_lba_low;
								break;
							}
						}
					}
				}
			}
			if(j<16){
				cons_putstr0(task_now()->cons," ahci cmp ok\n");
				break;
			}
		}
		if(m<32){
			cons_putstr0(task_now()->cons," ahci cmp ok\n");
			break;
		}
		
	}
	memman_free_page_64_4m(NULL,read_base);
	if(i<ahci_table_addr->number){
		void* p=task_now()->cons;
		//task_now()->cons=0;
		CACHE_TABLE* cache_table=0;
		cache_init(&cache_table);//缓存系统
		cache_table->pci_dev=&(ahci_table_addr->ahci_dev[ahci_id]);//ahci sata硬盘读写系统
		FILE* file=fat32_init(NULL,cache_table,device_id,part_base_lba);//fat32文件访问系统
		unsigned long long file_size;
		file->fsize(file,&file_size);
		char* buff=malloc(file_size);
		for(int i=0;i<file_size;i++){
			buff[i]=0;
		}
		file->fread(file,buff,0, file_size);
		task_now()->file_cache=cache_table;
		task_now()->file=file;
		task_now()->root_dir_addr=buff;
		cmd_dir(task_now()->cons);
	}
	return;
}
/*
buff:保存的地址
size：读取的大小
fat32_addr： fat表的地址
part_base_lba：本分区前的保留扇区数量
mbr：文件系统的头扇区
start_lba_low：文件的第一个簇的位置
*/
unsigned int _read_file(char* buff,unsigned int* size,unsigned int fat32_addr,unsigned part_base_lba,FAT32_HEADER* mbr,unsigned int start_lba_low,unsigned int start_lba_high,unsigned int device_id){
	unsigned int i;
	for(i=0;;i++){
		//如果多读一个簇就会超过size设定的大小
		if((i+1)*(mbr->BPB_SecPerClus)*512>*size){
			*size=i*(mbr->BPB_SecPerClus)*512;//设定已读内容数量
			return 0;
		}
		//扇区位置:逻辑分区基地址+保留分区+FAT所占的分区*FAT数量+(簇号-2)*簇大小
		dmg_read(buff+i*512*(mbr->BPB_SecPerClus),part_base_lba+mbr->BPB_ResvdSecCnt+(mbr->BPB_FATSz32)*(mbr->BPB_NumFATs)+(start_lba_low-2)*(mbr->BPB_SecPerClus),mbr->BPB_SecPerClus,device_id);
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
int dmg_read(char* buff,int lba28,int block_number,int device){
	if(((device>>16)&0xffff)==1){//ahci设备
		AHCI_SATA_FIS* fis=ahci_make_fis(fis,buff,lba28,block_number,0x25,0);
		ahci_fis_write_prdt(fis,0,buff,(block_number*512)-1);
		ahci_fis_send(&(ahci_table_addr->ahci_dev[(device>>8)&0xff]),device&0xff,fis,1,0x05);
		for(;;){
			unsigned long long prdbc=*(fis->cfis.ahci_cfis_0x27.prdbc_addr);
			if(prdbc==(block_number*512)){
				break;
			}
		}
		unsigned long long prdbc=*(fis->cfis.ahci_cfis_0x27.prdbc_addr);
		sprintf(text_buff,"dmg prdbc:%ld\n",prdbc);
		cons_putstr0(task_now()->cons,text_buff);
		return;
	}
	return;
	for(;;){
		if(block_number<=255){
			dmg_read2(buff,lba28,block_number,device);
			break;
		}
		else{
			dmg_read2(buff,lba28,255,device);
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