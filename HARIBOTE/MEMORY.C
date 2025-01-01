/* メモリ関係 */

#include "bootpack.h"
//#include <stdlib.h>

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000
char buff[128];
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386か、486以降なのかの確認 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386ではAC=1にしても自動で0に戻ってしまう */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* キャッシュ禁止 */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* キャッシュ許可 */
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* あき情報の個数 */
	man->maxfrees = 0;		/* 状況観察用：freesの最大値 */
	man->lostsize = 0;		/* 解放に失敗した合計サイズ */
	man->losts = 0;			/* 解放に失敗した回数 */
	return;
}

unsigned long long memman_total(struct MEMMAN *man)
/* あきサイズの合計を報告 */
{
	unsigned long long i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned long long memman_alloc(struct MEMMAN *man, unsigned long long size)
/* 確保 */
{
	unsigned long long i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 十分な広さのあきを発見 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* free[i]がなくなったので前へつめる */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 構造体の代入 */
				}
			}
			return a;
		}
	}
	return 0; /* あきがない */
}

int memman_free(struct MEMMAN *man, unsigned long long addr, unsigned long long size)
/* 解放 */
{
	int i, j;
	/* まとめやすさを考えると、free[]がaddr順に並んでいるほうがいい */
	/* だからまず、どこに入れるべきかを決める */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前がある */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 前のあき領域にまとめられる */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 後ろもある */
				if (addr + size == man->free[i].addr) {
					/* なんと後ろともまとめられる */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]の削除 */
					/* free[i]がなくなったので前へつめる */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 構造体の代入 */
					}
				}
			}
			return 0; /* 成功終了 */
		}
	}
	/* 前とはまとめられなかった */
	if (i < man->frees) {
		/* 後ろがある */
		if (addr + size == man->free[i].addr) {
			/* 後ろとはまとめられる */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功終了 */
		}
	}
	/* 前にも後ろにもまとめられない */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]より後ろを、後ろへずらして、すきまを作る */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 最大値を更新 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功終了 */
	}
	/* 後ろにずらせなかった */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失敗終了 */
}

struct FREEINFO memman_get_item(struct MEMMAN* man){
	if(man->frees>0){
		return man->free[0];
	}
	else{
		struct FREEINFO info;
		info.size=0;
		info.addr=0;
		return info;
	}
}

unsigned long long memman_alloc_4k(struct MEMMAN *man, unsigned long long size)
{
	unsigned long long a;
	size = (size + 0xfff) & 0xfffffffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned long long addr, unsigned long long size)
{
	int i;
	size = (size + 0xfff) & 0xfffffffff000;
	i = memman_free(man, addr, size);
	return i;
}

void init_page(struct PAGEMAN32 *man){
	int i;
	int j=0;
	unsigned int a;
	int addr_from,addr_to;
	struct MEMINFO* meminfo=(struct MEMINFO*)0x26a000;//ADR_MEMINFO;
	j=0;
	//初始化内核2M二级页表
	for (i=0x400000;i<0x800000;i=i+4){
		//*(int*)i=(j<<12) | 7;//4k页面，G全局访问标志，可读可写,存在标志,用户可以使用
		//j++;
		*(int*)i=0;
	}
	unsigned long long cr3=load_cr3();
	//初始化内核4K一级页表
	j=0;
	for (i=0x268000;i<0x269000-8;i+=8){
		*(unsigned long long*)i=(0x400000+0x1000*j) | 7;//4k页面，可读可写,存在标志,用户可以使用
		j++;
	}
	for(i=0;i<1;i++){
		*((unsigned long long*)0x268000+i)=*((unsigned long long*)cr3+i);
	}
	store_cr3(0x268000);
	//初始化内核紧急页表
	j=0;
	for (i=0x269000;i<0x26a000;i+=4){
		*(int*)i=(j<<22) | (1<<8) | (1<<7) | 7;//4k页面，G全局访问标志，，PS标志，可读可写,存在标志,用户可以使用
		j++;
	}
	man->mem_map_base=(char*)0x00800000;
	//memset(man->mem_map_base,0xff,0x00100000);//所有内存不可用
	//for(i=0;i<0x00100000;i++){
	//	*(((char*)man->mem_map_base)+i)=0xff;
	//}
	man->total_page_num=0;
	man->free_page_num=0;
	for(i=0x800000;i<0x900000;i++){
		if((*(unsigned char*)i)==0){
			man->total_page_num++;
			man->free_page_num++;
		}
	}
	*(unsigned int*)0x0026f004=man->total_page_num;
	return;
}


