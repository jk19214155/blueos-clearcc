#include "bootpack.h"

EFI_STATUS fat32_find_node_from_fat(FILE_OF_FAT32* this,unsigned int* node){
	printf("call fat32_find_node_from_fat\n");
	for (unsigned int i = 2; i < (this->mbr->BPB_FATSz32)*512; i++) {
		//printf("call fat32_find_node_from_fat find %ld\n",i);
		if (this->fat32_fat[i] == 0) { // 找到未分配簇
			*node=i;
			this->fat32_fat[i]=0x0FFFFFFF;
			printf("call fat32_find_node_from_fat finded %ld\n",i);
			return EFI_SUCCESS;
		}
	}
	return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS _flush_fat(FILE_OF_FAT32* this){
	//写入磁盘块并同步
	for(int i=0;i<this->mbr->BPB_NumFATs;i++){
		this->cache->write(this->cache,this->fat32_fat,(this->part_base_lba+this->mbr->BPB_ResvdSecCnt+i*this->mbr->BPB_FATSz32)*512,this->mbr->BPB_FATSz32*512);
	}
	this->cache->sync(this->cache);
    this->cache->flush(this->cache);
}




EFI_STATUS __fat32_read_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size,unsigned long long node_start){
	char* temp=memman_alloc_page_64_4m(NULL);
	// 检查参数对齐
	unsigned int SECTOR_SIZE=512;
	unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
	unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;


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
		//从缓冲区temp复制到buff
		//for(int i=0;i<sectors_in_cluster;i++){
		//	((char*)buff)[i]=temp[i];
		//}
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
		//从缓冲区temp复制到buff
		//for(int i=0;i<sectors_in_cluster;i++){
		//	((char*)((unsigned long long)buff+buff_offset))[i]=temp[i];
		//}
		count+=sectors_in_cluster;

        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降


        current_cluster = this->fat32_fat[current_cluster];
    }
	this->cache->sync(this->cache);
    return EFI_SUCCESS;
}

EFI_STATUS fat32_read_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size){
	if(offset+size<=this->size){//读取位置都在文件范围内
		__fat32_read_file_from_cache(this,buff,offset,size,this->fnode);
	}
	else if(offset<this->size){//部分内容在文件范围内
		__fat32_read_file_from_cache(this,buff,offset,this->size-offset,this->fnode);
		memset((unsigned long long)buff+this->size-offset,0,size+offset-this->size);
	}
	else{//读取范围在文件范围外
		memset((unsigned long long)buff,0,size);
	}
	return 0;
}


