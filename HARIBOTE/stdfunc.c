#include "bootpack.h"
#include <stdio.h>
void* memcpy(void* str1 ,void* str2,unsigned long long n){
	asm_memcpy(str1,str2,n);
	return str1;
}


struct malloc_map{ 
	unsigned long long addr;//虚拟地址从哪里开始
	unsigned long long size;//申请了多少字节
}
*malloc_map0=NULL;
unsigned int malloc_map0_mumber=0;
struct malloc_page_map{
	unsigned long long addr;//虚拟地址从哪里开始
	unsigned long long paddr;//物理地址从哪里开始
	unsigned long long use;//使用了了多少字节
	unsigned long long free;//释放的大小
}* malloc_page_map0=NULL;
unsigned int malloc_page_map0_mumber=0;
struct malloc_map* malloc_map_find(unsigned long long addr){
	int i;
	for(i=0;i<malloc_map0_mumber;i++){
		if(addr==malloc_map0[i].addr){
			return malloc_map0+i;
		}
	}
	if(addr==0){
		return malloc_map0+i;
	}
	else{
		return NULL;
	}
}
struct malloc_page_map* malloc_page_map_find(unsigned long long addr){
	int i;
	for(i=0;i<malloc_page_map0_mumber;i++){
		//unsigned long long addr=malloc_page_map0[i].addr;
		//unsigned long long use=malloc_page_map0[i].use;
		if(addr==malloc_page_map0[i].addr){
			return malloc_page_map0+i;
		}
	}
	if(addr==0){
		return malloc_page_map0+i;
	}
	else{
		return NULL;
	}
}
#define MALLOC_MAP_SIZE (4*1024*1024)
void malloc_init(){
	//申请一个4M大小的记录表
	if(malloc_map0==NULL)
		malloc_map0=memman_alloc_page_64_4m(NULL);
	if(malloc_page_map0==NULL)
		malloc_page_map0=memman_alloc_page_64_4m(NULL);
	for(int i=0;i<MALLOC_MAP_SIZE/sizeof(struct malloc_map);i++){
		malloc_map0[i].addr=0;
		malloc_map0[i].size=0;
	}
	for(int i=0;i<MALLOC_MAP_SIZE/sizeof(struct malloc_page_map);i++){
		malloc_page_map0[i].addr=0;
		malloc_page_map0[i].use=0;
		malloc_page_map0[i].free=0;
	}
	malloc_map0_mumber=0;
	malloc_page_map0_mumber=0;
}

#define PAGE_SIZE 4096

// 链表节点结构，用于页内分配管理
typedef struct PageBlock {
    struct PageBlock* next;         // 下一个块的指针
    unsigned long long allocated_size;          // 当前块已分配的大小
    unsigned long long free_count;              // 释放次数
} PageBlock;

// 页内管理链表的头指针
PageBlock* page_list_head = NULL;

// 简单的内存分配函数，根据需要分配内存
void* malloc(unsigned long long size) {
	//printf("malloc\n");
	if(size>4000){
		return malloc_4k(size);
	}
    unsigned long long aligned_size = (size + 7) & ~7;  // 确保分配的内存对齐
    PageBlock* current = page_list_head;

    while (current != NULL) {
        // 检查当前块是否有足够的空间
        if (PAGE_SIZE - current->allocated_size >= aligned_size) {
            // 如果当前块有足够空间，则分配内存
            void* allocated_ptr = (void*)((char*)current + current->allocated_size);
            current->allocated_size += aligned_size;
            current->free_count++;  // 增加分配计数
			//printf("malloc: size %ld p %p\n",size,allocated_ptr);
            return allocated_ptr;
        }
        current = current->next;
    }

    // 如果链表没有合适的空闲块，则分配新的4K页面
    char* new_page = malloc_4k(4096);
    if (new_page == NULL) {
        return NULL;  // 分配失败
    }

    // 创建新页面的块信息
    PageBlock* new_block = (PageBlock*)new_page;
    new_block->next = page_list_head;
    new_block->allocated_size = (sizeof(PageBlock)+ 7) & ~7;  // 先占用管理结构空间
    new_block->free_count = 1;  // 第一次分配

    // 将新块添加到链表头部
    page_list_head = new_block;

    // 在新页面上分配内存
    void* allocated_ptr = (void*)(new_page + new_block->allocated_size);
    new_block->allocated_size += aligned_size;
	//printf("malloc: size %ld p %p\n",size,allocated_ptr);
    return allocated_ptr;
}

// 简单的内存释放函数
void free(void* ptr) {
	//printf("free\n");
    if ((unsigned long long)ptr % PAGE_SIZE == 0) {
        // 如果是4K对齐的地址，直接调用free_4k进行释放
        free_4k(ptr);
        return;
    }

    // 否则在链表中查找对应的块
    PageBlock* current = page_list_head;
	unsigned long long sptr=ptr;
    while (current != NULL) {
        if (sptr >= (unsigned long long)current && sptr < ((unsigned long long)current + PAGE_SIZE)) {
            // 找到了对应的页内块
            current->free_count--;  // 释放次数减1
			//printf("free: count %ld p %p\n",current->free_count,sptr);
            if (current->free_count == 0) {
                // 如果该块的释放次数已达到分配次数，释放该页面
                // 从链表中删除当前块
                if (current == page_list_head) {
                    page_list_head = current->next;
                } else {
                    PageBlock* prev = page_list_head;
                    while (prev->next != current) {
                        prev = prev->next;
                    }
                    prev->next = current->next;
                }
				//释放页面
				free_4k(current);
            }
			return;
        }
        current = current->next;
    }
    // 如果没有找到对应的块，可以选择返回错误或者直接忽略
    printf("Error: Pointer not found in allocated blocks\n");
}

