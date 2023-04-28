#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include "bootpack.h"
typedef struct {
    unsigned short type;       // 文件类型，必须是 "BM"（0x424d）
    unsigned int size;         // 整个 BMP 文件大小（以字节为单位）
    unsigned short reserved1;  // 保留，必须是 0
    unsigned short reserved2;  // 保留，必须是 0
    unsigned int offset;       // BMP 数据的偏移量，即文件头长度
} bmp_header;

typedef struct {
    unsigned int size;                  // 此结构体的大小
    int width;                          // 图像宽度（以像素为单位）
    int height;                         // 图像高度（以像素为单位）
    unsigned short planes;              // 颜色平面数，必须是 1
    unsigned short bits_per_pixel;      // 每个像素的位数
    unsigned int compression;           // 图像数据的压缩类型，0 表示不压缩
    unsigned int image_size;            // 图像数据的大小（以字节为单位）
    int x_pixels_per_meter;             // 水平分辨率（像素/米）
    int y_pixels_per_meter;             // 垂直分辨率（像素/米）
    unsigned int colors_used;           // 实际使用的颜色表中的颜色数
    unsigned int colors_important;      // 对图像显示有重要影响的颜色数，0 表示都重要
} bmp_info;

void write_bmp(int* buff, int xsize, int ysize, char* buff2) {
    bmp_header header;
    bmp_info info;
	void* p;
	int buff2_point=0;
    int padding = ((xsize+3)&0xfffffffc)-xsize;//每一行需要补齐到xsize是4的倍数
    int row_size = xsize * 4;
    int i, j;

    memset(&header, 0, sizeof(header));
    memset(&info, 0, sizeof(info));

    header.type = 0x4D42; // "BM"
    header.offset = sizeof(header) + sizeof(info);
    header.size = header.offset + (row_size + padding) * ysize;

    info.size = sizeof(info);
    info.width = xsize;
    info.height = ysize;
    info.planes = 1;
    info.bits_per_pixel = 32;
    info.image_size = row_size * ysize;
    //写入头部数据
	p=&header;
	for(i=0;i<sizeof(header);i++){
		buff2[buff2_point++]=((char*)p)[i];
	}
	//写入info数据
	p=&info;
	for(i=0;i<sizeof(info);i++){
		buff2[buff2_point++]=((char*)p)[i];
	}

    for (i = ysize - 1; i >= 0; i--) {
        for (j = 0; j < xsize; j++) {
            int index = (i * xsize) + j;
			//依次写入三个数据
            buff2[buff2_point++]=(buff[index] >> 0) & 0xff;
			buff2[buff2_point++]=(buff[index] >> 8) & 0xff;
			buff2[buff2_point++]=(buff[index] >> 16) & 0xff;
        }
		//补齐至4的倍数
        for(j=0;j<padding;j++){
			buff2[buff2_point++]=0;
		}
    }
	return;
}