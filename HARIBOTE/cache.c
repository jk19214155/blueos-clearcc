#include "bootpack.h"


AHCI_SATA_FIS* cache_fis[32];
unsigned long long* cache_fis_data[32];
 char cache_text_buff[64];
void insert_sorted(CACHE_BLOCK* array[], CACHE_BLOCK* new_block, unsigned int count) {
    int i;
    for (i = count - 1; (i >= 0 && array[i]->lba > new_block->lba); i--) {
        array[i + 1] = array[i];
    }
    array[i + 1] = new_block;
}

void insert_sorted_sync(SYNC_REQUEST* array[], SYNC_REQUEST* new_request, unsigned int count) {
    int i;
    for (i = count - 1; (i >= 0 && array[i]->cache_blocks_point->lba > new_request->cache_blocks_point->lba); i--) {
        array[i + 1] = array[i];
    }
    array[i + 1] = new_request;
}

unsigned long long cache_clear(CACHE_TABLE *this){
	
}

EFI_STATUS _cache_block_rel(CACHE_TABLE *this, CACHE_BLOCK **cache_block_point) {
    if (this == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // 遍历 cache_blocks_point 数组，找到目标 cache_block
    for (unsigned int i = 0; i < this->block_count; i++) {
        CACHE_BLOCK *block = this->cache_blocks_point[i];
        
        // 检查是否为 ACTIVE 状态
        if (block->valid == CACHE_BLOCK_ACTIVE) {
            // 释放 block 的 buffer
            if (block->buffer != NULL) {
                memman_free_page_64_4m(NULL,block->buffer);
                block->buffer = NULL;
            }
            
            // 将 block 状态设置为 IDLE
            block->valid = CACHE_BLOCK_IDLE;

            // 将释放的缓存块地址返回
            *cache_block_point = block;
            
            // 将数组中的元素向前移动，填补空位
            for (unsigned int j = i; j < this->block_count - 1; j++) {
                this->cache_blocks_point[j] = this->cache_blocks_point[j + 1];
            }
            
            // 更新 block_count
            this->block_count--;
            
            return EFI_SUCCESS;
        }
    }

    // 如果未找到处于 ACTIVE 状态的 cache_block，返回 NULL
    *cache_block_point = NULL;
    return EFI_NOT_FOUND;
}
//buff:调用者的缓冲区地址 offset:从设备的哪一个偏移开始读取 第二个扇区的100字节视为 612=512+100 size:本次读入大小
EFI_STATUS cache_read(CACHE_TABLE *this, void *buff, unsigned long long offset, unsigned long long size) {
    CACHE_TABLE *table = this;
    CACHE_BLOCK* cache_block = 0;
    char cache_text_buff[128];
    sprintf(cache_text_buff, "DEBUG: cache_read called - this: %ld, buff: %ld, offset: %ld, size: %ld\n", this, buff, offset, size);
    cons_putstr0(task_now()->cons, cache_text_buff);
    
    while (size > 0) {
		int cache_block_index=0;
        // 查找合适的缓存块
        for (cache_block_index = 0; cache_block_index < table->block_count; cache_block_index++) {
            cache_block = table->cache_blocks + cache_block_index;
            unsigned long long start_lba = offset / CACHE_SIZE * ( CACHE_SIZE / 512);//找到本次读取需要对齐的块
            if (cache_block->lba == start_lba && cache_block->valid != CACHE_BLOCK_IDLE) {
                break;
            }
        }
        
        // 计算块内偏移和复制大小
        unsigned long long block_offset = offset % CACHE_SIZE;//本次循环读取的缓存块内偏移量
        unsigned long long block_base = offset - block_offset;//本次循环读取的缓存块所对应设备空间的基地址
        unsigned long long copy_size = (CACHE_SIZE - block_offset) < size ? (CACHE_SIZE - block_offset) : size;//本次复制大小
        
        sprintf(cache_text_buff, "DEBUG: Block info - base: %ld, offset: %ld\n", block_base, block_offset);
        //cons_putstr0(task_now()->cons, cache_text_buff);
		//如果找到对应的缓冲块
        if (cache_block_index < table->block_count) {
            if(cache_block->valid == CACHE_BLOCK_ACTIVE || cache_block->valid == CACHE_BLOCK_DIRTY) {
                // 缓存块活跃，直接复制数据
                sprintf(cache_text_buff, "DEBUG: Copying from cache - size: %ld, base: %ld\n", copy_size, block_base);
                //cons_putstr0(task_now()->cons, cache_text_buff);
                asm_memcpy(buff, cache_block->buffer + block_offset, copy_size);

                cache_block_index++;
                if (cache_block_index < table->block_count) {
                    cache_block = table->cache_blocks_point[cache_block_index];
                }
            } else if(cache_block->valid == CACHE_BLOCK_ENABLE) {
                // 缓存块启用但未活跃，创建请求
                unsigned long long request_index;
                for (request_index = 0; request_index < MAX_REQUESTS; request_index++) {
                    if (table->requests[request_index].valid == CACHE_BLOCK_IDLE) {
                        break;
                    }
                }

                REQUEST *req = &table->requests[request_index];
                req->sbuffer = cache_block->buffer + block_offset;//从哪里复制
                req->dbuffer = buff;//复制到这里
                req->size = copy_size;//复制大小
                req->valid = CACHE_BLOCK_ACTIVE;
                sprintf(cache_text_buff, "DEBUG: New request created - index: %ld, total requests: %ld\n", request_index, table->request_count);
                cons_putstr0(task_now()->cons, cache_text_buff);
                table->request_count++;
                
                sprintf(cache_text_buff, "DEBUG: Request details - sbuffer: %ld, dbuffer: %ld, size: %ld\n", req->sbuffer, req->dbuffer, req->size);
                cons_putstr0(task_now()->cons, cache_text_buff);
                
                req->cache_blocks_point = cache_block;
			}
        } else {
            // 缓存未命中，创建新的缓存块
            cache_block = NULL;
            if (table->block_count == MAX_CACHES) {
                // 缓存已满，释放一个块
                EFI_STATUS status = _cache_block_rel(this, &cache_block);
                if(EFI_ERROR(status)) {
                    sprintf(cache_text_buff, "ERROR: Failed to release cache block - status: %ld\n", status);
                    cons_putstr0(task_now()->cons, cache_text_buff);
                    return status;
                }
            } else {
                // 查找空闲的缓存块
                for (int i = 0; i < MAX_CACHES; i++) {
                    if (table->cache_blocks[i].valid == CACHE_BLOCK_IDLE) {
                        cache_block = table->cache_blocks + i;
                        break;
                    }
                }
            }
            // 分配内存并初始化新的缓存块
            if(cache_block->buffer == NULL)
                cache_block->buffer = memman_alloc_page_64_4m(NULL);
            cache_block->size = CACHE_SIZE;//缓冲块大小
            cache_block->lba = block_base / 512;//本缓存块对应的磁盘lba地址
            cache_block->valid = CACHE_BLOCK_ENABLE;//已创建未同步
            sprintf(cache_text_buff, "DEBUG: New cache block created - lba: %ld, size: %ld\n", cache_block->lba, cache_block->size);
            cons_putstr0(task_now()->cons, cache_text_buff);
            
            // 插入新的缓存块
            insert_sorted(table->cache_blocks_point, cache_block, table->block_count++);
            
            // 创建新的请求
            unsigned long long request_index;
            for (request_index = 0; request_index < MAX_REQUESTS; request_index++) {
                if (table->requests[request_index].valid == CACHE_BLOCK_IDLE) {
                    break;
                }
            }

            REQUEST *req = &(table->requests[request_index]);
            req->cache_blocks_point = cache_block;
            req->sbuffer = cache_block->buffer + block_offset;//从设备同步完成后会从这个位置开始复制
            req->dbuffer = buff;//复制到这里
            req->size = copy_size;//复制大小
            req->valid = CACHE_BLOCK_ACTIVE;
            sprintf(cache_text_buff, "DEBUG: New request created for new cache block - index: %ld, total requests: %ld\n", request_index, table->request_count);
            cons_putstr0(task_now()->cons, cache_text_buff);
			table->request_count++;
            
            sprintf(cache_text_buff, "DEBUG: Request details - sbuffer: %p, dbuffer: %p, size: %ld\n", req->sbuffer, req->dbuffer, req->size);
            cons_putstr0(task_now()->cons, cache_text_buff);

            // 如果达到阈值，执行同步操作
            if (table->request_count == MAX_REQUESTS) {
                this->sync(this);
            }
        }
		offset = ((unsigned long long)offset) + copy_size;
		buff = ((unsigned long long)buff) + copy_size;
        size = ((unsigned long long)size) - copy_size;
    }
    return EFI_SUCCESS;
}
EFI_STATUS cache_write(CACHE_TABLE *this, void *buff, unsigned long long offset, unsigned long long size) {
     CACHE_TABLE *table = this;
    CACHE_BLOCK* cache_block = 0;
    char cache_text_buff[128];
    sprintf(cache_text_buff, "DEBUG: cache_write called - this: %ld, buff: %ld, offset: %ld, size: %ld\n", this, buff, offset, size);
    cons_putstr0(task_now()->cons, cache_text_buff);
    
    while (size > 0) {
		int cache_block_index=0;
        // 查找合适的缓存块
        for (cache_block_index = 0; cache_block_index < table->block_count; cache_block_index++) {
            cache_block = table->cache_blocks + cache_block_index;
            unsigned long long start_lba = offset / CACHE_SIZE * ( CACHE_SIZE / 512);//找到本次读取需要对齐的块
            if (cache_block->lba == start_lba && cache_block->valid != CACHE_BLOCK_IDLE) {
                break;
            }
        }
        
        // 计算块内偏移和复制大小
        unsigned long long block_offset = offset % CACHE_SIZE;//本次循环读取的缓存块内偏移量
        unsigned long long block_base = offset - block_offset;//本次循环读取的缓存块所对应设备空间的基地址
        unsigned long long copy_size = (CACHE_SIZE - block_offset) < size ? (CACHE_SIZE - block_offset) : size;//本次复制大小
        
        sprintf(cache_text_buff, "DEBUG: Block info - base: %ld, offset: %ld\n", block_base, block_offset);
        cons_putstr0(task_now()->cons, cache_text_buff);
		//如果找到对应的缓冲块
        if (cache_block_index < table->block_count) {
            if(cache_block->valid == CACHE_BLOCK_ACTIVE || cache_block->valid == CACHE_BLOCK_DIRTY) {
                // 缓存块活跃，直接复制数据
                sprintf(cache_text_buff, "DEBUG: Copying from cache - size: %ld, base: %ld\n", copy_size, block_base);
                cons_putstr0(task_now()->cons, cache_text_buff);
                asm_memcpy(cache_block->buffer + block_offset,buff, copy_size);
				cache_block->valid = CACHE_BLOCK_DIRTY;	
				
                cache_block_index++;
                if (cache_block_index < table->block_count) {
                    cache_block = table->cache_blocks_point[cache_block_index];
                }
				
            } else if(cache_block->valid == CACHE_BLOCK_ENABLE) {
                // 缓存块启用但未活跃，创建请求
                unsigned long long request_index;
                for (request_index = 0; request_index < MAX_REQUESTS; request_index++) {
                    if (table->requests[request_index].valid == CACHE_BLOCK_IDLE) {
                        break;
                    }
                }

                REQUEST *req = &table->requests[request_index];
                req->dbuffer = cache_block->buffer + block_offset;//复制到这里
                req->sbuffer = buff;//从哪里复制
                req->size = copy_size;//复制大小
                req->valid = CACHE_BLOCK_ACTIVE;
                sprintf(cache_text_buff, "DEBUG: New request created - index: %ld, total requests: %ld\n", request_index, table->request_count);
                cons_putstr0(task_now()->cons, cache_text_buff);
                table->request_count++;
                
                sprintf(cache_text_buff, "DEBUG: Request details - sbuffer: %ld, dbuffer: %ld, size: %ld\n", req->sbuffer, req->dbuffer, req->size);
                cons_putstr0(task_now()->cons, cache_text_buff);
                
                req->cache_blocks_point = cache_block;
			}
        } else {
            // 缓存未命中，创建新的缓存块
            cache_block = NULL;
            if (table->block_count == MAX_CACHES) {
                // 缓存已满，释放一个块
                EFI_STATUS status = _cache_block_rel(this, &cache_block);
                if(EFI_ERROR(status)) {
                    sprintf(cache_text_buff, "ERROR: Failed to release cache block - status: %ld\n", status);
                    cons_putstr0(task_now()->cons, cache_text_buff);
                    return status;
                }
            } else {
                // 查找空闲的缓存块
                for (int i = 0; i < MAX_CACHES; i++) {
                    if (table->cache_blocks[i].valid == CACHE_BLOCK_IDLE) {
                        cache_block = table->cache_blocks + i;
                        break;
                    }
                }
            }
            // 分配内存并初始化新的缓存块
            if(cache_block->buffer == NULL)
                cache_block->buffer = memman_alloc_page_64_4m(NULL);
            cache_block->size = CACHE_SIZE;//缓冲块大小
            cache_block->lba = block_base / 512;//本缓存块对应的磁盘lba地址
            cache_block->valid = CACHE_BLOCK_ENABLE;//已创建未同步
            sprintf(cache_text_buff, "DEBUG: New cache block created - lba: %ld, size: %ld\n", cache_block->lba, cache_block->size);
            cons_putstr0(task_now()->cons, cache_text_buff);
            
            // 插入新的缓存块
            insert_sorted(table->cache_blocks_point, cache_block, table->block_count++);
			if(block_offset == 0 && copy_size == CACHE_SIZE){//如果本次写覆盖了整个缓存块
				asm_memcpy(cache_block->buffer + block_offset,buff, copy_size);
				cache_block->valid = CACHE_BLOCK_DIRTY;//已经写入
				continue;
			}
			else{
				// 创建新的请求
				unsigned long long request_index;
				for (request_index = 0; request_index < MAX_REQUESTS; request_index++) {
					if (table->requests[request_index].valid == CACHE_BLOCK_IDLE) {
						break;
					}
				}
				REQUEST *req = &(table->requests[request_index]);
				req->cache_blocks_point = cache_block;
				req->dbuffer = cache_block->buffer + block_offset;//复制到这里
				req->sbuffer = buff;//从设备同步完成后会从这个位置开始复制
				req->size = copy_size;//复制大小
				req->valid = CACHE_BLOCK_ACTIVE;
				sprintf(cache_text_buff, "DEBUG: New request created for new cache block - index: %ld, total requests: %ld\n", request_index, table->request_count);
				cons_putstr0(task_now()->cons, cache_text_buff);
				table->request_count++;
				sprintf(cache_text_buff, "DEBUG: Request details - sbuffer: %p, dbuffer: %p, size: %ld\n", req->sbuffer, req->dbuffer, req->size);
				cons_putstr0(task_now()->cons, cache_text_buff);

				// 如果达到阈值，执行同步操作
				if (table->request_count == MAX_REQUESTS) {
					this->sync(this);
				}
		    }
		}
		offset = ((unsigned long long)offset) + copy_size;
		buff = ((unsigned long long)buff) + copy_size;
        size = ((unsigned long long)size) - copy_size;
    }
    return EFI_SUCCESS;
}

/*
sync的作用是 保持缓存数据一致性 也就是将CACHE_BLOCK_ENABLE状态的缓存从磁盘读入数据 
而后按照请求列表复制内容到缓存或复制内容到外部内存
*/

EFI_STATUS cache_sync_ahci(CACHE_TABLE *this) {
	
    int fis_index = 0;
    char cache_text_buff[128];
    
    
    for (int i = 0; i < this->block_count; i++) {
        CACHE_BLOCK* cache_block = this->cache_blocks+i;
		if(cache_block->valid==CACHE_BLOCK_ENABLE){
			unsigned long long lba = cache_block->lba;
			unsigned long long size = cache_block->size;
			unsigned long long buff = (unsigned long long)cache_block->buffer;
			
			sprintf(cache_text_buff,"AHCI FIS : LBA %ld, Size %ld bytes, Buffer 0x%lx\n", lba, size, buff);
			cons_putstr0(task_now()->cons,cache_text_buff);
			
			cache_fis[fis_index] = ahci_make_fis(cache_fis[fis_index], NULL, lba, size / 512, ATA_READ_DMA_EX, NULL);
			ahci_fis_write_prdt(cache_fis[fis_index], 0, buff, size - 1);
			cache_fis_data[fis_index]=size;
			ahci_fis_send(this->pci_dev,(this->device_id)&0xff,cache_fis[fis_index],1,0x05);
            fis_index++;
			cache_block->valid=CACHE_BLOCK_ACTIVE;
			//已请求数量达到上限
			if(fis_index == MAX_SYNC_REQUESTS){
				sprintf(cache_text_buff,"Waiting for %ld FIS transfers to complete...\n", fis_index);
				cons_putstr0(task_now()->cons,cache_text_buff);
				for(int i=0;i<fis_index;i++){
					unsigned long long prdbc=*(cache_fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
					if(prdbc==cache_fis_data[i]){
						sprintf(cache_text_buff,"FIS %ld transfer complete: %ld/%ld bytes\n", i, prdbc, cache_fis_data[i]);
						cons_putstr0(task_now()->cons,cache_text_buff);
						continue;
					}
					else{
						//sprintf(cache_text_buff,"FIS %d transfer incomplete: %ld/%ld bytes, retrying...\n", i, prdbc, cache_fis_data[i]);
						//cons_putstr0(task_now()->cons,cache_text_buff);
						//这里看一下读取是否出现错误

						i=-1;
						continue;
					}
				}
				fis_index=0;
			}
		}
    }
	EFI_STATUS status=EFI_SUCCESS;

	sprintf(cache_text_buff,"Waiting for %ld FIS transfers to complete...\n", fis_index);
	//cons_putstr0(task_now()->cons,cache_text_buff);
	
	//Wait for transfer completion
	for(int i=0;i<fis_index;i++){
		unsigned long long prdbc=*(cache_fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
		if(prdbc==cache_fis_data[i]){
			sprintf(cache_text_buff,"FIS %ld transfer complete: %ld/%ld bytes\n", i, prdbc, cache_fis_data[i]);
			cons_putstr0(task_now()->cons,cache_text_buff);
			continue;
		}
		else{
			//sprintf(cache_text_buff,"FIS %d transfer incomplete: %ld/%ld bytes, retrying...\n", i, prdbc, cache_fis_data[i]);
			//cons_putstr0(task_now()->cons,cache_text_buff);
            //这里看一下读取是否出现错误

			i=-1;
			continue;
		}
	}
	
    for (int request_index=0; request_index < MAX_REQUESTS; request_index++) {
		if (this->requests[request_index].valid == CACHE_BLOCK_ACTIVE) {
			REQUEST *req = this->requests+request_index;
			CACHE_BLOCK *cache_block = req->cache_blocks_point;
			
			sprintf(cache_text_buff,"Processing request %ld: Cache block status %ld, Request address 0x%lx\n", request_index, cache_block->valid, (unsigned long long)req);
			cons_putstr0(task_now()->cons,cache_text_buff);
			
			if (cache_block->valid == CACHE_BLOCK_ACTIVE || cache_block->valid == CACHE_BLOCK_DIRTY) {
				unsigned long long copy_size = req->size;
				unsigned long long copy_base_s = (unsigned long long)req->sbuffer;
				unsigned long long copy_base_d = (unsigned long long)req->dbuffer;
				
				sprintf(cache_text_buff,"Memory copy: Source 0x%lx -> Destination 0x%lx, Size %ld bytes\n", copy_base_s, copy_base_d, copy_size);
				cons_putstr0(task_now()->cons,cache_text_buff);
				
				asm_memcpy(copy_base_d, copy_base_s, copy_size);
				if(copy_base_d >= cache_block->buffer && (copy_base_d + copy_size) <= (cache_block->buffer + CACHE_SIZE)){//是向缓存写入
					cache_block->valid = CACHE_BLOCK_DIRTY;//数据被更改
				}
			   
				req->valid=CACHE_BLOCK_IDLE;
				this->request_count--;
				sprintf(cache_text_buff,"Request %ld processed, Remaining requests: %d\n", request_index, this->request_count);
				cons_putstr0(task_now()->cons,cache_text_buff);
			}
		}
    }
	sprintf(cache_text_buff,"Cache synchronization complete, Status: %ld\n", status);
	cons_putstr0(task_now()->cons,cache_text_buff);
	return status;
}

/*
flush的定义是 将所有处于CACHE_BLOCK_DIRTY状态的缓存块写入磁盘
*/

EFI_STATUS cache_flush_ahci(CACHE_TABLE *this) {
	
    int fis_index = 0;
    char cache_text_buff[128];
	sprintf(cache_text_buff, "cache_table:\n");
    cons_putstr0(task_now()->cons, cache_text_buff);
    
    sprintf(cache_text_buff, "block_number: %ld, request_count: %ld\n", 
            this->block_count, this->request_count);
    cons_putstr0(task_now()->cons, cache_text_buff);
    
    sprintf(cache_text_buff, "cache_block:\n");
    cons_putstr0(task_now()->cons, cache_text_buff);
    for (int i = 0; i < this->block_count; i++) {
		CACHE_BLOCK* cache_block=this->cache_blocks + i;
		if(cache_block->valid==CACHE_BLOCK_DIRTY){
			unsigned long long lba = cache_block->lba;
			unsigned long long size = cache_block->size;
			unsigned long long buff = (unsigned long long)cache_block->buffer;
			
			sprintf(cache_text_buff,"AHCI FIS %s: LBA %ld, Size %ld bytes, Buffer 0x%lx\n",
                    "write", lba, size, buff);
			cons_putstr0(task_now()->cons,cache_text_buff);
			
			
			cache_fis[fis_index] = ahci_make_fis(cache_fis[fis_index], NULL, lba, size / 512, ATA_WRITE_DMA_EX, NULL);
			ahci_fis_write_prdt(cache_fis[fis_index], 0, buff, size - 1);
			cache_fis_data[fis_index]=size;
				
			ahci_fis_send(this->pci_dev,(this->device_id)&0xff,cache_fis[fis_index],1,0x05|ahci_command_header$flags$w);
			fis_index++;
			cache_block->valid=CACHE_BLOCK_ACTIVE;
            
			//已请求数量达到上限
			if(fis_index == MAX_SYNC_REQUESTS){
				sprintf(cache_text_buff,"Waiting for %ld FIS transfers to complete...\n", fis_index);
				cons_putstr0(task_now()->cons,cache_text_buff);
				for(int i=0;i<fis_index;i++){
					unsigned long long prdbc=*(cache_fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
					if(prdbc==cache_fis_data[i]){
						sprintf(cache_text_buff,"FIS %ld transfer complete: %ld/%ld bytes\n", i, prdbc, cache_fis_data[i]);
						cons_putstr0(task_now()->cons,cache_text_buff);
						continue;
					}
					else{
						//sprintf(cache_text_buff,"FIS %d transfer incomplete: %ld/%ld bytes, retrying...\n", i, prdbc, cache_fis_data[i]);
						//cons_putstr0(task_now()->cons,cache_text_buff);
						//这里看一下读取是否出现错误

						i=-1;
						continue;
					}
				}
				fis_index=0;
			}
		}
    }
	EFI_STATUS status=EFI_SUCCESS;
	//Wait for transfer completion
	for(int i=0;i<fis_index;i++){
		unsigned long long prdbc=*(cache_fis[i]->cfis.ahci_cfis_0x27.prdbc_addr);
		if(prdbc==cache_fis_data[i]){
			sprintf(cache_text_buff,"FIS %ld transfer complete: %ld/%ld bytes\n", i, prdbc, cache_fis_data[i]);
			cons_putstr0(task_now()->cons,cache_text_buff);
			continue;
		}
		else{
			printf("FIS %ld transfer complete: %ld/%ld bytes\n", i, prdbc, cache_fis_data[i]);
			//sprintf(cache_text_buff,"FIS %d transfer incomplete: %ld/%ld bytes, retrying...\n", i, prdbc, cache_fis_data[i]);
			//cons_putstr0(task_now()->cons,cache_text_buff);
			//这里看一下读取是否出现错误
			i=-1;
			continue;
		}
	}
	sprintf(cache_text_buff,"Cache synchronization complete, Status: %ld\n", status);
	cons_putstr0(task_now()->cons,cache_text_buff);
	return status;
}




void cache_init(CACHE_TABLE** this){
	*this=memman_alloc_page_64_4m(NULL);
	char* p=*this;
	for(int i=0;i<4*1024*1024;i++){
		p[i]=0;
	}
	(*this)->read=cache_read;
	(*this)->sync=cache_sync_ahci;
	(*this)->write=cache_write;
	(*this)->flush=cache_flush_ahci;
	return;
}