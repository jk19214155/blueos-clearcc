#include "bootpack.h"
#define FILE_FUNC \
	EFI_STATUS (*fread)(struct _FILE* this,void* buff,unsigned long long seek,unsigned long long size); \
	EFI_STATUS (*fwrite)(struct _FILE* this,void* buff,unsigned long long seek,unsigned long long size); \
	EFI_STATUS (*fsize)(struct _FILE* this,unsigned long long* size); \
	EFI_STATUS (*fopen)(struct _FILE* this,unsigned long long index); \
	EFI_STATUS (*fclose)(struct _FILE* this);
typedef struct _FILE{
	FILE_FUNC
}FILE;
#define CACHE_FUNC \
	EFI_STATUS (*read)(void *this, void *buff, unsigned long long offset, unsigned long long size); \
	EFI_STATUS (*write)(void *this, void *buff, unsigned long long offset, unsigned long long size); \
    EFI_STATUS (*sync)(void *this);
typedef struct _CACHE{
	CACHE_FUNC
}CACHE;
typedef struct _FILE_OF_FAT32{
	FILE_FUNC
	struct{
		void* buff;//size:4M
		unsigned long long seek;
		char flag;
	}temp[16];
	unsigned long long node_id;//当前文件的起始FAT编号
	unsigned long long device_id;
	unsigned long long part_base_lba;
	unsigned long long part_end_lba;
	unsigned long long part_id;
	PCI_DEV* pci_dev;
	unsigned long long seek;
	unsigned long long fsizeof;
	unsigned int* fat32_fat;
	unsigned long long size;
	FAT32_HEADER* mbr;
	CACHE* cache;
	
}FILE_OF_FAT32;

extern char* text_buff;