unsigned int memmam_link_page_32_m(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address,unsigned int physical_address,int page_num,int mode){
	int i;
	sprintf(buff,"memman_alloc: alloc page num %d linear_address: %x physical_address: %x\n",page_num,linear_address,physical_address);
	com_out_string(0x3f8,buff);
	if(mode==0){//没有要链接的页面
		for(i=0;i<page_num;i++){
			memman_link_page_32(man,cr3_address,linear_address+i*0x1000,physical_address,mode);
		}
	}
	else{
		for(i=0;i<page_num;i++){
			memman_link_page_32(man,cr3_address,linear_address+i*0x1000,physical_address+i*0x1000,mode);
		}
	}
	*(int*)0x0026f00c=man->free_page_num;
	return physical_address;
}

unsigned int memman_link_page_32(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address,unsigned int physical_address,int mode){
	int index,addr;
	index=(linear_address>>20)&0xffc;
	addr=index+cr3_address;
	if((*(int*)addr)&1==0){//要链接的目标页面不存在
		*(int*)addr=memman_alloc_page_32(man)|7;
	}
	addr=(*(int*)addr)&0xfffff000;//获取页表的地址
	index=((linear_address>>10)&0xffc);//保留中间10位索引
	addr=addr+index;
	if(mode==0){//没有想要链接的地址
		physical_address=(physical_address&0xfff)|(memman_alloc_page_32(man));
		*(int*)addr=physical_address;
		return physical_address;
	}
	*(int*)addr=physical_address;
	return *(int*)addr;
}


unsigned int memman_unlink_page_32(struct PAGEMAN32 *man,unsigned int cr3_address,unsigned int linear_address){
	if(linear_address>=0xfffff000){
		return 0;
	}
	int index,addr;
	unsigned int res;
	index=(linear_address>>20)&0xffc;
	addr=index+cr3_address;
	if((*(int*)addr)&1==0){//要链接的目标页面不存在
		//*(int*)addr=memman_alloc_page_32(man)|7;
		return -1;//不用释放
	}
	addr=(*(int*)addr)&0xfffff000;//获取页表的地址
	index=((linear_address>>10)&0xffc);//保留中间10位索引
	addr=addr+index;
	res=*(unsigned int*)addr;
	*(int*)addr=0;//取消链接
	//memman_free_page_32(man,*(unsigned int*)addr);//释放内存页
	return res;
}

unsigned int pageman_link_page_32_m(struct PAGEMAN32 *man,unsigned int linear_address,unsigned int physical_address,int page_num,int mode){
	io_cli();
	int i;
	if(mode==0){//没有要链接的页面
		for(i=0;i<page_num;i++){
			pageman_link_page_32(man,linear_address+i*0x1000,physical_address,mode);
		}
	}
	else{
		for(i=0;i<page_num;i++){
			pageman_link_page_32(man,linear_address+i*0x1000,physical_address+i*0x1000,mode);
		}
	}
	*(int*)0x0026f00c=man->free_page_num;
	i=load_cr3();
	store_cr3(i);
	io_sti();
	return physical_address;
}

