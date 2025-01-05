#include "bootpack.h"



extern char* text_buff;


EFI_STATUS fat32_create_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size){
	
}


EFI_STATUS fat32_read_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size){
	// 检查参数对齐
	unsigned int SECTOR_SIZE=512;
	unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
	unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;

    unsigned long long node_start = this->node_id;

	unsigned long long dbuff=(unsigned long long)buff;
    unsigned long long block_size = CLUSTER_SIZE;
    unsigned long long cluster_offset = offset / block_size;
    unsigned long long cluster_remainder = offset % block_size;
    unsigned long long current_cluster = node_start;

    // 找到起始簇
    for (unsigned long long i = 0; i < cluster_offset; ++i) {
        if (current_cluster == 0x0FFFFFFF) {
            return EFI_DEVICE_ERROR; // 超出文件尾部
        }
        current_cluster = this->fat32_fat[current_cluster];
    }

    unsigned long long buff_offset = 0;//buff的偏移量
    unsigned long long sectors_to_read = size;//所需的总读取数量

	unsigned long long count=0;
	
	
    // 处理非对齐的起始位置
    if (cluster_remainder != 0) {
        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus) +
                                 (cluster_remainder / SECTOR_SIZE);

        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus * 512 - cluster_remainder;//一个簇的字节数量
        if (sectors_in_cluster > sectors_to_read) {//如果所需大小小于一个簇的大小
            sectors_in_cluster = sectors_to_read;
        }
		
		this->cache->read(this->cache,buff,lba*512+cluster_remainder,sectors_in_cluster);
		count+=sectors_in_cluster;
        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降

        current_cluster = this->fat32_fat[current_cluster];
    }

    // 处理对齐的簇
    while (sectors_to_read > 0 && current_cluster != 0x0FFFFFFF) {
        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus * 512;
        if (sectors_in_cluster > sectors_to_read) {//如果所需大小小于一个簇的大小
            sectors_in_cluster = sectors_to_read;
        }

        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus);

		this->cache->read(this->cache,(unsigned long long)buff+buff_offset,lba*512,sectors_in_cluster);
		count+=sectors_in_cluster;

        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降


        current_cluster = this->fat32_fat[current_cluster];
    }
	this->cache->sync(this->cache);
    return EFI_SUCCESS;
}

EFI_STATUS fat32_write_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size){
	// 检查参数对齐
	unsigned int SECTOR_SIZE=512;
	unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
	unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;

    unsigned long long node_start = this->node_id;

	unsigned long long dbuff=(unsigned long long)buff;
    unsigned long long block_size = CLUSTER_SIZE;
    unsigned long long cluster_offset = offset / block_size;
    unsigned long long cluster_remainder = offset % block_size;
    unsigned long long current_cluster = node_start;

    // 找到起始簇
    for (unsigned long long i = 0; i < cluster_offset; ++i) {
        if (current_cluster == 0x0FFFFFFF) {
            return EFI_DEVICE_ERROR; // 超出文件尾部
        }
        current_cluster = this->fat32_fat[current_cluster];
    }

    unsigned long long buff_offset = 0;//buff的偏移量
    unsigned long long sectors_to_read = size;//所需的总读取数量

	unsigned long long count=0;
	
	
    // 处理非对齐的起始位置
    if (cluster_remainder != 0) {
        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus) +
                                 (cluster_remainder / SECTOR_SIZE);

        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus * 512 - cluster_remainder;//一个簇的字节数量
        if (sectors_in_cluster > sectors_to_read) {//如果所需大小小于一个簇的大小
            sectors_in_cluster = sectors_to_read;
        }
		
		this->cache->write(this->cache,buff,lba*512+cluster_remainder,sectors_in_cluster);
		count+=sectors_in_cluster;
        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降

        current_cluster = this->fat32_fat[current_cluster];
    }

    // 处理对齐的簇
    while (sectors_to_read > 0 && current_cluster != 0x0FFFFFFF) {
        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus * 512;
        if (sectors_in_cluster > sectors_to_read) {//如果所需大小小于一个簇的大小
            sectors_in_cluster = sectors_to_read;
        }

        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus);

		this->cache->write(this->cache,(unsigned long long)buff+buff_offset,lba*512,sectors_in_cluster);
		count+=sectors_in_cluster;

        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降


        current_cluster = this->fat32_fat[current_cluster];
    }
	this->cache->sync(this->cache);
	this->cache->flush(this->cache);
    return EFI_SUCCESS;
}