#define MAX_FIS 32
EFI_STATUS fat32_read_file_from_ahci(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size) {
    // 检查参数对齐
	cons_putstr0(task_now()->cons, "fat32:read_file_from_ahci start\n");
	sprintf(text_buff,"mbr:%ld\n",this->mbr);
	cons_putstr0(task_now()->cons,text_buff);
	unsigned int SECTOR_SIZE=512;
	unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
	unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;
    if (offset % SECTOR_SIZE != 0 || size % SECTOR_SIZE != 0) {
        return EFI_DEVICE_ERROR;
    }

    unsigned long long node_start = this->node_id;


    unsigned long long block_size = CLUSTER_SIZE;
    unsigned long long cluster_offset = offset / block_size;
    unsigned long long cluster_remainder = offset % block_size;
    unsigned long long current_cluster = node_start;
	cons_putstr0(task_now()->cons, "fat32:read_file_from_ahci start\n");
    // 找到起始簇
    for (unsigned long long i = 0; i < cluster_offset; ++i) {
        if (current_cluster == 0x0FFFFFFF) {
            return EFI_DEVICE_ERROR; // 超出文件尾部
        }
        current_cluster = this->fat32_fat[current_cluster];
    }

    AHCI_SATA_FIS* fis[MAX_FIS];
	unsigned long long fis_data[MAX_FIS];
    unsigned long long buff_offset = 0;
    unsigned long long sectors_to_read = size / SECTOR_SIZE;
    unsigned long long fis_index = 0;
	unsigned long long count=0;
	for(int i=0;i<MAX_FIS;i++){
		fis[i]=0;
	}
	cons_putstr0(task_now()->cons, "fat32:read_file_from_ahci start\n");
    // 处理非对齐的起始位置
    if (cluster_remainder != 0) {
        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus) +
                                 (cluster_remainder / SECTOR_SIZE);

        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus - (cluster_remainder / SECTOR_SIZE);
        if (sectors_in_cluster > sectors_to_read) {
            sectors_in_cluster = sectors_to_read;
        }
		cons_putstr0(task_now()->cons, "fat32:read_file_from_ahci start\n");
		
		sprintf(text_buff,"makefis:lba:%ld count:%ld",lba,sectors_in_cluster);
		cons_putstr0(task_now()->cons,text_buff);
		sprintf(text_buff,"prdt:buff:%ld size:%ld",buff,sectors_in_cluster*512-1);
		cons_putstr0(task_now()->cons,text_buff);
        fis[fis_index] = ahci_make_fis(fis[fis_index], NULL, lba, sectors_in_cluster, 0x25, 0);
		fis_data[fis_index]=sectors_in_cluster*512;
		ahci_fis_write_prdt(fis[fis_index],0,(char*)buff,sectors_in_cluster*512-1);
		count+=sectors_in_cluster;
        buff_offset += sectors_in_cluster * SECTOR_SIZE;
        sectors_to_read -= sectors_in_cluster;
        fis_index++;

        current_cluster = this->fat32_fat[current_cluster];
    }
	cons_putstr0(task_now()->cons, "fat32:read_file_from_ahci start\n");
    // 处理对齐的簇
    while (sectors_to_read > 0 && current_cluster != 0x0FFFFFFF) {
        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus;
        if (sectors_in_cluster > sectors_to_read) {
            sectors_in_cluster = sectors_to_read;
        }

        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus);

        fis[fis_index] = ahci_make_fis(fis[fis_index], NULL, lba, sectors_in_cluster, 0x25, 0);
		fis_data[fis_index]=sectors_in_cluster*512;
		//ahci_fis_write_prdt(AHCI_FIS* fis,unsigned int prdt_index,void* buff,unsigned int size);
		ahci_fis_write_prdt(fis[fis_index],0,(char*)buff+count*512,sectors_in_cluster*512-1);
		sprintf(text_buff,"makefis:lba:%ld count:%ld\n",lba,sectors_in_cluster);
		cons_putstr0(task_now()->cons,text_buff);
		sprintf(text_buff,"prdt:buff:%ld size:%ld\n",(char*)buff+count*512,sectors_in_cluster*512-1);
		cons_putstr0(task_now()->cons,text_buff);
		count+=sectors_in_cluster;
        if (!fis[fis_index]) {
            return EFI_DEVICE_ERROR; // 无法创建FIS
        }

        buff_offset += sectors_in_cluster * SECTOR_SIZE;
        sectors_to_read -= sectors_in_cluster;
        fis_index++;

        if (fis_index == MAX_FIS || sectors_to_read == 0 ) {
            EFI_STATUS status;
			for(int i=0;i<fis_index;i++){
				//ahci_fis_send(??,??,unsigned int prdt_number);
				status=ahci_fis_send(this->pci_dev,(this->device_id)&0xff,fis[i],1,0x05);
			}
            if (EFI_ERROR(status)) {
                return status; // 发送FIS失败
            }
			AHCI_DEV* ahci_dev=this->pci_dev;
			AHCI_REGS* ahci_base_address=ahci_dev->ahci_config_space_address;
			//等待传输完成
			for(int i=0;i<fis_index;i++){
				unsigned long long prdbc=*(fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
				if(prdbc==fis_data[i]){
					continue;
				}
				else{
					i=-1;
					continue;
				}
			}
			for(int i=0;i<fis_index;i++){
				unsigned long long prdbc=*(fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
				sprintf(text_buff,"prdbc:%ld\n",prdbc);
				cons_putstr0(task_now()->cons,text_buff);
			}
            fis_index = 0;
			(ahci_base_address->port)[this->device_id].i.HBA_RPxCI=0;
        }

        current_cluster = this->fat32_fat[current_cluster];
    }
	//如果仍有FIS没有发出，则进行发出
	if(fis_index!=0){
		EFI_STATUS status;
		for(int i=0;i<fis_index;i++){
			status=ahci_fis_send(this->pci_dev,(this->device_id)&0xff,fis[i],1,0x05);
		}
        if (EFI_ERROR(status)) {
            return status; // 发送FIS失败
        }
		AHCI_DEV* ahci_dev=this->pci_dev;
		AHCI_REGS* ahci_base_address=ahci_dev->ahci_config_space_address;
		//等待传输完成
		for(int i=0;i<fis_index;i++){
			unsigned long long prdbc=*(fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
			if(prdbc==fis_data[i]){
				continue;
			}
			else{
				i=-1;
				continue;
			}
		}
		for(int i=0;i<fis_index;i++){
			unsigned long long prdbc=*(fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
			sprintf(text_buff,"prdbc:%ld\n",prdbc);
			cons_putstr0(task_now()->cons,text_buff);
		}
        fis_index = 0;
		(ahci_base_address->port)[this->device_id].i.HBA_RPxCI=0;
	}
	//收尾工作 回收内存
	for(int i=0;i<MAX_FIS;i++){
		if(fis[i]!=0){
			memman_free_page_64_4k(NULL,fis[i]);
		}
	}
	cons_putstr0(task_now()->cons, "fat32:read_file_from_ahci end\n");
    return EFI_SUCCESS;
}




#define BLOCK_SIZE (4 * 1024 * 1024) // 4MB

#define CACHE_SIZE 16

EFI_STATUS fat32_read(struct _FILE_OF_FAT32* this,void* buff,unsigned long long seek,unsigned long long size){
	unsigned int i;
	if(this==0||buff==0){
		return 0xffffffff;
	}
	if(seek!=0xffffffff){
		this->seek=seek;
	}
	else{
		seek=this->seek;
	}
	if(size==0){
		return 0;
	}
	if(size>this->size){
		size=this->size;
	}
	sprintf(text_buff,"read size:%ld\n",size);
	cons_putstr0(task_now()->cons,text_buff);
	unsigned long long end_seek = seek + size;
    unsigned long long current_seek = seek;
    unsigned char* dest = (unsigned char*)buff;
    void* new_buff=0;
    while (current_seek < end_seek) {
        unsigned long long aligned_seek = current_seek & ~(BLOCK_SIZE - 1); // Align to 4MB boundary
        unsigned long long offset = current_seek - aligned_seek; // Offset within the block
        unsigned long long remaining_size_in_block = BLOCK_SIZE - offset;
        unsigned long long copy_size = (end_seek - current_seek < remaining_size_in_block) ? end_seek - current_seek : remaining_size_in_block;
        
        // Check if the aligned block is in cache
		
        for (i = 0; i < CACHE_SIZE; i++) {
            if ((this->temp[i].seek == aligned_seek) && (this->temp[i].flag!=0)) {
                asm_memcpy(dest, (unsigned char*)this->temp[i].buff + offset, copy_size);
                break;
            }
        }
        if (i>=CACHE_SIZE) {
            // If not found in cache, load the block from disk
			if(new_buff==0){
				new_buff = memman_alloc_page_64_4m(NULL);
			}
			unsigned long long cache_size=CACHE_SIZE;
            if (fat32_read_file_from_ahci(this,new_buff,aligned_seek,BLOCK_SIZE)) {
                memman_free_page_64_4m(NULL,new_buff);
                return EFI_DEVICE_ERROR;
            }
            
			
			
            // Update the cache
            int insert_pos = 0;
            if (this->temp[CACHE_SIZE - 1].buff != NULL) {
                //free(this->temp[CACHE_SIZE - 1].buff);
            }
            for (insert_pos = CACHE_SIZE - 1; insert_pos > 0; insert_pos--) {
                if (this->temp[insert_pos - 1].seek < aligned_seek) {
                    break;
                }
                this->temp[insert_pos] = this->temp[insert_pos - 1];
            }
            this->temp[insert_pos].buff = new_buff;
            this->temp[insert_pos].seek = aligned_seek;
            this->temp[insert_pos].flag = 1;
			
			
            asm_memcpy(dest, (unsigned char*)new_buff + offset, copy_size);
        }else{
			sprintf(text_buff,"cache :%ld\n",i);
			cons_putstr0(task_now()->cons,text_buff);
		}
        
        current_seek += copy_size;
        dest += copy_size;
    }
    
    return EFI_SUCCESS;
}
extern AHCI_TABLE* ahci_table_addr;
FILE_OF_FAT32* fat32_init(FILE_OF_FAT32* file_info,unsigned int device_id,unsigned long long part_base_lba){
	if(file_info==0){
		file_info=memman_alloc_page_64_4k(NULL);
	}
	FAT32_HEADER* mbr=memman_alloc_page_64_4k(NULL);
	dmg_read(mbr,part_base_lba,1,device_id);
	unsigned int BPB_TotSec32=mbr->BPB_TotSec32;//fat大小
	unsigned int BPB_ResvdSecCnt=mbr->BPB_ResvdSecCnt;//fat前保留的扇区数
	//获取FAT大小
	unsigned int BPB_FATSz32=mbr->BPB_FATSz32;
	unsigned int* fat32_addr=memman_alloc_page_64_4m(NULL);//分配内存
	dmg_read(fat32_addr,part_base_lba+mbr->BPB_ResvdSecCnt,BPB_FATSz32,device_id);
	file_info->fat32_fat=fat32_addr;
	file_info->node_id=mbr->BPB_Root;
	file_info->pci_dev=&(ahci_table_addr->ahci_dev[(device_id>>8)&0xff]);
	file_info->mbr=mbr;
	for(int i=mbr->BPB_Root;;){
		if(i==0xfffffff){
			file_info->size=i*mbr->BPB_SecPerClus*512;
			break;
		}
		else{
			i=fat32_addr[i];
		}
	}
	file_info->device_id=device_id;
	file_info->part_base_lba=part_base_lba;
	//注册函数
	file_info->fread=fat32_read_file_from_ahci;
	
	return file_info;
}

void fat32_get_info(unsigned int device,unsigned long long node){
	
}