/*经过此函数连接的内存，保证其物理内存连续性*/
unsigned int pageman_link_page_32_mf(struct PAGEMAN32* man, unsigned int linear_address,unsigned int physical_address,int page_num){
	int num=0;
	int start=0;
	for(int i=0;i<0x100000;i++){
		if(man->mem_map_base[i]==0){
			if(num==0){//开始
				start=i;
			}
			num++;
			if(num==page_num){//连续页数量符合要求
				/*进行连接*/
				break;
			}
			else{
				
			}
		}
		else{
			num=0;
			continue;
		}
	}
	if(num==page_num){
		for(int i=0;i<page_num;i++){
			pageman_link_page_32(man,linear_address+0x1000*i,(start+i)*0x1000,9999);
			man->mem_map_base[i]=1;
		}
		return 0;
	}
	else{//连接失败
		return -1;
	}
}

unsigned int pageman_link_page_32(struct PAGEMAN32 *man,unsigned int linear_address,unsigned int physical_address,int mode){
	int addr,i;
	addr=0xfffff000|((linear_address>>20)&0xfffffffc);
	if(((*(int*)addr)&1)==0){//要链接的目标页面不存在
		*(int*)addr=memman_alloc_page_32(man)|7;
		addr=(0xffc00000|((linear_address>>10)&0xfffffffc))&0xfffff000;
		for(i=0;i<4096;i++){
			*(char*)addr=0;
		}
		addr=0xffc00000|((linear_address>>10)&0xfffffffc);
	}
	else{
		addr=0xffc00000|((linear_address>>10)&0xfffffffc);
	}
	if(mode==0){//没有想要链接的地址
		physical_address=(physical_address&0xfff)|(memman_alloc_page_32(man));
		*(int*)addr=physical_address;
		return *(int*)addr;
	}
	*(int*)addr=physical_address;
	return *(int*)addr;
}

unsigned int get_physical_by_linear_32(unsigned int linear_address){
	unsigned int addr=0xffc00000|((linear_address>>10)&0xfffffffc);
	return *(unsigned int*)addr;
}

unsigned int pageman_unlink_page_32_m(struct PAGEMAN32 *man,unsigned int linear_address,int page_num,int mode){
	int i,addr;
	for(i=page_num-1;i>=0;i--){
		pageman_unlink_page_32(man,linear_address+i*0x1000,mode);
		//addr=0xfffff000|((linear_address>>20)&0xfffffffc);
	}
	if(mode&1){//看一下一级页表是否已经空
		int addr_from=(linear_address)&0xffc00000;//1M对齐
		int addr_to=((linear_address+page_num*0x1000-0x1000)+0xcfffff)&0xffc00000;//1M对齐
		for(i=addr_to;i>=addr_from;i-=0x400000){
			addr=0xfffff000|((linear_address>>20)&0xfffffffc);
			if(((*(int*)addr)&1)==0){//要链接的目标页面不存在
				continue;
			}
			addr=0xffc00000|((linear_address>>10)&0xfffffffc);
			int j;
			for(j=0;j<0x1000;j+=4){
				if((*(int*)addr+j)&1){//只要有一个页存在就不释放一级页表
					break;
				}
			}
			if(j<0x1000){
				continue;//不释放
			}
			else{
				addr=0xfffff000|((linear_address>>20)&0xfffffffc);
				memman_free_page_32(man,*(int*)addr);
				*(int*)addr=0;
			}
		}
	}
	*(int*)0x0026f00c=man->free_page_num;
	
	return 0;
}


unsigned int pageman_unlink_page_32(struct PAGEMAN32 *man,unsigned int linear_address,int mode){
	if(linear_address>=0xfffff000){
		io_cli();
		for(;;);
		return 0;
	}
	int addr,i,res;
	addr=0xfffff000|((linear_address>>20)&0xfffffffc);
	if(((*(int*)addr)&1)==0){//要链接的目标页面不存在
		return 0;
	}
	addr=0xffc00000|((linear_address>>10)&0xfffffffc);
	res=*(int*)addr;
	*(int*)addr=0;
	if(mode&1){
		memman_free_page_32(man,res);
	}
	return res;
}