#define MAX_FIS 32
AHCI_SATA_FIS* fis[MAX_FIS];


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


extern AHCI_TABLE* ahci_table_addr;
FILE_OF_FAT32* fat32_init(FILE_OF_FAT32* file_info,CACHE_TABLE* cache_table ,unsigned int device_id,unsigned long long part_base_lba){
	if(file_info==0){
		file_info=memman_alloc_page_64_4k(NULL);
	}
	file_info->cache=cache_table;
	FAT32_HEADER* mbr=memman_alloc_page_64_4k(NULL);
	
	//dmg_read(mbr,part_base_lba,1,device_id);
	cache_table->read(cache_table,mbr,part_base_lba*512,512);
	cache_table->sync(cache_table);
	file_info->mbr=mbr;
	
	
	unsigned int BPB_TotSec32=mbr->BPB_TotSec32;//fat大小
	unsigned int BPB_ResvdSecCnt=mbr->BPB_ResvdSecCnt;//fat前保留的扇区数
	//获取FAT大小
	unsigned int BPB_FATSz32=mbr->BPB_FATSz32;
	unsigned int* fat32_addr=memman_alloc_page_64_4m(NULL);//分配内存
	
	//dmg_read(fat32_addr,part_base_lba+mbr->BPB_ResvdSecCnt,BPB_FATSz32,device_id);
	cache_table->read(cache_table,fat32_addr,(part_base_lba+mbr->BPB_ResvdSecCnt)*512,BPB_FATSz32*512);
	cache_table->sync(cache_table);
	file_info->fat32_fat=fat32_addr;
	
	//写一遍测试写功能
	//cache_table->write(cache_table,fat32_addr,(part_base_lba+mbr->BPB_ResvdSecCnt)*512,BPB_FATSz32*512);
	//cache_table->sync(cache_table);
	//cache_table->flush(cache_table);
	
	
	file_info->device_id=device_id;
	file_info->part_base_lba=part_base_lba;
	file_info->node_id=mbr->BPB_Root;
	//注册函数
	file_info->fread=fat32_read_file_from_cache;
	file_info->fwrite=fat32_write_file_from_cache;
	return file_info;
}

void fat32_get_info(unsigned int device,unsigned long long node){
	
}

EFI_STATUS fat32_read_cache(CACHE_TABLE* this, void* buff, unsigned long long offset, unsigned long long size, FILE_OF_FAT32* file_info) {
		unsigned long long current_cluster = offset / (file_info->mbr->BPB_SecPerClus * 512);
		unsigned long long cluster_offset = offset % (file_info->mbr->BPB_SecPerClus * 512);
		unsigned long long remaining_size = size;
		unsigned long long buff_offset = 0;
		while (remaining_size > 0) {
			unsigned long long cluster_lba = file_info->part_base_lba + file_info->mbr->BPB_ResvdSecCnt +
											 (file_info->mbr->BPB_FATSz32 * file_info->mbr->BPB_NumFATs) +
											 (current_cluster - 2) * file_info->mbr->BPB_SecPerClus;
			unsigned long long read_size = file_info->mbr->BPB_SecPerClus * 512 - cluster_offset;
			if (read_size > remaining_size) {
				read_size = remaining_size;
			}

			EFI_STATUS status = this->read(this, (char*)buff + buff_offset, cluster_lba * 512 + cluster_offset, read_size);
			if (status != EFI_SUCCESS) {
				return status;
			}

			remaining_size -= read_size;
			buff_offset += read_size;
			cluster_offset = 0;
			current_cluster = file_info->fat32_fat[current_cluster];
			if (current_cluster == 0x0FFFFFFF) {
				break;
			}
		}

		return EFI_SUCCESS;
	}


EFI_STATUS fat32_write_cache(CACHE_TABLE* this,void* buff,unsigned long long offset,unsigned long long size){

}