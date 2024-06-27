#include "bootpack.h"


AHCI_SATA_FIS* fis[32];
extern char* text_buff;
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

void cache_init(CACHE_TABLE** this){
	*this=memman_alloc_page_64_4m(NULL);
	char* p=*this;
	for(int i=0;i<4*1024*1024;i++){
		p[i]=0;
	}
	return;
}

EFI_STATUS read(CACHE_TABLE *this, void *buff, unsigned long long offset, unsigned long long size) {
    CACHE_TABLE *table = this;
    CACHE_BLOCK* cache_block =0 ;
    unsigned long long cache_block_index = 0;
    int start_index = 0;

	cache_block = table->cache_blocks_point[0];
	cache_block_index = 0;
	
	
    while (size > 0) {
        
		for (int i = start_index; i < table->block_count; i++) {
			cache_block = table->cache_blocks_point[i];
			cache_block_index = i;
			unsigned long long start_lba = offset / 512;
			if (cache_block->lba == start_lba && cache_block->valid == CACHE_BLOCK_ACTIVE) {
				break;
			}
		}
		
		
		//获取本次要读取的块地址和大小
		unsigned long long block_offset = offset % CACHE_SIZE;
		unsigned long long block_base = offset - block_offset;
		unsigned long long copy_size = (CACHE_SIZE - block_offset) < size ? (CACHE_SIZE - block_offset) : size;
		
		
		//sprintf(text_buff," block_base %ld block_offset %ld \n",block_base,block_offset);
		//cons_putstr0(task_now()->cons,text_buff);

        if (cache_block_index < table->block_count && (cache_block->lba *512) == block_base && cache_block->valid == CACHE_BLOCK_ACTIVE) {
            
            unsigned long long copy_base = block_base;
			//sprintf(text_buff," copy_size %ld copy_base %ld \n",copy_size,copy_base);
			//cons_putstr0(task_now()->cons,text_buff);
            asm_memcpy(buff + offset - copy_base, cache_block->buffer + block_offset, copy_size);

            cache_block_index++;
            if (cache_block_index < table->block_count) {
                cache_block = table->cache_blocks_point[cache_block_index];
            }
        } else {

            unsigned long long request_index;
            for (request_index = 0; request_index < MAX_REQUESTS; request_index++) {
                if (table->requests[request_index].valid == CACHE_BLOCK_IDLE) {
                    break;
                }
            }

            REQUEST *req = &table->requests[request_index];
            req->offset = block_offset;
            req->buffer = buff + offset - block_base;
            req->size = copy_size;
            req->valid = CACHE_BLOCK_ACTIVE;
			//sprintf(text_buff," request_count %ld \n",table->request_count);
			//cons_putstr0(task_now()->cons,text_buff);
            table->requests_point[table->request_count++] = req;
			
			//sprintf(text_buff," offset %ld buff %ld  size %ld\n",req->offset,req->buffer,req->size);
			//cons_putstr0(task_now()->cons,text_buff);
			
            if ((cache_block!=0) && cache_block->lba == block_base && cache_block->valid == CACHE_BLOCK_ENABLE) {
                req->cache_blocks_point = cache_block;
                continue;
            }

            unsigned int new_cache_index=0;
            if (table->block_count == MAX_CACHES) {
				int i;
                for (i = 0; i < MAX_CACHES; i++) {
                    if (table->cache_blocks_point[i]->valid == CACHE_BLOCK_ACTIVE) {
                        new_cache_index = table->cache_blocks_point[i]-table->cache_blocks;
                        break;
                    }
                }
				for(;i < MAX_CACHES-1;i++){
					table->cache_blocks_point[i]=table->cache_blocks_point[i+1];
				}
            } else {
                for (int i = 0; i < MAX_CACHES; i++) {
                    if (table->cache_blocks[i].valid == CACHE_BLOCK_IDLE) {
                        new_cache_index = i;
                        break;
                    }
                }
            }

            cache_block = &table->cache_blocks[new_cache_index];
            //cache_block->buffer = memman_alloc_page_64_4m(NULL);
			cache_block->buffer=0;
            cache_block->size = CACHE_SIZE;
            cache_block->lba = block_base / 512;
            cache_block->valid = CACHE_BLOCK_ENABLE;


			//sprintf(text_buff,"cache: lba %ld size %ld\n",cache_block->lba, cache_block->size);
			//cons_putstr0(task_now()->cons,text_buff);
			
            req->cache_blocks_point = cache_block;

            insert_sorted(table->cache_blocks_point, cache_block, table->block_count++);

            if (table->sync_request_count == MAX_SYNC_REQUESTS) {
                sync(this);
            }

            unsigned int sync_index;
            for (sync_index = 0; sync_index < MAX_SYNC_REQUESTS; sync_index++) {
                if (table->sync_requests[sync_index].valid == CACHE_BLOCK_IDLE) {
                    break;
                }
            }

            SYNC_REQUEST *sync_req = &table->sync_requests[sync_index];
            sync_req->cache_blocks_point = cache_block;
            sync_req->valid = CACHE_BLOCK_ENABLE;
            insert_sorted_sync(table->sync_requests_point, sync_req, table->sync_request_count++);

            if (table->sync_request_count == MAX_SYNC_REQUESTS || table->request_count == MAX_REQUESTS || table->block_count == MAX_CACHES) {
                sync(this);
            }
        }
		offset += copy_size;
        size -= copy_size;
    }
    return EFI_SUCCESS;
}