unsigned int memman_alloc_page_32(struct PAGEMAN32 *man){
	int i;
	for(i=0;i<0x100000;i++){
		if(man->mem_map_base[i]==0){
			man->mem_map_base[i]=1;//已使用
			man->free_page_num--;
			if(task_now()!=0){//记录内存使用情况
				task_now()->mem_use++;
			}
			return i*4096;//返回物理地址
		}
	}
	*(int*)0x0026f010=man->free_page_num;//记录最后一次内存请求失败的时候的free值
	for(;;){
		io_cli();
		io_hlt();
	}
	return -1;//没有找到可用的物理内存
}


#define memman_alloc_page_64 memman_alloc_page_64_4k

unsigned long long memman_alloc_page_64_4k(struct PAGEMAN32 *man){
	if(man==0){
		man=pageman_get();
	}
	unsigned long long i;
	for(i=0;i<0x100000;i++){
		if(man->mem_map_base[i]==0){
			man->mem_map_base[i]=1;//已使用
			man->free_page_num--;
			if(task_now()!=0){//记录内存使用情况
				task_now()->mem_use++;
			}
			for(int j=0;j<4096;j++){
				*(char*)(i*4096+j)=0;
			}
			return i*4096;//返回物理地址
		}
	}
	*(int*)0x0026f010=man->free_page_num;//记录最后一次内存请求失败的时候的free值
	for(;;){
		io_cli();
		io_hlt();
	}
	return -1;//没有找到可用的物理内存
}

void memman_free_page_64_4k(struct PAGEMAN32 *man,unsigned long long addr){
	if(man==0){
		man=pageman_get();
	}
	man->mem_map_base[addr/4096]==0;
	task_now()->mem_use--;
	man->free_page_num++;
	return 0;
}

unsigned long long memman_alloc_page_64_4m(struct PAGEMAN32 *man){
	if(man==0){
		man=pageman_get();
	}
	unsigned long long i,j;
	for(i=0;i<0x100000;i+=1024){
		for(j=0;j<1024;j++){
			if(man->mem_map_base[i+j]!=0){
				break;
			}
		}
		if(j==1024){
			for(j=0;j<1024;j++){
				man->mem_map_base[i+j]==1;
			}
			man->free_page_num-=1024;
			if(task_now()!=0){//记录内存使用情况
				task_now()->mem_use+=1024;
			}
			return i*4096;//返回物理地址
		}
	}
	*(int*)0x0026f010=man->free_page_num;//记录最后一次内存请求失败的时候的free值
	for(;;){
		io_cli();
		io_hlt();
	}
	return -1;//没有找到可用的物理内存
}

void memman_free_page_64_4m(struct PAGEMAN32 *man,unsigned long long addr){
	if(man==0){
		man=pageman_get();
	}
	unsigned long long i,j;
	for(i=(addr/4096);i<1024;i+=1024){
		man->mem_map_base[i]=0;
	}
	man->free_page_num+=1024;
	task_now()->mem_use-=1024;
	*(int*)0x0026f010=man->free_page_num;//记录最后一次内存请求失败的时候的free值
	return 0;
}

unsigned int memman_free_page_32_m(struct PAGEMAN32 *man,unsigned int physical_address,int page_num){
	if(man==0){
		man=pageman_get();
	}
	int index=physical_address>>12;//得到?的索引
	int i;
	for(i=0;i<page_num;i++){
		if(man->mem_map_base[index+i]==1){//正在使用
			man->free_page_num++;
			if(task_now()!=0){//记录内存使用情况
				task_now()->mem_use--;
			}
		}
		man->mem_map_base[index+i]=0;//未使用;
	}
	return 0;
}
unsigned int memman_free_page_32(struct PAGEMAN32 *man,unsigned int physical_address){
	if(man==0){
		man=pageman_get();
	}
	int index=physical_address>>12;//得到?的索引
	if(man->mem_map_base[index]==1){//正在使用
		man->free_page_num++;
		if(task_now()!=0){//记录内存使用情况
			task_now()->mem_use--;
		}
	}
	man->mem_map_base[index]=0;//未使用;
	*(int*)0x0026f00c=man->free_page_num;
	return 0;
}

