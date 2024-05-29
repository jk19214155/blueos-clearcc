#include "bootpack.h"

#define HBA_RCAP 0
#define HBA_RGHC 1
#define HBA_RIS 2
#define HBA_RPI 3
#define HBA_RVER 4
/*
#define HBA_RPBASE (0x40)
#define HBA_RPSIZE (0x80 >> 2)
#define HBA_RPxCLB 0
#define HBA_RPxFB 2
#define HBA_RPxIS 4
#define HBA_RPxIE 5
#define HBA_RPxCMD 6
#define HBA_RPxTFD 8
#define HBA_RPxSIG 9
#define HBA_RPxSSTS 10
#define HBA_RPxSCTL 11
#define HBA_RPxSERR 12
#define HBA_RPxSACT 13
#define HBA_RPxCI 14
#define HBA_RPxSNTF 15
#define HBA_RPxFBS 16
*/
#define AHCI_CLASS_ID 0x0106

//unsigned long long fis_base[32];
//unsigned long long fis_page_base[2];
// unsigned  long long clb_base[32];
// unsigned  long long clb_page_base[8];
int ahci_find_device_in_pcie(PCI_DEV* pci_dev){
	int recode=pcie_find_dev_by_class(pci_dev,AHCI_CLASS_ID);
	if(recode == 0){
		/*找到了设备*/
		return 0;
	}
	else if(recode == -1){
		/*没有找到*/
		return -1;
	}
	else{
		return recode;
	}
}
#define PCI_RCMD_MM_ACCESS (1<<1)
#define PCI_RCMD_DISABLE_INTR (1<<10)
#define PCI_RCMD_BUS_MASTER (1<<2)
#define HBA_PxCMD_FRE (1 << 4)
#define HBA_PxCMD_CR (1 << 15)
#define HBA_PxCMD_FR (1 << 14)
#define HBA_PxCMD_ST (1)
#define HBA_PxINTR_DMA (1 << 2)
static unsigned int ahci_regs_header_cap;
static unsigned int ahci_regs_header_pi;
//static AHCI_REGS* ahci_base_address;
extern char buff[1024];
#pragma pack(push,16)
	static char ahci_buff[4096];
#pragma pack(pop)

