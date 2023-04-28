/* ファイル関係 */

#include "bootpack.h"

int _file_dmg_read2(char* buff,int lba28,unsigned char block_number,int size){
	struct TASK* task=task_now();
	//char s[30];
	int i,j;
	//sprintf(s,"addr is%x\n",buff);
	//cons_putstr0(task->cons, s);
	//sprintf(s,"size is%x\n",size);
	//cons_putstr0(task->cons, s);
	//sprintf(s,"block_number is%x\n",block_number);
	//cons_putstr0(task->cons, s);
	//sprintf(s,"lba28 is%x\n\n",lba28);
	//cons_putstr0(task->cons, s);
	io_out8(0x1f2,block_number);//写入要?取的数量
	io_out8(0x1f3,(lba28>>0)&0xff);//7-0
	io_out8(0x1f4,(lba28>>8)&0xff);//15-8
	io_out8(0x1f5,(lba28>>16)&0xff);//23-16
	io_out8(0x1f6,(lba28>>24)&0xff|0xe0);//27-24
	io_out8(0x1f7,0x20);//?数据
	for(i=0;i<block_number;i++){
		for(;;){//循?等待硬?准?就?,注意?个扇区?取完成后都需要等待就位
			unsigned char data=io_in8(0x1f7);
			*(unsigned char*)0x0026f02c=data;
			data&=0x88;
			if(data==0x08){
				break;
			}
		}
		if(size<512){
			for(j=0;j<256;j++){
				int data=io_in16(0x1f0);
				buff[i*512+j*2]=data&0xff;
				size--;
				if(size==0)
					break;
				buff[i*512+j*2+1]=(data>>8)&0xff;
				size--;
				if(size==0)
					break;
			}
			//?完剩下的数据
			for(j;j<256;j++){
				int data=io_in16(0x1f0);
			}
		}
		else{
			for(j=0;j<256;j++){
				int data=io_in16(0x1f0);
				buff[i*512+j*2]=data&0xff;
				buff[i*512+j*2+1]=(data>>8)&0xff;
			}
			size-=512;
		}
	}
	return 0;
}

unsigned int _file_read_file(char* buff,unsigned int* size,unsigned int fat32_addr,unsigned part_base_lba,FAT32_HEADER* mbr,unsigned int start_lba_low,unsigned int start_lba_high){
	unsigned int i;
	unsigned size2=*size;
	for(i=0;;i++){
		if(size2>(mbr->BPB_SecPerClus*512)){//可以读完一个簇
		//扇区位置:分区基地址+保留分区+FAT所占的分区*FAT数量+(簇号-2)*簇大小
			_file_dmg_read2(buff+i*512*(mbr->BPB_SecPerClus),part_base_lba+mbr->BPB_ResvdSecCnt+(mbr->BPB_FATSz32)*(mbr->BPB_NumFATs)+(start_lba_low-2)*(mbr->BPB_SecPerClus),mbr->BPB_SecPerClus,mbr->BPB_SecPerClus*512);
			size2-=mbr->BPB_SecPerClus*512;
		}
		else{
			_file_dmg_read2(buff+i*512*(mbr->BPB_SecPerClus),part_base_lba+mbr->BPB_ResvdSecCnt+(mbr->BPB_FATSz32)*(mbr->BPB_NumFATs)+(start_lba_low-2)*(mbr->BPB_SecPerClus),(size2+511)/512,size2);
			break;
		}
		start_lba_low=((unsigned int*)fat32_addr)[start_lba_low];
		if(start_lba_low==0x0fffffff){
			break;
		}
	}
	return 0;
}

void file_readfat(int *fat, unsigned char *img)
/* ディスクイメージ内のFATの圧縮をとく */
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}
	return;
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
	int i;
	_file_read_file(buf,&size,fat,*(unsigned int*)0x0026f030,*(unsigned int*)0x0026f024,clustno,0);
	return;
	for (;;) {
		if (size <= 512) {
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		if (j >= 11) { return 0; /* 見つからなかった */ }
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			s[j] = name[i];
			if ('a' <= s[j] && s[j] <= 'z') {
				/* 小文字は大文字に直す */
				s[j] -= 0x20;
			} 
			j++;
		}
	}
	for (i = 0; i < max; ) {
		if (finfo->name[0] == 0x00) {
			break;
		}
		if ((finfo[i].type & 0x18) == 0) {
			for (j = 0; j < 11; j++) {
				if (finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			return finfo + i; /* ファイルが見つかった */
		}
next:
		i++;
	}
	return 0; /* 見つからなかった */
}

struct FILEINFO *file_full_search(char *name, struct FILEINFO *finfo, unsigned char type_have ,unsigned char type_havnt ,int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}
	int k = 0;
	for (i = 0; name[i] != 0; i++) {
		s[k] = name[i];
		if ('a' <= s[k] && s[k] <= 'z') {
			/* 小文字は大文字に直す */
			s[k] -= 0x20;
		} 
		k++;
	}
	for (i = 0; i < max; ) {
		if (finfo->name[0] == 0x00) {
			break;
		}
		if ((finfo[i].type & type_have) == type_have && (finfo[i].type & type_havnt) == 0) {//包含必要?志位并且不含有禁止的?志位
			for (j = 0; j < k; j++) {
				if (finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			return finfo + i; /* ファイルが見つかった */
		}
next:
		i++;
	}
	return 0; /* 見つからなかった */
}

char *file_loadfile2(int clustno, int *psize, int *fat)
{
	int size = *psize, size2;
	struct MEMMAN *memman = task_now()->memman;
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	char *buf, *buf2;
	int i;
	buf = (char *) memman_alloc_4k(memman, size);
	pageman_link_page_32_m(pageman,buf,7,(size+0xfff)>>12,0);//
	//file_loadfile(clustno, size, buf, fat, (char *) (ADR_DISKIMG + 0x003e00));
	_file_read_file(buf,&size,fat,*(unsigned int*)0x0026f030,*(unsigned int*)0x0026f024,clustno,0);
	if (size >= 17) {
		size2 = tek_getsize(buf);
		if (size2 > 0) {	/* tek圧縮がかかっていた */
			buf2 = (char *) memman_alloc_4k(memman, size2);
			pageman_link_page_32_m(pageman,buf2,7,(size2+0xfff)>>12,0);//
			tek_decomp(buf, buf2, size2);
			for(i=0;i<(size+0xfff)>>12;i++){
				void* po=pageman_unlink_page_32(pageman,(int)(buf)+0x1000*i,1);
				//memman_free_page_32(pageman,po);
			}
			memman_free_4k(memman, (int) buf, size);
			buf = buf2;
			*psize = size2;
		}
	}
	return buf;
}
int task_vfs(){
	struct PAGEMAN32 *pageman=*(struct PAGEMAN32 **)ADR_PAGEMAN;
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	return 0;
}