void memman_copy_page_32_m(struct PAGEMAN32 *man,unsigned int cr3_address_s,int cr3_address_d,unsigned int linear_address_s,unsigned int linear_address_d,int num){
	int addr_s,addr_d;
	int i;
	if(man==0){
		man=pageman_get();
	}
	for(i=0;i<num;i++){
		int index,addr;
		index=((linear_address_s+i<<12)>>20)&0xffc;
		addr=index+cr3_address_s;
		if((*(int*)addr)&1==0){//要链接的目标页面不存在
			*(int*)addr=memman_alloc_page_32(man)|7;
		}
		addr=(*(int*)addr)&0xfffff000;//获取页表的地址
		index=(((linear_address_s+i<<12)>>10)&0xffc);//保留中间10位索引
		addr_s=addr+index;
		
		
		index=((linear_address_d+i<<12)>>20)&0xffc;
		addr=index+cr3_address_d;
		if((*(int*)addr)&1==0){//要链接的目标页面不存在
			*(int*)addr_s=0;
			continue;
		}
		addr=(*(int*)addr)&0xfffff000;//获取页表的地址
		index=(((linear_address_d+i<<12)>>10)&0xffc);//保留中间10位索引
		addr_d=addr+index;
		
		*(int*)addr_d=*(int*)addr_s;
	}
	return;
}

int inthandler0e(int cr2,int* esp){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	if(*esp&1){//缺页故障
		memman_link_page_32(pageman,(task_now()->tss).cr3,cr2&0xfffff000,7,0);
		return 0;
	}
	else{
		for(;;){
			io_cli();
			io_hlt();
		}
	}
	return 0;
}

unsigned long long memman_link_page_64(struct PAGEMAN32* pageman,unsigned long long cr3_address,unsigned long long linear_address,unsigned long long physical_address,unsigned int mode){
	unsigned long long index,addr;
	unsigned long long mask=0xff8;
	unsigned long long mask2=0xfff;
	index=(linear_address>>36)&mask;
	addr=index+(cr3_address&0xfffffffffffff000);
	if(pageman==0){
		pageman=pageman_get();
	}
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		*(unsigned long long*)addr=memman_alloc_page_64(pageman)|3;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>27)&mask);//保留中间10位索引
	addr=addr+index;
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		*(unsigned long long*)addr=memman_alloc_page_64(pageman)|3;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>18)&mask);//保留中间10位索引
	addr=addr+index;
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		*(unsigned long long*)addr=memman_alloc_page_64(pageman)|3;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>9)&mask);//保留中间10位索引
	addr=addr+index;
	if(mode==0){//没有想要链接的地址
		physical_address=(physical_address&mask2)|(memman_alloc_page_64(pageman));
		*(unsigned long long*)addr=physical_address;
		return physical_address;
	}
	*(unsigned long long*)addr=physical_address;
	return *(unsigned long long*)addr;
}

