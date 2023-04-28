#include "bootpack.h"
#define MaxDevice 32
StorageDevice *List=0;
int StorageDeviceNumber=0;
void device_init(){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct PAGEMAN32 *pageman=*((struct PAGEMAN32 **)ADR_PAGEMAN);
	struct TASK *task = task_now();
	int i;
	List=0;
	StorageDeviceNumber=0;
	List=memman_alloc_4k(memman, MaxDevice * sizeof(StorageDevice));
	pageman_link_page_32_m(pageman,List,7,(MaxDevice * sizeof(StorageDevice)+0xfff)>>12,0);
	for(i=0;i<MaxDevice;i++){
		List[i].driver=0;//没有设备驱动程序
		List[i].status=0;//设备未挂载
	}
	return;
}
unsigned int device_add(char* name,unsigned int size,unsigned int free,StorageDeviceOperations* driver){
	int i;
	for(i=0;i<MaxDevice;i++){
		if(List[i].status==0){
			List[i].status=1;
			List[i].name=name;
			List[i].size=size;
			List[i].free=free;
			List[i].driver=driver;
			List[i].rand=rdrand();//获取一个随机数
			return i;
		}
	}
	return -1;//没有空位了
}
unsigned int device_remove(unsigned int index){
	if(index>MaxDevice){
		return-1;
	}
	else{
		List[index].status=0;
		List[index].rand=rdrand();
	}
}
StorageDevice* device_get_list(){
	return List;
}
unsigned int device_open(unsigned int device_index,int* rand,int* user_index,char* name,int mode){
	if(device_index>MaxDevice){
		return -1;
	}
	else if(List[device_index].status==0){
		return -1;
	}
	else{
		*rand=List[device_index].rand;
		int status=List[device_index].driver->open(List+device_index,user_index,name,mode);
		if(List[device_index].status==0){//设备已经关闭
			List[device_index].rand=rdrand();
		}
		return status;
	}
}
unsigned int device_read(unsigned int device_index,int rand,int user_index,char* buff,unsigned int* count){
	if(device_index>MaxDevice){
		return -1;
	}
	else if(List[device_index].status==0){
		return -1;
	}
	else if(List[device_index].rand!=rand){
		return -1;
	}
	else{
		int status=List[device_index].driver->read(List+device_index,user_index,buff,count);
		if(List[device_index].status==0){//设备已经关闭
			List[device_index].rand=rdrand();
		}
		return status;
	}
}
unsigned int device_write(unsigned int device_index,int rand,int user_index,char* buff,unsigned int* count){
	if(device_index>MaxDevice){
		return -1;
	}
	else if(List[device_index].status==0){
		return -1;
	}
	else if(List[device_index].rand!=rand){
		return -1;
	}
	else{
		int status=List[device_index].driver->write(List+device_index,user_index,buff,count);
		if(List[device_index].status==0){//设备已经关闭
			List[device_index].rand=rdrand();
		}
		return status;
	}
}
unsigned int device_seek(unsigned int device_index,int rand,int user_index,int* count){
	if(device_index>MaxDevice){
		return -1;
	}
	else if(List[device_index].status==0){
		return -1;
	}
	else if(List[device_index].rand!=rand){
		return -1;
	}
	else{
		int status=List[device_index].driver->seek(List+device_index,user_index,count);
		if(List[device_index].status==0){//设备已经关闭
			List[device_index].rand=rdrand();
		}
		return status;
	}
}