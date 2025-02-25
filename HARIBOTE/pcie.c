#include "bootpack.h"
int pcie_cp(PCI_DEV* from,PCI_DEV* to){
	to->bus=from->bus;
	to->device=from->device;
	to->function=from->function;
	to->vendor_id=from->vendor_id;
	to->device_id=from->device_id;
	to->class_code=from->class_code;
	to->subclass_code=from->subclass_code;
	return;
}
unsigned int pcie_read_config(PCI_DEV* dev,int reg_offset){
	unsigned int id = (dev->bus<<16) | (dev->device<<11) | (1<<31) | (dev->function<<8);
	io_out32(PCI_CONFIG_PORT,id|reg_offset);
	id = io_in32(PCI_DATA_PORT);
	return id;
}
void pcie_write_config(PCI_DEV* dev,int reg_offset,unsigned int config){
	unsigned int id = (dev->bus<<16) | (dev->device<<11) | (1<<31) | (dev->function<<8);
	io_out32(PCI_CONFIG_PORT,id|reg_offset);
	io_out32(PCI_DATA_PORT,config);
	return;
}
EFI_STATUS  pcie_find_dev_by_class(PCI_DEV *pci_dev, int class_id){
	int bus= pci_dev->bus;
	int dev=pci_dev->device;
	int func=pci_dev->function;
	goto start;
	for(bus=0; bus < 256; bus++){
        // 枚举每一总线上的所有设备
        for(dev=0; dev < 32; dev++){
			for(func=0;func<8;func++){
				start:
				unsigned int id = (bus<<16) | (dev<<11) | (1<<31) | (func<<8);
				io_out32(PCI_CONFIG_PORT,id); 
				unsigned int u = io_in32(PCI_DATA_PORT);
				unsigned short vendor_id =u&0xffff;
				if(vendor_id==0xffff){//设备不存在
					continue;
				}
				io_out32(PCI_CONFIG_PORT,id|0x08);
				u = io_in32(PCI_DATA_PORT);
				if((u>>16) == class_id){
					extern char buff[1024];
					struct TASK* task=task_now();
					struct CONSOLE* cons=task->cons;
					if(cons!=0){
						sprintf(buff,"pcie_find_dev_by_class| class id %x\n",class_id);
						cons_putstr0(cons,buff);
						sprintf(buff,"pcie_find_dev_by_class| class u>>16 %x\n",(u>>16));
						cons_putstr0(cons,buff);
						sprintf(buff,"pcie_find_dev_by_class| class bus %x\n",bus);
						cons_putstr0(cons,buff);
						sprintf(buff,"pcie_find_dev_by_class| class dev %x\n",dev);
						cons_putstr0(cons,buff);
						sprintf(buff,"pcie_find_dev_by_class| class func %x\n",func);
						cons_putstr0(cons,buff);
					}
					pci_dev->bus=bus;
					pci_dev->device=dev;
					pci_dev->function=func;
					pci_dev->class_code=u;
					io_out32(PCI_CONFIG_PORT,id);
					u = io_in32(PCI_DATA_PORT);
					pci_dev->vendor_id=u&0xffff;
					pci_dev->device_id=(u>>16)&0xffff;
					return 0;
				}
			}
		}
	}
	/*找不到特定设备*/
	return 0xffffffff;
}

int pcie_get_id_by_dev(PCI_DEV* pci_dev,unsigned int id){
	
}
	
int pcie_get_bar_from_device(PCI_DEV* pci_dev,unsigned int bar,unsigned int* res){
	unsigned int id = (pci_dev->bus<<16) | (pci_dev->device<<11) | (1<<31) | (pci_dev->function<<8);
	id|=bar*4+0x10;
	io_out32(PCI_CONFIG_PORT,id); 
	unsigned int u = io_in32(PCI_DATA_PORT);
	*res=u;
	return 0;
}

int pcie_get_size_from_bar(PCI_DEV* pci_dev,unsigned int bar,unsigned int* size){
	/*读取原本的数值*/
	unsigned int id = (pci_dev->bus<<16) | (pci_dev->device<<11) | (1<<31) | (pci_dev->function<<8);
	id|=bar*4+0x10;
	io_out32(PCI_CONFIG_PORT,id); 
	unsigned int u = io_in32(PCI_DATA_PORT);
	/*写入全1测试大小*/
	io_out32(PCI_CONFIG_PORT,id);
	io_out32(PCI_DATA_PORT,0xffffffff);
	io_out32(PCI_CONFIG_PORT,id); 
	unsigned int w = io_in32(PCI_DATA_PORT)&~0x01;
	*size=~w+1;
	/*恢复原值*/
	io_out32(PCI_CONFIG_PORT,id);
	io_out32(PCI_DATA_PORT,u);
	return 0;
}

unsigned int pcie_get_rcba(){
	unsigned int id = (0<<16) | (31<<11) | (1<<31) | (0<<8) | 0xf0;
	unsigned long long address = (unsigned long long)((0 << 16) | (31 << 11) | (0 << 8) | (0xf0 & 0xFC) | ((unsigned long)0x80000000));
	io_out32(PCI_CONFIG_PORT,address);
	id = io_in32(PCI_DATA_PORT);
	id &= ~0xffff;
	return id;
}
unsigned int pcie_find_capbility_by_id(PCI_DEV* pci_dev,unsigned int id){
	unsigned char capbility_pointer = pcie_read_config(pci_dev,0x34*4)&0xff;
	if(capbility_pointer==0){
		return 0;
	}
	for(;;){
		unsigned i=pcie_read_config(pci_dev,capbility_pointer*4);
		unsigned char capbility_id=i&0xff;
		if(capbility_id==id){
			return capbility_pointer;
		}
		unsigned char capbility_pointer = (i>>8)&0xff;
		if(capbility_pointer==0){
			return 0;
		}
	}
	return 0;
}