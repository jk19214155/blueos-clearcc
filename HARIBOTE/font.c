#include "bootpack.h"


unsigned long long font_unicode16_to_unicode32(unsigned short* unicode16){
    unsigned long long unicode32=0;
    unicode32=(unsigned long long)(unicode16[0]<<8|unicode16[1]);
    return unicode32;
}