unsigned long long memman_unlink_page_64(struct PAGEMAN32* pageman, unsigned long long cr3_address, unsigned long long linear_address) {
    
	
	unsigned long long index, addr;
    unsigned long long mask = 0xff8;
    unsigned long long mask2 = 0xfff;

    // 计算 PML4 索引
    index = (linear_address >> 36) & mask;
    addr = index + (cr3_address & 0xfffffffffffff000);

    if (pageman == 0) {
        pageman = pageman_get();
    }

    // 获取 PDPT 地址并检查有效性
    if ((*(unsigned long long*)addr & 1) == 0) {
        return 1;  // 目标页表不存在，直接返回
    }
    addr = (*(unsigned long long*)addr) & 0xfffffffffffff000;

    // 计算 PDPT 索引并获取 PDT 地址
    index = (linear_address >> 27) & mask;
    addr = addr + index;
    if ((*(unsigned long long*)addr & 1) == 0) {
        return 1;  // 目标页表不存在，直接返回
    }
    addr = (*(unsigned long long*)addr) & 0xfffffffffffff000;

    // 计算 PDT 索引并获取 PT 地址
    index = (linear_address >> 18) & mask;
    addr = addr + index;
    if ((*(unsigned long long*)addr & 1) == 0) {
        return 1;  // 目标页表不存在，直接返回
    }
    addr = (*(unsigned long long*)addr) & 0xfffffffffffff000;

    // 计算 PT 索引并找到最终的物理页地址
    index = (linear_address >> 9) & mask;
    addr = addr + index;
    if ((*(unsigned long long*)addr & 1) == 0) {
        return 1;  // 物理页不存在，无需回收
    }

    // 释放最终的物理页
    unsigned long long physical_address = (*(unsigned long long*)addr & 0xfffffffffffff000);
    memman_free_page_64_4k(pageman, physical_address);
    *(unsigned long long*)addr = 0;

    // 检查是否可以释放 PT 页表
    char is_empty = 1;
    for (int i = 0; i < 512; i++) {
        if (((unsigned long long*)addr)[i] == 0) {
            is_empty = 0;
            break;
        }
    }
    if (is_empty) {
        memman_free_page_64_4k(pageman, addr);
        addr = (linear_address >> 18) & mask;
        *(unsigned long long*)addr = 0;

        // 检查并释放 PDT 页表
        is_empty = 1;
        for (int i = 0; i < 512; i++) {
            if (((unsigned long long*)addr)[i] == 0) {
                is_empty = 0;
                break;
            }
        }
        if (is_empty) {
            memman_free_page_64_4k(pageman, addr);
            addr = (linear_address >> 27) & mask;
            *(unsigned long long*)addr = 0;

            // 检查并释放 PDPT 页表
            is_empty = 1;
            for (int i = 0; i < 512; i++) {
                if (((unsigned long long*)addr)[i] == 0) {
                    is_empty = 0;
                    break;
                }
            }
            if (is_empty) {
                memman_free_page_64_4k(pageman, addr);
                addr = (linear_address >> 36) & mask;
                *(unsigned long long*)addr = 0;

                // 检查并释放 PML4 页表
                is_empty = 1;
                for (int i = 0; i < 512; i++) {
                    if (((unsigned long long*)addr)[i] == 0) {
                        is_empty = 0;
                        break;
                    }
                }
                if (is_empty) {
                    memman_free_page_64_4k(pageman, addr);
                }
            }
        }
    }

    return 0; // 回收成功
}


unsigned long long memman_link_page_64_4m(struct PAGEMAN32* pageman,unsigned long long cr3_address,unsigned long long linear_address,unsigned long long physical_address,unsigned int mode){
	unsigned long long index,addr;
	unsigned long long mask=0xff8;
	unsigned long long mask2=0xfff;
	index=(linear_address>>36)&mask;
	addr=index+(cr3_address&0xfffffffffffff000);
	if(pageman==0){
		pageman=pageman_get();
	}
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		*(unsigned long long*)addr=memman_alloc_page_64(pageman)|3;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>27)&mask);//保留中间10位索引
	addr=addr+index;
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		*(unsigned long long*)addr=memman_alloc_page_64(pageman)|3;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>18)&mask);//保留中间10位索引
	addr=addr+index;
	if(mode==0){//没有想要链接的地址
		physical_address=(physical_address&mask2)|(memman_alloc_page_64_4m(pageman));
		*(unsigned long long*)addr=physical_address;
		return physical_address;
	}
	*(unsigned long long*)addr=physical_address;
	return *(unsigned long long*)addr;
}

