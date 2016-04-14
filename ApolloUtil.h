

#ifndef __APOLLO_UTIL_H__
#define __APOLLO_UTIL_H__

#include "ubx.h"

#define HIGH_HEXTOSTRING(x)		(((x & 0xF0) >> 4)+0x30)
#define LOW_HEXTOSTRING(x)		((x & 0xF) + 0x30)

//x '1' '8'
#define STRINGTOHEX(x, y)			(((((x = ((x >= 0x30 && x <= 0x39) ? x : 0x30)) - 0x30)  & 0xFF )<< 4)| (((y = ((y >= 0x30 && y <= 0x39) ? y : 0x30)) - 0x30) & 0xFF) )


void hextostring(char* id,const char *hex, int length);
int dbg_printf(const char *fmt, ...);

int write_dat(const char *filename,const char *data, int len) ;
int read_dat(const char *filename, char *data, int len) ;

void genBlockFileName(char* sender, char* filename);
void genBlockFilePathName(char *filename, char *pathname);
void genAgpsFilePathName(char *filename, char *pathname);

int takeDataPacket(const char *filename,unsigned char *data, char *packet);

int Separation(char ch, char *sequence, char ***pChTable, int *Count);
int apollo_atoi(char* pstr) ;
void writeTimeHeader(CH *buf);




//#define DEBUG 1
#ifndef DEBUG
#define apollo_printf dbg_printf 
#else
#define apollo_printf printf
#endif


#endif