void* realloc(void* p,unsigned long long size){
	if((unsigned long long)p % PAGE_SIZE == 0){
		return realloc_4k(p,size);
	}
	PageBlock* current = page_list_head;
	unsigned long long sptr=p;
	while (current != NULL) {
        if (sptr >= (unsigned long long)current && sptr < ((unsigned long long)current + PAGE_SIZE)) {
            // 找到了对应的页内块
            unsigned old_size=current->allocated_size - (unsigned long long)p;
			if(old_size>size){
				old_size=size;
			}
			char* new_point=malloc(size);
			char* old_point=p;
			for(int i=0;i<old_size;i++){
				new_point[i]=old_point[i];
			}
			free(p);
			return new_point;
        }
        current = current->next;
    }
}

void* malloc_4k(unsigned long long size){
	//printf("malloc 4k\n");
	void* p=NULL;
	p=memman_alloc_4k(task_now()->memman,size);//内存分配!!!
	memman_link_page_64_m(NULL,NULL,p,3,((size)+0xfff)>>12,0);//没有目的链接地址
	for(int i=0;i<MALLOC_MAP_SIZE/sizeof(struct malloc_map);i++){
		//遍历表找到记录空位
		if(malloc_map0[i].addr==0 && malloc_map0[i].size==0){
			malloc_map0[i].addr=p;
			malloc_map0[i].size=size;
			return p;
		}
	}
}

void* realloc_4k(void* p,unsigned long long size){
	//printf("remalloc\n");
	char* np=malloc(size);
	for(int i=0;i<MALLOC_MAP_SIZE/sizeof(struct malloc_map);i++){
		//遍历表找到记录空位
		if(malloc_map0[i].addr==p && malloc_map0[i].size!=0){
			char* from=malloc_map0[i].addr;
			unsigned long long old_size=malloc_map0[i].size;
			for(int j=0;j<old_size;j++){
				np[j]=from[j];
			}
			free(from);
		}
	}
	return np;
}

void free_4k(void *ptr){
	//printf("free 4k\n");
	for(int i=0;i<MALLOC_MAP_SIZE/sizeof(struct malloc_map);i++){
		if(malloc_map0[i].addr==ptr && malloc_map0[i].size!=0){
			memman_free_4k(task_now()->memman,ptr,malloc_map0[i].size);
			memman_unlink_page_64_m(NULL,NULL,ptr,((malloc_map0[i].size)+0xfff)>>12);
			malloc_map0[i].addr=0;
			malloc_map0[i].size=0;
		}
	}
	return;
}

void *memchr(const void *str, int c, unsigned long long n){
	char* s=str;
	char ch=(char)c;
	for(int i=0;i<n;i++){
		if(s[i]==ch){
			return s+i;
		}
	}
	return NULL;
}



void *memmove(void *str1, const void *str2, unsigned long long n){
	unsigned long long si=str1;
	unsigned long long di=str2;
	if(si<di){
		for(int i=0;i<n;i++){
			((char*) str1)[i]=((char*) str2)[i];
		}
	}
	else{
		for(int i=n;i>=0;i--){
			((char*) str1)[i]=((char*) str2)[i];
		}
	}
	return str1;
}

unsigned long long strspn(const char *s, const char *accept){
	char temp[32];
	for(int i=0;i<32;i++){
		temp[i]=0;
	}
	for(int i=0;;i++){
		if(accept[i]==0){
			break;
		}
		else{
			char a=accept[i];
			temp[a/8]|=1<<(a%8);
		}
	}
	for(int i=0;;i++){
		char a=s[i];
		if((temp[a/8])&(1<<(a%8))==0){
			return i;
		}
	}
}

char *strchr(const char *str, int c){
	char ch=c&0xff;
	for(int i=0;;i++){
		if(str[i]==0){
			return NULL;
		}
		else{
			if(str[i]==ch){
				return str+i;
			}
		}
	}
	return NULL;
}

unsigned long long strcspn(const char *str1, const char *str2){
	char temp[32];
	for(int i=0;i<32;i++){
		temp[i]=0;
	}
	for(int i=0;;i++){
		if(str2[i]==0){
			break;
		}
		else{
			char a=str2[i];
			temp[a/8]|=1<<(a%8);
		}
	}
	for(int i=0;;i++){
		char a=str1[i];
		if((temp[a/8])&(1<<(a%8))==0){
			continue;
		}
		else{
			return i;
		}
	}
}

char *strrchr(const char *str, int c){
	char ch=c&0xff;
	char* s=NULL;
	for(int i=0;;i++){
		if(str[i]==0){
			return s;
		}
		else if(str[i]==ch){
			s=str+i;
		}
	}
	return NULL;
}


double fabs(double x){
	if(x<0){
		return -x;
	}
	else{
		return x;
	}
}
int printf(char* format,...){
	va_list ap;
	char* s=memman_alloc_page_64_4k(NULL);
	int i;
	va_start(ap,format);
	i=vsprintf(s,format,ap);
	cons_putstr0(task_now()->cons,s);
	va_end(ap);
	memman_free_page_64_4k(NULL,s);
	return i;
}
unsigned long long strnlen(char* str,long long n){
	int i;
	for(i=0;i<n;i++){
		if(str[i]==0){
			break;
		}
	}
	return i;
}