unsigned int memman_link_page_64_m(struct PAGEMAN32 *man,unsigned long long cr3_address,unsigned long long linear_address,unsigned long long physical_address,int page_num,int mode){
	io_cli();
	unsigned long long i;
	if(man==0){
		man=pageman_get();
	}
	if(cr3_address==NULL){
		cr3_address=load_cr3();
	}
	if(mode==0){//没有要链接的页面
		for(i=0;i<page_num;i++){
			memman_link_page_64(man,cr3_address,linear_address+i*0x1000,physical_address,mode);
		}
	}
	else{
		for(i=0;i<page_num;i++){
			memman_link_page_64(man,cr3_address,linear_address+i*0x1000,physical_address+i*0x1000,mode);
		}
	}
	i=load_cr3();
	store_cr3(i);
	io_sti();
	return physical_address;
}

unsigned int memman_unlink_page_64_m(struct PAGEMAN32 *man,unsigned long long cr3_address,unsigned long long linear_address,int page_num){
	return 0;
	io_cli();
	unsigned long long i;
	if(man==0){
		man=pageman_get();
	}
	if(cr3_address==NULL){
		cr3_address=load_cr3();
	}
	if(1){
		for(i=0;i<page_num;i++){
			memman_unlink_page_64(man,cr3_address,linear_address+i*0x1000);
		}
	}
	else{
		for(i=0;i<page_num;i++){
			memman_unlink_page_64(man,cr3_address,linear_address+i*0x1000);
		}
	}
	i=load_cr3();
	store_cr3(i);
	io_sti();
	return 0;
}

unsigned long long pageman_get_physical_address(unsigned long long cr3_address,unsigned long long linear_address){
	unsigned long long index,addr;
	unsigned long long mask=0xff8;
	unsigned long long mask2=0xfff;
	index=(linear_address>>36)&mask;
	addr=index+(cr3_address&0xfffffffffffff000);
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		return -1;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>27)&mask);//保留中间10位索引
	addr=addr+index;
	if(((*(unsigned long long*)addr)&1)==0){//要链接的目标页面不存在
		return -1;
	}
	addr=(*(unsigned long long*)addr)&0xfffffffffffff000;//获取页表的地址
	index=((linear_address>>18)&mask);//保留中间10位索引
	addr=addr+index;
	return *(unsigned long long*)addr;
}
struct PAGEMAN32* global_pageman_point;
void pageman_set(struct PAGEMAN32* pageman){
	global_pageman_point=pageman;
}
void* pageman_get(){
	return global_pageman_point;
}

// EFI AllocatePool with Physical Address Linking
unsigned long long EFI_AllocatePool(EFI_MEMORY_TYPE PoolType, unsigned long long Size, void** Buffer) {
    if (Buffer == NULL || Size == 0) {
        return EFI_INVALID_PARAMETER; // 参数无效
    }

    // 4K 向上对齐分配大小
    Size = (Size + 0xFFF) & ~0xFFF;

    // 获取当前任务的内存管理器
    struct MEMMAN *memman = task_now()->memman;
    if (memman == NULL) {
        return EFI_OUT_OF_RESOURCES; // 内存资源不足
    }

    // 分配线性地址空间
    unsigned long long linear_address = memman_alloc_4k(memman, Size);
    if (linear_address == 0) {
        return EFI_OUT_OF_RESOURCES; // 分配失败，资源不足
    }

    // 连接物理地址，确保分配的线性地址有效
    unsigned long long cr3_address = task_now()->tss.cr3;  // 获取当前任务的CR3地址
    unsigned long long physical_address = memman_alloc_page_64(memman); // 分配物理页地址
    if (physical_address == 0) {
        // 回收线性地址并返回错误
        memman_free_4k(memman, linear_address, Size);
        return EFI_OUT_OF_RESOURCES;
    }

    // 链接线性地址和物理地址
    memman_link_page_64_m(memman, cr3_address, linear_address, physical_address, Size/4096,0);

    *Buffer = (void*)linear_address;
	if(Size!=4096){//如果只有一个页面申请那么不记录
		EFI_FreePool_set_size(Buffer,Size);
	}
    return EFI_SUCCESS;
}