EFI_STATUS ahci_init(AHCI_DEV* ahci_dev);
//AHCI_TABLE* ahci_table;
AHCI_TABLE* ahci_init_all(){
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	struct MEMMAN* memman=task_now()->memman;
	struct TASK* task=task_now();
	struct CONSOLE* cons=task->cons;
	unsigned int i;
	AHCI_TABLE* ahci_table0=memman_alloc_page_32(pageman);
	//清空区域
	for(i=0;i<4096/8;i++){
		*((unsigned long long*)ahci_table0+i)=0;
	}
	for(i=0;i<8;i++){
		if(i>0){
			((PCI_DEV*)(&ahci_table0->ahci_dev[i]))->bus=((PCI_DEV*)(&ahci_table0->ahci_dev[i-1]))->bus;
			((PCI_DEV*)(&ahci_table0->ahci_dev[i]))->device=((PCI_DEV*)(&ahci_table0->ahci_dev[i-1]))->device;
			((PCI_DEV*)(&ahci_table0->ahci_dev[i]))->function=((PCI_DEV*)(&ahci_table0->ahci_dev[i-1]))->function+1;
		}
		EFI_STATUS status=ahci_init(&ahci_table0->ahci_dev[i]);
		if(EFI_ERROR(status)){
			cons_putstr0(cons, "AHCI: fin\n\n");
			ahci_table0->number=i;
			break;
		}
		else{
			cons_putstr0(cons, "AHCI: find\n\n");
		}
		
		AHCI_SATA_FIS* fis=ahci_make_fis(0,ahci_table0->ahci_dev[i].dev_info,0,0,0xec,0);
		unsigned int hba_pi=ahci_table0->ahci_dev[i].ahci_config_space_address->header.i.pi;
		for(int j=0;j<32;j++){
			if(hba_pi&(1<<j)){
				ahci_table0->ahci_dev[i].dev_info[j]=memman_alloc_page_32(pageman);
				ahci_fis_write_prdt(fis,0,ahci_table0->ahci_dev[i].dev_info[j],4095|(1<<31));
				ahci_fis_send(&(ahci_table0->ahci_dev[i]),j,fis,1);
			}
			else{
				ahci_table0->ahci_dev[i].dev_info[j]=NULL;
			}
		}
	}
	//ahci_table=ahci_table0;
	return ahci_table0;
}
EFI_STATUS ahci_software_reset(AHCI_ABI_REGS* port){
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	AHCI_SATA_FIS* fis1=memman_alloc_page_32(pageman);
	AHCI_SATA_FIS* fis2=memman_alloc_page_32(pageman);
	fis1=memman_alloc_page_32(pageman);
	fis2=memman_alloc_page_32(pageman);
	//ahci_make_fis(AHCI_SATA_FIS* fis,void* buff,unsigned long long lba,unsigned long long count,unsigned long long command,unsigned long long flag);
	//ahci_make_fis(AHCI_SATA_FIS* fis,void* buff,unsigned long long lba,unsigned long long count,unsigned long long command,unsigned long long flag);
}
EFI_STATUS ahci_init(AHCI_DEV* ahci_dev){
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	struct MEMMAN* memman=task_now()->memman;
	struct TASK* task=task_now();
	struct CONSOLE* cons=task->cons;
	EFI_STATUS status=ahci_find_device_in_pcie(ahci_dev);
	if(EFI_ERROR(status)){
		return -1;
	}
	if(cons!=0){
		sprintf(buff,"ahci_init:bus is %d\n",((PCI_DEV*)ahci_dev)->bus);
		cons_putstr0(cons, buff);
		sprintf(buff,"ahci_init:dev is %d\n",((PCI_DEV*)ahci_dev)->device);
		cons_putstr0(cons, buff);
		sprintf(buff,"ahci_init:func is %d\n",((PCI_DEV*)ahci_dev)->function);
		cons_putstr0(cons, buff);
	}
	unsigned int ahci_bar_6;
	/*读取6号bar的数值*/
	pcie_get_bar_from_device(ahci_dev,5,&ahci_bar_6);
	if(cons!=0){
		sprintf(buff,"ahci_init:ahci_bar_6 is %x\n",ahci_bar_6);
		cons_putstr0(cons, buff);
	}
	unsigned int ahci_bar_6_size;
	/*读取6号bar的大小*/
	//pcie_get_size_from_bar(&ahci_dev,6,&ahci_bar_6_size);
	if(cons!=0){
		sprintf(buff,"ahci_init:ahci_bar_6_size is %x\n",ahci_bar_6_size);
		cons_putstr0(cons, buff);
	}
	/*映射空间*/
	//AHCI_REGS* ahci_bar_6_base=memman_alloc_4k(memman,ahci_bar_6_size);
	//pageman_link_page_32_m(pageman,ahci_bar_6_base,ahci_bar_6|0x07,(ahci_bar_6_size+0xfff)>>12,1);
	//64位模式不进行映射
	AHCI_REGS* ahci_bar_6_base=ahci_bar_6;
	ahci_dev->ahci_config_space_address=ahci_bar_6_base;
	//return ahci_dev;
	/*配置控制寄存器*/
	unsigned int pcie_command=pcie_read_config(ahci_dev,0x04);
	pcie_command|=PCI_RCMD_MM_ACCESS|PCI_RCMD_DISABLE_INTR|PCI_RCMD_BUS_MASTER;
	pcie_write_config(ahci_dev,0x04,pcie_command);
	/*启用AHCI*/
	(ahci_bar_6_base->header).i.ghc|=(1<<31);//激活AHCI模式
	(ahci_bar_6_base->header).i.ghc|=(1<<1);//激活中断
	
	ahci_regs_header_cap=(ahci_bar_6_base->header).i.cap;
	ahci_regs_header_pi=(ahci_bar_6_base->header).i.pi;//所有已经开启的端口
	if(cons!=0){
		sprintf(buff,"ahci_init:ahci.header.cap is %x\n",ahci_regs_header_cap);
		cons_putstr0(cons, buff);
	}
	/*分配fis的空间*/
	for(int i=0;i<2;i++){
		ahci_dev->fis_page_base[i]=memman_alloc_page_64_4k(pageman);
		/*同一个页分配16个fis*/
		for(int j=0;j<16;j++){
			ahci_dev->fis_base[i*16+j]=ahci_dev->fis_page_base[i]+(256*j);
		}
	}
	/*首先分配clb的空间*/
	for(int i=0;i<8;i++){
		ahci_dev->clb_page_base[i]=memman_alloc_page_64_4k(pageman);
		/*同一个页分配4个clb*/
		for(int j=0;j<4;j++){
			ahci_dev->clb_base[i*4+j]=ahci_dev->clb_page_base[i]+(1024*j);
		}
	}
	/*端口重置*/
	for(int i=0;i<32;i++){
		if(cons!=0){
			sprintf(buff,"ahci_init:HBA_RPxSIG is %x\n",(ahci_bar_6_base->port)[i].i.HBA_RPxSIG);
			cons_putstr0(cons, buff);
		}
		//检测端口是否开启
		if((((ahci_bar_6_base->header).i.pi)&(1<<i))==0){//端口未开启
			continue;
		}
		if((ahci_bar_6_base->port)[i].i.HBA_RPxSIG==0xffffffff){
			continue;
		}
		//关闭ST
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD&=~(0x01);
		//关闭FRE
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD&=~(0x10);
		//关闭端口
		unsigned int rand=rdrand();
		struct TASK* task=task_now();
		struct TIMER* timer=timer_alloc(0);
		unsigned int fps=timer_get_fps(0);
		unsigned long long count=(100*fps)/1000;
		timer_init(timer, &(task->fifo),rand);
		timer_settime(0,timer,count&0xffffffff,(count>>32)&0xffffffff);
		while(1){
			unsigned int data;
			if(fifo32_status(&(task->fifo))!=0){
				for(;;){
					data=fifo32_get(&(task->fifo));
					if(data==rand||fifo32_status(&(task->fifo))==0){
						break;
					}
				}
			}
			if(data==rand){
				break;
			}
			//FR
			if((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&0x4000){
				continue;
			}
			//CR
			if((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&0x8000){
				continue;
			}
			break;
		}
		timer_free(timer);
		//FR
		if((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&0x4000){
			continue;
		}
		//CR
		if((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&0x8000){
			continue;
		}
		//分配操作空间
		(ahci_bar_6_base->port)[i].i.fb=ahci_dev->fis_base[i];
		(ahci_bar_6_base->port)[i].i.fbu=0;
		(ahci_bar_6_base->port)[i].i.clb=ahci_dev->clb_base[i];
		(ahci_bar_6_base->port)[i].i.clbu=0;
		(ahci_bar_6_base->port)[i].i.HBA_RPxCI=0;
		(ahci_bar_6_base->port)[i].i.HBA_RPxSERR=-1;
		//开启端口
		while((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&0x8000);
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD|=(0x10);
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD|=(0x01);
		
	}
	/*
	//端口重置
	
	for(int i =0;i<1;i++){//禁用重置
		if(cons!=0){
			sprintf(buff,"reg rsset %x\n",i);
			cons_putstr0(cons, buff);
		}
		if((((ahci_bar_6_base->header).i.pi)&(1<<i))==0){//端口未开启
			continue;
		}
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD&=~(0x01);
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD&=~(1<<4);
		for(int j=0;j<1000*1000;j++){
			sys_nop();
		}
		if((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&HBA_PxCMD_CR){
			if(cons!=0){
				cons_putstr0(cons, "port CR set\n");
			}
			continue;
		}
		else{
			if(cons!=0){
				cons_putstr0(cons, "port CR not set\n");
			}
		}
		(ahci_bar_6_base->port)[i].i.HBA_RPxSCTL =(((ahci_bar_6_base->port)[i].i.HBA_RPxSCTL)& ~0x0f) |1;
		for(int j=0;j<1000*1000;j++){
			sys_nop();
		}
		(ahci_bar_6_base->port)[i].i.HBA_RPxSCTL =((ahci_bar_6_base->port)[i].i.HBA_RPxSCTL)& ~0x0f;
	}
	*/
	/*分配操作空间*/
	/*clb size: 1024b=1/4 page *32=8 page*/
	/*fis size: 256b=1/16 page *32=2 page*/
	
	unsigned int msi_index = pcie_find_capbility_by_id(ahci_dev,0x05);//寻找MSI中断扩展配置 MSI-X的编码为0x11
	unsigned a=pcie_read_config(ahci_dev,msi_index*4);
	unsigned int msg_ctl=a>>16;
	int offset= (!!(msg_ctl&0x80))*4;//这里判断MSI结构格式是否有另外的4个字节偏移 64位系统有4个字节的高地址 32位系统没有
	pcie_write_config(ahci_dev,msi_index*4+8+offset,0x80);//0x80是硬盘中断号 可自定义
	if(msg_ctl&0x100){//如果支持中断MASK
		pcie_write_config(ahci_dev,msi_index*4+0x0c+offset,0);
	}
	a=(a&0xff8fffff)|0x10000;
	pcie_write_config(ahci_dev,msi_index*4,a);
	return EFI_SUCCESS;
}
AHCI_SATA_FIS* ahci_make_fis(AHCI_SATA_FIS* fis,void* buff,unsigned long long lba,unsigned long long count,unsigned long long command,unsigned long long flag){
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	if(fis==0){
		fis=memman_alloc_page_32(pageman);
	}
	(fis->cfis).ahci_cfis_0x27.type=0x27;
	(fis->cfis).ahci_cfis_0x27.command=command;
	(fis->cfis).ahci_cfis_0x27.device=0xe0;
	(fis->cfis).ahci_cfis_0x27.flag=(1<<7);
	(fis->cfis).ahci_cfis_0x27.lba15_0=lba&0xffff;
	(fis->cfis).ahci_cfis_0x27.lba23_16=(lba>>16)&0xff;
	(fis->cfis).ahci_cfis_0x27.lba39_24=(lba>>24)&0xffff;
	(fis->cfis).ahci_cfis_0x27.lba47_40=(lba>>40)&0xff;
	(fis->cfis).ahci_cfis_0x27.count=count;
	(fis->cfis).ahci_cfis_0x27.aux=0;
	(fis->cfis).ahci_cfis_0x27.reserved=0;
	(fis->cfis).ahci_cfis_0x27.features2=0;
	(fis->cfis).ahci_cfis_0x27.features=0;
	//fis->prdt[0].dba=data_base;
	//fis->prdt[0].dbau=0;
	//fis->prdt[0].dbc=4096-1;//设备信息有512字节大小
	return fis;
}
void ahci_fis_write_prdt(AHCI_SATA_FIS* fis,unsigned int index,unsigned long long address,unsigned long long count){
	fis->prdt[index].dba=address&0xffffffff;
	fis->prdt[index].dbau=(address>>32)&0xffffffff;
	fis->prdt[index].dbc=count-1;
}
AHCI_SATA_FIS* ahci_fis_send(AHCI_DEV* dev,unsigned int ahci_abi_regs_index,AHCI_SATA_FIS* fis,unsigned long long prdtl){
	struct MEMMAN* memman=task_now()->memman; 
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	struct TASK* task=task_now();
	struct CONSOLE* cons=task->cons;
	AHCI_COMMAND_HEADER* clb=(AHCI_COMMAND_HEADER*)(dev->clb_base)[ahci_abi_regs_index];//命令区块地址 
	//AHCI_COMMAND_HEADER* clb=(void*)0x61108000;
	/*寻找空闲的命令队列*/
	AHCI_REGS* ahci_base_address=dev->ahci_config_space_address;
	unsigned int HBA_RPxCI=( ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxCI;
	unsigned int HBA_RPxSACT=(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSACT;
	int i;
	for(i=0;i<32;i++){
		if((HBA_RPxCI&(1<<i))==1){//被占用则测试下一个
			continue;
		}
		if((HBA_RPxSACT&(1<<i))==1){//被占用则测试下一个
			continue;
		}
		//找到一个没有被占用的命令槽位
		//判断是哪一种设备
		unsigned int HBA_RPxSIG=(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSIG;
		if(HBA_RPxSIG==0x00000101){//ATA设备
			if(cons!=0){
				sprintf(buff,"AHCI ATA HBA_RPxSIG is %x\n",HBA_RPxSIG);
				cons_putstr0(cons,buff);
			}
			if(cons!=0){
				sprintf(buff,"AHCI ATA i is %x\n",i);
				cons_putstr0(cons,buff);
			}
			if(cons!=0)
				cons_putstr0(cons,"AHCI ATA device 00\n");

			clb[i].flags=(0x05)|ahci_command_header$flags$c;
			clb[i].prdtl=prdtl;
			clb[i].command_table_address_32=((unsigned long long)fis)&0xffffffff;//挂载
			clb[i].command_table_address_64=((unsigned long long)fis>>32)&0xffffffff;
			//HBA_RPxSACT|=(1<<i);//执行
			//(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSACT=HBA_RPxSACT;
			//HBA_RPxSACT|=(1<<i);//执行
			//(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSACT=HBA_RPxSACT;
			HBA_RPxCI|=(1<<i);//执行
			(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxCI=HBA_RPxCI;
			return fis;
			//释放内存
			//pageman_unlink_page_32(pageman,fis,1);
			//memman_free_4k(memman,fis,4096);
		}
		else if(HBA_RPxSIG==0xeb140101){//ATAPI设备;
			if(cons!=0)
				cons_putstr0(cons, "AHCI ATAPI device");
			break;
		}
		else if(HBA_RPxSIG==0xc33c0101){//SATA 保留类型
			if(cons!=0)
				cons_putstr0(cons, "AHCI SATA RES01 device");
			return -1;
		}
		else if(HBA_RPxSIG==0x96690101){//SATA 保留类型
			if(cons!=0)
				cons_putstr0(cons, "AHCI SATA RES02 device");
			return -1;
		}
		else if(HBA_RPxSIG==0xaace0000){//未知设备,HBA_RPxSIG值为0xaacexxxx(x是N/A)
			if(cons!=0)
				cons_putstr0(cons, "AHCI SATA unknow device");
			return -1;
		}
	}
	if(i>=32){//没有空闲命令槽位
		return -1;
	}
	else{
		return 0;
	}
}
/*获取设备类型*/
unsigned int ahci_get_device_type(PCI_DEV* dev,unsigned int ahci_abi_regs_index){
	return (((AHCI_DEV*)dev)->ahci_config_space_address->port)[ahci_abi_regs_index].i.HBA_RPxSIG;//获取设备签名
}
void ahci_make_sata_fis(AHCI_SATA_FIS* fis){
	
}