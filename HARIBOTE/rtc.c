#include "bootpack.h"

// 等待 RTC 不忙
unsigned char rtc_is_updating() {
    io_out8(0x70, 0x0A); // 选择状态寄存器 A
    return io_in8(0x71) & 0x80; // 检查更新标志
}

// 读取 RTC 单个寄存器
unsigned char rtc_read_register(unsigned char reg) {
    io_out8(0x70, reg); // 选择寄存器
    return io_in8(0x71); // 读取数据
}

// 将 BCD 转换为二进制
unsigned char bcd_to_binary(unsigned char bcd) {
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

// 获取当前时间
void get_rtc_time(unsigned char* hour, unsigned char* minute, unsigned char* second) {
    while (rtc_is_updating()); // 等待 RTC 不忙

    *second = rtc_read_register(0x00);
    *minute = rtc_read_register(0x02);
    *hour = rtc_read_register(0x04);

    // 如果是 BCD 编码，转换为二进制
    unsigned char status_b = rtc_read_register(0x0B);
    if (!(status_b & 0x04)) { // 检查是否是 BCD 编码
        *second = bcd_to_binary(*second);
        *minute = bcd_to_binary(*minute);
        *hour = bcd_to_binary(*hour);
    }
}

void my_custom_time(void *context, int argc, void **argv) {
    unsigned long long now = 0;
    sqlite3_result_int64(context, (unsigned long long)now);
}

unsigned long long localtime_s(struct tm* result, const time_t* timep){
	result->tm_sec=10;
	result->tm_min=10;
	result->tm_hour=1;
	result->tm_mday=1;
	result->tm_mon=1;
	result->tm_year=2000;
	result->tm_wday=1;
	result->tm_yday=1;
	result->tm_isdst=0;
}

unsigned long long localtime(time_t* timep){
	timep=0;
	return 0;
}