struct _EFI_FreePool_table_struct{
	void* this;
	UINT64 size_of_this;
	UINT64 item_number;
	struct item_struct{
		void* buff;
		UINT64 size;
	}item[0];
}*_EFI_FreePool_table=0;

unsigned long long EFI_FreePool(void* Buffer);

void EFI_FreePool_set_size(void* Buffer,UINT64 size){
	if(size==4096){
		return;
	}
	if(_EFI_FreePool_table==0){
		EFI_AllocatePool(0, 4096, &_EFI_FreePool_table);
		_EFI_FreePool_table->this=_EFI_FreePool_table;
		_EFI_FreePool_table->size_of_this=4096;
		_EFI_FreePool_table->item_number=0;
	}
	else if(_EFI_FreePool_table->this==0){
		asm_memcpy(Buffer,_EFI_FreePool_table,_EFI_FreePool_table->size_of_this);//复制内容
		EFI_FreePool(_EFI_FreePool_table);//释放原本的内存
		_EFI_FreePool_table=Buffer;
		_EFI_FreePool_table->size_of_this=size;
		_EFI_FreePool_table->this=_EFI_FreePool_table;
		return;
	}
	
	UINT32 size_1=sizeof(struct _EFI_FreePool_table_struct);
	UINT32 size_2=sizeof(_EFI_FreePool_table->item[0]);
	
	UINT32 size_3=_EFI_FreePool_table->item_number*size_2+size_1;
	if(size_3>_EFI_FreePool_table->size_of_this){//如果超过了大小上限
		_EFI_FreePool_table->this=0;
		EFI_AllocatePool(0, size_3, &_EFI_FreePool_table);
	}
	_EFI_FreePool_table->item[_EFI_FreePool_table->item_number].buff=Buffer;
	_EFI_FreePool_table->item[_EFI_FreePool_table->item_number].size=size;
	
	
	_EFI_FreePool_table->item_number++;
	return;
}

UINT64 EFI_FreePool_get_size(void* Buffer){
	int i;
	if(Buffer==_EFI_FreePool_table){
		return _EFI_FreePool_table->size_of_this;
	}
	for(i=0;i<_EFI_FreePool_table->item_number;i++){
		if(_EFI_FreePool_table->item[i].buff==Buffer){
			break;
		}
	}
	if(i<_EFI_FreePool_table->item_number){//没找到
		return _EFI_FreePool_table->item[i].size;
	}
	else{
		return 4096;
	}
}

// EFI FreePool with Physical Address Unlinking
unsigned long long EFI_FreePool(void* Buffer) {
    if (Buffer == NULL) {
        return EFI_INVALID_PARAMETER; // 参数无效
    }

    // 获取当前任务的内存管理器
    struct MEMMAN *memman = task_now()->memman;
    if (memman == NULL) {
        return EFI_NOT_FOUND; // 当前任务无有效内存管理器
    }

    unsigned long long linear_address = (unsigned long long)Buffer;
    unsigned long long cr3_address = task_now()->tss.cr3;

    // 取消链接物理地址
	UINT64 pagenum=EFI_FreePool_get_size(Buffer)/4096;
    unsigned long long result = memman_unlink_page_64_m(memman, cr3_address, linear_address,pagenum);
    if (result != 0) {
        return EFI_NOT_FOUND; // 未找到链接的页表
    }

    // 释放线性地址空间
    memman_free_4k(memman, linear_address, 0x1000);
    return EFI_SUCCESS;
}