EFI_STATUS __fat32_write_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size,unsigned long long node_start){
	char* temp=memman_alloc_page_64_4m(NULL);
	// 检查参数对齐
	unsigned int SECTOR_SIZE=512;
	unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
	unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;

	//看一下本次读取的最大大小是否已经包含在簇链中
	unsigned long long dbuff=(unsigned long long)buff;//目标buff 线性地址
    unsigned long long block_size = CLUSTER_SIZE;//簇大小
    unsigned long long cluster_offset = offset / block_size;//写入起始簇
	unsigned long long max_cluster_offset = (offset+size) / block_size;//写入最大簇
    unsigned long long cluster_remainder = offset % block_size; //块内读取位置
    unsigned long long current_cluster = node_start;//文件起始簇
	// 找到起始簇
	unsigned long long i=0;
    for (unsigned long long i = 0; i < cluster_offset; ++i) {
		if(this->fat32_fat[current_cluster] == 0x0FFFFFFF){
			unsigned int new_current_cluster;
            fat32_find_node_from_fat(this,&new_current_cluster); // 超出文件尾部 申请一个新的id
			this->fat32_fat[current_cluster]=new_current_cluster;
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
		//从缓冲区temp复制到buff
		//for(int i=0;i<sectors_in_cluster;i++){
		//	temp[i]=((char*)((unsigned long long)buff+buff_offset))[i];
		//}
		this->cache->write(this->cache,buff,lba*512+cluster_remainder,sectors_in_cluster);
		count+=sectors_in_cluster;
        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降

        if(sectors_to_read > 0){//如果还有要写的内容
			if(this->fat32_fat[current_cluster]==0x0FFFFFFF){
				unsigned int new_current_cluster;
				fat32_find_node_from_fat(this,&new_current_cluster); // 超出文件尾部 申请一个新的id
				this->fat32_fat[current_cluster]=new_current_cluster;
			}
			current_cluster = this->fat32_fat[current_cluster];
		}
    }

    // 处理对齐的簇
    while (sectors_to_read > 0) {
        unsigned long long sectors_in_cluster = this->mbr->BPB_SecPerClus * 512;
        if (sectors_in_cluster > sectors_to_read) {//如果所需大小小于一个簇的大小
            sectors_in_cluster = sectors_to_read;
        }

        unsigned long long lba = this->part_base_lba + this->mbr->BPB_ResvdSecCnt +
                                 (this->mbr->BPB_FATSz32 * this->mbr->BPB_NumFATs) +
                                 ((current_cluster - 2) * this->mbr->BPB_SecPerClus);
		//从缓冲区temp复制到buff
		//for(int i=0;i<sectors_in_cluster;i++){
		//	temp[i]=((char*)((unsigned long long)buff+buff_offset))[i];
		//}
		this->cache->write(this->cache,(unsigned long long)buff+buff_offset,lba*512,sectors_in_cluster);
		count+=sectors_in_cluster;

        buff_offset += sectors_in_cluster;//buff偏移加上本次读取大小
        sectors_to_read -= sectors_in_cluster;//所需读取内容数量下降

		if(sectors_to_read > 0){//如果还有要写的内容
			if(this->fat32_fat[current_cluster]==0x0FFFFFFF){
				unsigned int new_current_cluster;
				fat32_find_node_from_fat(this,&new_current_cluster); // 超出文件尾部 申请一个新的id
				this->fat32_fat[current_cluster]=new_current_cluster;
				this->fat32_fat[new_current_cluster]=0x0FFFFFFF;
			}
			current_cluster = this->fat32_fat[current_cluster];
		}
    }
	this->cache->sync(this->cache);
	this->cache->flush(this->cache);
	_flush_fat(this);
    return EFI_SUCCESS;
}
EFI_STATUS fat32_write_file_from_cache(FILE_OF_FAT32* this, void* buff, unsigned long long offset, unsigned long long size){
	if(buff!=0){
		// 检查参数对齐
		unsigned int SECTOR_SIZE=512;
		unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
		unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;

		//看一下本次读取的最大大小是否已经包含在簇链中
		unsigned long long dbuff=(unsigned long long)buff;//目标buff 线性地址
		unsigned long long block_size = CLUSTER_SIZE;//簇大小
		unsigned long long cluster_offset = offset / block_size;//写入起始簇
		unsigned long long max_cluster_offset = (offset+size) / block_size;//写入最大簇
		unsigned long long cluster_remainder = offset % block_size; //块内读取位置
		unsigned long long current_cluster = this->fnode;//文件起始簇
		// 如果写入大小不是0并且当前文件没有分配簇号的话
		if (size > 0 && current_cluster == 0) {
			// 分配一个新的簇
			unsigned int new_cluster;
			EFI_STATUS status = fat32_find_node_from_fat(this, &new_cluster);
			this->fnode = new_cluster; // 更新文件起始簇
			current_cluster = new_cluster;
			if(max_cluster_offset!=0){//继续分配簇链
				unsigned int current_cluster_a=new_cluster;
				unsigned int current_cluster_b=0;
				for(unsigned int count=0;count<max_cluster_offset;count++){
					EFI_STATUS status = fat32_find_node_from_fat(this, &current_cluster_b);
					this->fat32_fat[current_cluster_a]=current_cluster_b;
					current_cluster_a=current_cluster_b;
				}
			}
			// 更新目录文件，将新的簇号写入目录项
			unsigned int directory_entry_size = 32; // FAT32 目录项大小为 32 字节
			unsigned int findex = this->findex; // 文件在目录中的顺序记录偏移号
			unsigned long long directory_entry_offset = findex * directory_entry_size; // 目录项在目录文件中的偏移

			// 读取目录文件的簇
			unsigned int directory_cluster = this->dnode; // 目录文件的起始簇号
			FAT32_FILE_INFO* directory_buffer = (unsigned char*)malloc(CLUSTER_SIZE); // 分配一个簇大小的缓冲区
			if (!directory_buffer) {
				return EFI_OUT_OF_RESOURCES; // 内存分配失败
			}

			// 读取目录簇的内容
			status = __fat32_read_file_from_cache(this, directory_buffer, directory_entry_offset, directory_entry_size, directory_cluster);
			if (status != EFI_SUCCESS) {
				free(directory_buffer);
				return status; // 读取失败
			}

			// 更新目录项中的起始簇号
			directory_buffer->start_low = new_cluster & 0xFFFF; // 低 16 位
			directory_buffer->start_high = (new_cluster >> 16) & 0xFFFF; // 高 16 位
			directory_buffer->file_size=offset+size;//大小
			// 将更新后的目录项写回目录文件
			status = __fat32_write_file_from_cache(this, directory_buffer, directory_entry_offset, directory_entry_size, directory_cluster);
			if (status != EFI_SUCCESS) {
				free(directory_buffer);
				return status; // 写入失败
			}
			this->size=offset+size;
			free(directory_buffer); // 释放缓冲区
		}else if(this->size<offset+size){//写入范围比当前文件的大小更大
			unsigned now_current_cluster=current_cluster;
			unsigned int current_cluster_number=0;//当前簇数量
			for(;;){
				if(this->fat32_fat[now_current_cluster]==0x0fffffff){
					break;
				}
				now_current_cluster=this->fat32_fat[now_current_cluster];
				current_cluster_number++;
			}
			if(max_cluster_offset-current_cluster_number>0){//继续分配簇链
				unsigned int current_cluster_a=now_current_cluster;
				unsigned int current_cluster_b=0;
				for(unsigned int count=0;count<max_cluster_offset-current_cluster_number;count++){
					EFI_STATUS status = fat32_find_node_from_fat(this, &current_cluster_b);
					this->fat32_fat[current_cluster_a]=current_cluster_b;
					current_cluster_a=current_cluster_b;
				}
			}
			
			
			FAT32_FILE_INFO* directory_buffer = (unsigned char*)malloc(CLUSTER_SIZE); // 分配一个目录项大小的缓冲区
			unsigned int directory_entry_size = 32; // FAT32 目录项大小为 32 字节
			unsigned int findex = this->findex; // 文件在目录中的顺序记录偏移号
			unsigned int directory_cluster = this->dnode; // 目录文件的起始簇号
			unsigned long long directory_entry_offset = findex * directory_entry_size; // 目录项在目录文件中的偏移
			__fat32_read_file_from_cache(this, directory_buffer, directory_entry_offset, 32, directory_cluster);
			directory_buffer->file_size=offset+size;
			__fat32_write_file_from_cache(this, directory_buffer, directory_entry_offset, 32, directory_cluster);
			free(directory_buffer);
			this->size=offset+size;
			
		}
		
		_flush_fat(this);
		__fat32_write_file_from_cache(this,buff,offset, size,this->fnode);
		
	}
	else if(size==0){//截断模式
		unsigned int SECTOR_SIZE=512;
		unsigned int SECTORS_PER_CLUSTER =this->mbr->BPB_SecPerClus;
		unsigned int CLUSTER_SIZE= SECTORS_PER_CLUSTER*SECTOR_SIZE;
		unsigned int current_cluster=this->fnode;
		unsigned long long block_size = CLUSTER_SIZE;//簇大小
		unsigned long long cluster_offset = offset / block_size;//截断起始簇
		unsigned long long cluster_remainder = offset % block_size; //块内截断位置
		// 找到起始簇
		unsigned long long i=0;
		for (unsigned long long i = 0; i < cluster_offset; ++i) {
			current_cluster = this->fat32_fat[current_cluster];
		}
		if(this->fat32_fat[current_cluster]!=0x0fffffff){//断掉后面的簇链
			unsigned nexta=this->fat32_fat[current_cluster];
			unsigned nextb=this->fat32_fat[nexta];
			this->fat32_fat[current_cluster]=0x0fffffff;
			for(;;){
				if(nextb==0x0fffffff){//结束了
					this->fat32_fat[nexta]=0;
					break;
				}
				else{
					this->fat32_fat[nexta]=0;
					nexta=nextb;
					nextb=this->fat32_fat[nexta];
				}
			}
		}
		if(offset==0){//缩到0大小
			this->fat32_fat[this->fnode]=0;
		}
		//更改文件大小
		// 更新目录文件，将新的簇号写入目录项
		unsigned int directory_entry_size = 32; // FAT32 目录项大小为 32 字节
		unsigned int findex = this->findex; // 文件在目录中的顺序记录偏移号
		unsigned long long directory_entry_offset = findex * directory_entry_size; // 目录项在目录文件中的偏移

		// 读取目录文件的簇
		unsigned int directory_cluster = this->dnode; // 目录文件的起始簇号
		FAT32_FILE_INFO* directory_buffer = (unsigned char*)malloc(CLUSTER_SIZE); // 分配一个簇大小的缓冲区
		if (!directory_buffer) {
			return EFI_OUT_OF_RESOURCES; // 内存分配失败
		}

		// 读取目录簇的内容
		EFI_STATUS status = __fat32_read_file_from_cache(this, directory_buffer, directory_entry_offset, directory_entry_size, directory_cluster);
		if (status != EFI_SUCCESS) {
			free(directory_buffer);
			return status; // 读取失败
		}

		// 更新目录项中的起始簇号
		directory_buffer->file_size=offset;//大小
		if(offset==0){
			directory_buffer->start_high=0;
			directory_buffer->start_low=0;
		}
		// 将更新后的目录项写回目录文件
		status = __fat32_write_file_from_cache(this, directory_buffer, directory_entry_offset, directory_entry_size, directory_cluster);
		if (status != EFI_SUCCESS) {
			free(directory_buffer);
			return status; // 写入失败
		}
		this->size=offset;
		free(directory_buffer); // 释放缓冲区
		_flush_fat(this);
	}
	return 0;
}


EFI_STATUS fat32_fsize(FILE_OF_FAT32* this, unsigned long long* size){
	// 检查参数对齐
    unsigned long long current_cluster = this->fnode;
	unsigned long long count=0;
	if(current_cluster==0){
		*size=0;
		return 0;
	}
    // 统计簇数量
    for (;;) {
		count++;
        if (current_cluster == 0x0FFFFFFF) {
           break; // 超出文件尾部
        }
        current_cluster = this->fat32_fat[current_cluster];
    }
	*size=count*this->mbr->BPB_SecPerClus*512;
	return 0;
} 

EFI_STATUS __fat32_fsize(FILE_OF_FAT32* this, unsigned long long* size,unsigned int node){
	// 检查参数对齐
    unsigned long long current_cluster = node;
	unsigned long long count=0;
	if(current_cluster==0){
		*size=0;
		return 0;
	}
    // 统计簇数量
    for (;;) {
		count++;
        if (this->fat32_fat[current_cluster] == 0x0FFFFFFF) {
           break; // 超出文件尾部
        }
        current_cluster = this->fat32_fat[current_cluster];
    }
	*size=count*this->mbr->BPB_SecPerClus*512;
	return 0;
} 


EFI_STATUS fat32_open_file_from_cache(FILE_OF_FAT32* this, char* file_name, unsigned int* index){
	unsigned int start=*index;
	unsigned long long node=this->dnode;
	FAT32_FILE_INFO sfn_entry;
	unsigned int longer=strlen(file_name);
	if(longer>8){//限制在8个字符
		longer=8;
	}
	unsigned int size=0;
	__fat32_fsize(this,&size,node);
	size/=32;
	for(start;start<size;start++) {
		//sprintf(text_buff,"dmg prdbc:%ld\n",prdbc);
		//读入文件内容
        EFI_STATUS status = __fat32_read_file_from_cache(this, &sfn_entry, start*32, 32,this->dnode);
		int i;
		for(i=0;i<longer;i++){
			if(file_name[i]!=sfn_entry.name[i]){
				break;
			}
		}
		if(i==longer){
			for(i;i<8;i++){
				if(sfn_entry.name[i]!=' '){
					break;
				}
			}
			if(i==8){//匹配成功
				this->findex=start;
				this->size=sfn_entry.file_size;
				this->fnode=sfn_entry.start_low | ((sfn_entry.start_high) << 16);
				*index=start;
				return 0;
			}
		}
    }
}

EFI_STATUS fat32_delete_file_from_cache(FILE_OF_FAT32* this, unsigned long long index){
	unsigned long long node=this->dnode;
	FAT32_FILE_INFO sfn_entry;
	unsigned int size=0;
    EFI_STATUS status = __fat32_read_file_from_cache(this, &sfn_entry, index*32, 32,this->dnode);
	for(int i=0;i<8;i++){
		sfn_entry.name[i]=0;
	}
	sfn_entry.file_size=0;
	sfn_entry.type=0;
	__fat32_write_file_from_cache(this, &sfn_entry, index*32, 32,this->dnode);
	return 0;
} 

EFI_STATUS fat32_create_file_from_cache(FILE_OF_FAT32* this, char* file_name) {
	char text_buff[128];
    if (!this || !file_name) {
        return EFI_INVALID_PARAMETER;
    }
	cons_putstr0(task_now()->cons,"fat32_create_file_from_cache running\n");
    FAT32_HEADER* mbr = this->mbr;
    unsigned int cluster_size = mbr->BPB_BytesPerSec * mbr->BPB_SecPerClus;
    unsigned int current_cluster = this->dnode;

    // 1. 查找空目录项（支持跨簇）
    unsigned char* cluster_buffer = memman_alloc_page_64_4m(NULL);
    if (!cluster_buffer) {
        return EFI_OUT_OF_RESOURCES;
    }

    unsigned char* free_entry = NULL;
	unsigned long long index=0;
	int while_number=0;
    while (1) {
		//sprintf(text_buff,"dmg prdbc:%ld\n",prdbc);
		cons_putstr0(task_now()->cons,"find while");
		//读入文件内容
        EFI_STATUS status = __fat32_read_file_from_cache(this, cluster_buffer, while_number * cluster_size, cluster_size,this->dnode);
        if (status != EFI_SUCCESS) {
            memman_free_page_64_4m(NULL,cluster_buffer);
            return EFI_DEVICE_ERROR;
        }

        // 查找空目录项
        for (unsigned int i = 0; i < cluster_size; i += 32) {
            if (cluster_buffer[i] == 0x00 || cluster_buffer[i] == 0xE5) {
                free_entry = &cluster_buffer[i];
				index= (i/32) + while_number * (cluster_size/32);
                break;
            }
        }

        if (free_entry)
			break;
		while_number++;
        // 到达当前簇的末尾，获取下一个簇号
        unsigned int next_cluster = this->fat32_fat[current_cluster];
        if (next_cluster >= 0x0FFFFFF8) { // 无法继续，需要分配新簇
            for (unsigned int i = 2; i < mbr->BPB_TotSec32 / mbr->BPB_SecPerClus; i++) {
                if (this->fat32_fat[i] == 0) { // 找到未分配簇
                    this->fat32_fat[i] = 0x0FFFFFFF; // 标记为结束
                    this->fat32_fat[current_cluster] = i; // 链接新簇
                    current_cluster = i;
                    break;
                }
            }

            if (this->fat32_fat[current_cluster] != 0x0FFFFFFF) {
                memman_free_page_64_4m(NULL,cluster_buffer);
                return EFI_OUT_OF_RESOURCES; // 分配失败
            }

            // 新簇数据为空，跳过写入 0 操作
        } else {
            current_cluster = next_cluster;
        }
    }
	cons_putstr0(task_now()->cons,"fat32_create_file_from_cache long name\n");
	unsigned int lfn_entry_count=0;
    // 2. 处理长文件名
	cons_putstr0(task_now()->cons,"fat32_create_file_from_cache short name\n");
    // 填写短文件名
    FAT32_FILE_INFO* sfn_entry = free_entry;
	unsigned int new_node;
	//fat32_find_node_from_fat(this,&new_node);
	//this->fat32_fat[new_node]=0x0fffffff;
	sfn_entry->type=0x20;
	sfn_entry->start_low=0;
	sfn_entry->start_high=0;
	sfn_entry->create_time=0x0030;
	sfn_entry->creat_time_ms=0;
	sfn_entry->create_data=0x4A21;
	sfn_entry->last_found=1;
	sfn_entry->change_time=0x0030;
	sfn_entry->change_data=0x4A21;
	sfn_entry->file_size=0;
	{//FAT32文件名最多8个字符
		int q;
		for(q=0;q<8;q++){
			if(file_name[q]!=0){
				sfn_entry->name[q]=file_name[q];
			}
			else{
				break;
			}
		}
		for(q;q<11;q++){
			sfn_entry->name[q]=' ';
		}
	}
	cons_putstr0(task_now()->cons,"fat32_create_file_from_cache write dir\n");
    // 写入目录簇并同步
    EFI_STATUS status = __fat32_write_file_from_cache(this, cluster_buffer, while_number * cluster_size, cluster_size,this->dnode);
    if (status != EFI_SUCCESS) {
        memman_free_page_64_4m(NULL,cluster_buffer);
        return EFI_DEVICE_ERROR;
    }
	//写入磁盘块并同步
	for(int i=0;i<this->mbr->BPB_NumFATs;i++){
		this->cache->write(this->cache,this->fat32_fat,(this->part_base_lba+this->mbr->BPB_ResvdSecCnt+i*this->mbr->BPB_FATSz32)*512,this->mbr->BPB_FATSz32*512);
	}
    this->cache->sync(this->cache);
	cons_putstr0(task_now()->cons,"fat32_create_file_from_cache flush fat\n");
    // 3. 更新 FAT 表
    this->cache->flush(this->cache);

    memman_free_page_64_4m(NULL,cluster_buffer);
	
	this->findex=index;
	this->fnode=0;
	this->size=0;
    return EFI_SUCCESS;
}

#define MAX_FIS 32
AHCI_SATA_FIS* fis[MAX_FIS];

#define BLOCK_SIZE (4 * 1024 * 1024) // 4MB

#define CACHE_SIZE 16


extern AHCI_TABLE* ahci_table_addr;
FILE_OF_FAT32* fat32_init(FILE_OF_FAT32* file_info,CACHE_TABLE* cache_table ,unsigned int device_id,unsigned long long part_base_lba){
	if(file_info==0){
		file_info=memman_alloc_page_64_4k(NULL);
	
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
		file_info->size=0;//文件的大小
		//写一遍测试写功能
		//cache_table->write(cache_table,fat32_addr,(part_base_lba+mbr->BPB_ResvdSecCnt)*512,BPB_FATSz32*512);
		//cache_table->sync(cache_table);
		//cache_table->flush(cache_table);
		
		
		file_info->part_base_lba=part_base_lba;
		file_info->fnode=mbr->BPB_Root;
		file_info->dnode=mbr->BPB_Root;
		file_info->findex=0;
		//注册函数
		file_info->top.fread=fat32_read_file_from_cache;
		file_info->top.fwrite=fat32_write_file_from_cache;
		file_info->top.fsize=fat32_fsize;
		return file_info;
	}
	else{
		
		FILE_OF_FAT32* new_file_info=memman_alloc_page_64_4k(NULL);
		new_file_info->mbr=file_info->mbr;
		new_file_info->fat32_fat=file_info->fat32_fat;
		new_file_info->part_base_lba=part_base_lba;
		new_file_info->fnode=file_info->mbr->BPB_Root;
		new_file_info->dnode=file_info->mbr->BPB_Root;
		new_file_info->findex=0;
		new_file_info->cache=cache_table;
		//注册函数
		new_file_info->top.fread=fat32_read_file_from_cache;
		new_file_info->top.fwrite=fat32_write_file_from_cache;
		new_file_info->top.fsize=fat32_fsize;
		return new_file_info;
	}
	return 0;
}