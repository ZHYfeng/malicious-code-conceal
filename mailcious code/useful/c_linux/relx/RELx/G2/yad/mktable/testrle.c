#include <stdio.h>
#include <stdint.h>
#include "yad_data.h"

unsigned char rle_compress[] = {
0x60,0xfc,0x83,0xc4,0x24,0x5e,0x5f,0x59,
0x83,0xec,0x30,0x31,0xc0,0xac,0x89,0xc3,
0x31,0xd2,0xac,0x39,0xd8,0x75,0x0e,0x80,
0xfa,0x3f,0x74,0x09,0x80,0xf9,0x01,0x74,
0x04,0x42,0xe2,0xee,0x41,0x93,0x09,0xd2,
0x75,0x04,0x3c,0xc0,0x72,0x06,0x80,0xca,
0xc0,0x88,0x17,0x47,0xaa,0x89,0xfd,0x2b,
0x6c,0x24,0x28,0x3b,0x6c,0x24,0x2c,0x72,
0x04,0x31,0xed,0xeb,0x02,0xe2,0xc9,0x89,
0x6c,0x24,0x1c,0x61,0xc3
};

int main(int argc, char **argv)
{
	unsigned char dst[2048];
	int o, s;
	o = sizeof(yad_data);
	s = ((int(*)(char*,char*,int))rle_compress)(yad_data, dst, o);
	printf("Testing RLE comp. %d:%d (%.2f%%)\n", o, s, s*100.0/o);
	return 0;
}