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
#define AHCI_CLASS_ID 0x010601

static unsigned int fis_base[32];
static unsigned int fis_page_base[2];
static unsigned int clb_base[32];
static unsigned int clb_page_base[8];
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
#define PCI_RCMD_MM_ACCESS (1<<0)
#define PCI_RCMD_DISABLE_INTR (1<<3)
#define PCI_RCMD_BUS_MASTER (1<<2)
#define HBA_PxCMD_FRE (1 << 4)
#define HBA_PxCMD_CR (1 << 15)
#define HBA_PxCMD_FR (1 << 14)
#define HBA_PxCMD_ST (1)
#define HBA_PxINTR_DMA (1 << 2)
static unsigned int ahci_regs_header_cap;
static unsigned int ahci_regs_header_pi;
static AHCI_REGS* ahci_base_address;
#pragma pack(push,16)
	static char ahci_buff[4096];
#pragma pack(pop)
int ahci_init(){
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	struct MEMMAN* memman=task_now()->memman;
	PCI_DEV ahci_dev;
	ahci_find_device_in_pcie(&ahci_dev);
	unsigned int ahci_bar_6;
	/*读取6号bar的数值*/
	pcie_get_bar_from_device(&ahci_dev,6,&ahci_bar_6);
	unsigned int ahci_bar_6_size;
	/*读取6号bar的大小*/
	pcie_get_size_from_bar(&ahci_dev,6,&ahci_bar_6_size);
	/*映射空间*/
	AHCI_REGS* ahci_bar_6_base=memman_alloc_4k(memman,ahci_bar_6_size);
	pageman_link_page_32_m(pageman,ahci_bar_6_base,ahci_bar_6|0x07,(ahci_bar_6_size+0xfff)>>12,1);
	ahci_base_address=ahci_bar_6_base;
	/*配置控制寄存器*/
	unsigned int pcie_command=pcie_read_config(&ahci_dev,0x04);
	pcie_command|=PCI_RCMD_MM_ACCESS|PCI_RCMD_DISABLE_INTR|PCI_RCMD_BUS_MASTER;
	pcie_write_config(&ahci_dev,0x04,pcie_command);
	/*启用AHCI*/
	(ahci_bar_6_base->header).i.ghc|=0x01;//激活AHCI模式
	(ahci_bar_6_base->header).i.ghc|=(1<<32);
	ahci_regs_header_cap=(ahci_bar_6_base->header).i.cap;
	ahci_regs_header_pi=(ahci_bar_6_base->header).i.pi;//所有已经开启的端口
	
	/*端口重置*/
	for(int i =0;0&(i<32);i++){//禁用重置
		if(((ahci_bar_6_base->header).i.pi)&(1<<i)==0){//端口未开启
			continue;
		}
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD&=~(0x01);
		(ahci_bar_6_base->port)[i].i.HBA_RPxCMD&=~(1<<4);
		for(int j=0;j<1000*1000*500;j++){
			sys_nop();
		}
		if((ahci_bar_6_base->port)[i].i.HBA_RPxCMD&HBA_PxCMD_CR){
			continue;
		}
		(ahci_bar_6_base->port)[i].i.HBA_RPxSCTL =((ahci_bar_6_base->port)[i].i.HBA_RPxSCTL)& ~0x0f |1;
		for(int j=0;j<1000*1000;j++){
			sys_nop();
		}
		(ahci_bar_6_base->port)[i].i.HBA_RPxSCTL =((ahci_bar_6_base->port)[i].i.HBA_RPxSCTL)& ~0x0f;
	}
	
	/*分配操作空间*/
	/*clb size: 1024b=1/4 page *32=8 page*/
	/*fis size: 256b=1/16 page *32=2 page*/
	/*首先分配clb的空间*/
	for(int i=0;i<8;i++){
		unsigned int p=memman_alloc_4k(memman,4096);
		clb_page_base[i]=pageman_link_page_32(pageman,p,0x07,0);
		/*同一个页分配4个clb*/
		for(int j=0;j<4;j++){
			(ahci_bar_6_base->port)[i*16+j].i.clb=clb_page_base[i]+(1024*j);
			(ahci_bar_6_base->port)[i*16+j].i.clbu=0;
			clb_base[i*4+j]=clb_page_base[i]+(1024*j);
		}
	}
	/*分配fis的空间*/
	for(int i=0;i<2;i++){
		unsigned int p=memman_alloc_4k(memman,4096);
		fis_page_base[i]=pageman_link_page_32(pageman,ahci_bar_6_base,ahci_bar_6|0x07,0);
		/*同一个页分配4个clb*/
		for(int j=0;j<16;j++){
			(ahci_bar_6_base->port)[i*16+j].i.fb=clb_page_base[i]+(256*j);
			(ahci_bar_6_base->port)[i*16+j].i.fbu=0;
			fis_base[i*16+j]=clb_page_base[i]+(256*j);
		}
	}
	/*清空CI寄存器,清空PxSERR*/
	for(int i=0;i<32;i++){
		(ahci_bar_6_base->port)[i].i.HBA_RPxCI=0;
		(ahci_bar_6_base->port)[i].i.HBA_RPxSERR=-1;
		
	}
}
unsigned int ahci_get_info(PCI_DEV* dev,unsigned int ahci_abi_regs_index,void* buff){
	struct MEMMAN* memman=task_now()->memman;
	struct PAGEMAN32* pageman=*((void** )ADR_PAGEMAN);
	/*寻找空闲的命令队列*/
	unsigned int HBA_RPxCI=(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxCI;
	unsigned int HBA_RPxSACT=(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSACT;
	int i;
	for(i=0;i<32;i++){
		if(HBA_RPxCI&(1<<i)==1){//被占用则测试下一个
			continue;
		}
		if(HBA_RPxSACT&(1<<i)==1){//被占用则测试下一个
			continue;
		}
		//找到一个没有被占用的命令槽位
		AHCI_COMMAND_HEADER* clb=(AHCI_COMMAND_HEADER*)(ahci_base_address->port)[ahci_abi_regs_index].i.clb;//命令区块地址
		//判断是哪一种设备
		unsigned int HBA_RPxSIG=(ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSIG;
		if(HBA_RPxSIG==0x00000101){//ATA设备
			AHCI_SATA_FIS* fis=memman_alloc_4k(memman,4096);
			pageman_link_page_32(pageman,fis,0x07,0);
			(fis->cfis).ahci_cfis_0x27.type=0x27;
			(fis->cfis).ahci_cfis_0x27.command=ATA_IDENTIFY_DEVICE;
			
			unsigned int data_base=get_physical_by_linear_32((unsigned int)buff);//获取物理地址
			fis->prdt[0].dba=data_base;
			fis->prdt[0].dbau=0;
			fis->prdt[0].dbc=511|(1<<31);//设备信息有512字节大小
			
			clb[i].flags=(64/4)|ahci_command_header$flags$c;
			clb[i].prdtl=16;
			clb[i].command_table_address_32=fis;//挂载
			HBA_RPxCI&=(1<<i);//执行
			//释放内存
			pageman_unlink_page_32(pageman,fis,1);
			memman_free_4k(memman,fis,4096);
		}
		else if(HBA_RPxSIG==0xeb140101){//ATAPI设备
			
		}
		else if(HBA_RPxSIG==0xc33c0101){//SATA 保留类型
			return -1;
		}
		else if(HBA_RPxSIG==0x96690101){//SATA 保留类型
			return -1;
		}
		else if(HBA_RPxSIG==0xaace0000){//未知设备,HBA_RPxSIG值为0xaacexxxx(x是N/A)
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
	return (ahci_base_address->port)[ahci_abi_regs_index].i.HBA_RPxSIG;//获取设备签名
}
void ahci_make_sata_fis(AHCI_SATA_FIS* fis){
	
}