EFI_STATUS sync(CACHE_TABLE *this) {
	
    int fis_index = 0;

    for (int i = 0; i < this->sync_request_count; i++) {
        CACHE_BLOCK* cache_block = this->sync_requests_point[i]->cache_blocks_point;
		//this->sync_requests_point[i]->valid=CACHE_BLOCK_IDLE;
		//this->sync_requests_point[i]->cache_blocks_point->valid=CACHE_BLOCK_IDLE;
		//this->sync_requests_point[i]->valid=CACHE_BLOCK_IDLE;
        unsigned long long lba = cache_block->lba;
        unsigned long long size = cache_block->size;
        unsigned long long buff = (unsigned long long)cache_block->buffer;
		
		//sprintf(text_buff,"ahci fis write: lba %ld size %ld buffer %ld\n",lba,size,buff);
		//cons_putstr0(task_now()->cons,text_buff);
		
        //fis[fis_index] = ahci_make_fis(fis[fis_index], NULL, lba, size / 512, 0x25, 0);
        //ahci_fis_write_prdt(fis[fis_index], 0, buff, size - 1);
        fis_index++;
		cache_block->valid=CACHE_BLOCK_ACTIVE;
    }
	this->sync_request_count=0;
	for(int i=0;i<fis_index;i++){
		//ahci_fis_send(fis+fis_index,);
		//sprintf(text_buff,"ahci fis send  \n");
		//cons_putstr0(task_now()->cons,text_buff);
	}
	int request_count=this->request_count;
    for (int i = 0; i < request_count; i++) {
        REQUEST *req = this->requests_point[i];
        CACHE_BLOCK *cache_block = req->cache_blocks_point;
		
		//sprintf(text_buff,"cache_block->valid  %ld\n",cache_block->valid);
		//cons_putstr0(task_now()->cons,text_buff);
		//sprintf(text_buff,"req  %ld\n",req);
		//cons_putstr0(task_now()->cons,text_buff);
		
        if (cache_block->valid == CACHE_BLOCK_ACTIVE) {
            unsigned long long copy_size = req->size;
            unsigned long long copy_base = cache_block->lba * 512;
            //asm_memcpy(req->buffer, cache_block->buffer+cache_block->offset, copy_size);
			//sprintf(text_buff,"ahci copy: dbuffer %ld sbuffer %ld size %ld\n",req->buffer,cache_block->buffer+req->offset,copy_size);
			//cons_putstr0(task_now()->cons,text_buff);
           
			req->valid=CACHE_BLOCK_IDLE;
			this->request_count--;
			this->requests_point[i]=0;
        }
    }
	{	int i=0 ,j=0;
		while(1){
			for(i;i<request_count && this->requests_point[i]!=0 ;i++);
			for(j=i;j<request_count && this->requests_point[j]==0 ;j++);
			if(i>=request_count || j>=request_count){
				break;
			}
			else{
				//cons_putstr0(task_now()->cons,"this->requests_point[i]=this->requests_point[j]\n");
				this->requests_point[i]=this->requests_point[j];
				this->requests_point[j]=0;
			}
		}
	}
	return EFI_SUCCESS;
}
