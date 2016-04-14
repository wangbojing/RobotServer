

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "ApolloUtil.h"
#include "ApolloProtocol.h"

void hextostring(char* id,const char *hex, int length) {
	int i = 0;
	char data = 0x0;
	for (i = 0;i < length;i ++) {
		id[2*i] = HIGH_HEXTOSTRING(hex[i]);
		id[2*i+1] = LOW_HEXTOSTRING(hex[i]);
		if (id[2*i] > 0x39) id[2*i]= (((hex[i] & 0xF0)>>4) - 0x0A) + 0x41;
		if (id[2*i+1] > 0x39) id[2*i+1]=((hex[i] & 0x0F) - 0x0A) + 0x41;
	}
}

void genBlockFileName(char* sender, char* filename) {	
	sprintf(filename, "%s.dat", sender);
}

void genBlockFilePathName(char *filename, char *pathname) {
	
	sprintf(pathname, "block_data/%s", filename);
}

void genAgpsFilePathName(char *filename, char *pathname) {
	
	sprintf(pathname, "agps_data/%s_agps.dat", filename);
}



void writeTimeHeader(CH *buf) {
	time_t timer;
	struct tm *tblock;
	timer = time(NULL);
	tblock = localtime(&timer);

	buf[0] = (char)(tblock->tm_year - 100);
	buf[1] = (char)(tblock->tm_mon);
	buf[2] = (char)(tblock->tm_wday);
	buf[3] = (char)(tblock->tm_mday);
	buf[4] = (char)(tblock->tm_hour);
	buf[5] = (char)(tblock->tm_min);
	buf[6] = (char)(tblock->tm_sec);
}

int writeLogFileTimeStamp(char *buf) {
	time_t timer;
	struct tm *tblock;
	timer = time(NULL);
	tblock = localtime(&timer);

	sprintf(buf, " [%d-%d-%d %d:%d:%d] ", (tblock->tm_year%100),
		(tblock->tm_mon), (tblock->tm_mday), (tblock->tm_hour),
		(tblock->tm_min), (tblock->tm_sec));

	return strlen(buf);
}



#define LOGFILE_NAME		".log"
#define PRINTF_CHAR_MAX		256

static FILE * pLogFiles = NULL;
static FILE* get_logs_handler(void) {
	char buffer[128] = {0};
	time_t timer;
	struct tm *tblock;
	timer = time(NULL);
	tblock = localtime(&timer);

	sprintf(buffer, "./log_data/%d-%d-%d-%d-%d%s", tblock->tm_year, 
		tblock->tm_mon, tblock->tm_mday, tblock->tm_hour,
		tblock->tm_min, LOGFILE_NAME);
	
	if (pLogFiles == NULL) {
		pLogFiles = fopen(buffer, "w");
		if (pLogFiles == NULL) {
			apollo_printf("Error: can not create %s\n", LOGFILE_NAME);
			return NULL;
		}
	}
	fseek(pLogFiles, 0, SEEK_END);

	return pLogFiles;
}


int dbg_printf(const char *fmt, ...) {
	int i, iret;
	int len;
	char szPrint[PRINTF_CHAR_MAX] = {0};
	FILE *fp = get_logs_handler();

	va_list ap;
	va_start(ap, fmt);

	len = writeLogFileTimeStamp(szPrint);
	iret = vsnprintf(szPrint+len-1, PRINTF_CHAR_MAX-len, fmt, ap);
	va_end(ap);

	i = fwrite(szPrint, 1, iret, fp);
	fflush(fp);
	//tcflush(gpsDes,TCIOFLUSH);

	//IOT
	//fclose(fp);
	//fp = NULL;
	//pLogFiles = NULL;

	return i;
}

int write_dat(const char *filename, const char *data, int len) {
	FILE *pAMRFile = fopen(filename, "w+b");
	int size = 0;

	size = fwrite(data, 1, len, pAMRFile);
	fflush(pAMRFile);

	fclose(pAMRFile);

	return size;
}

int read_dat(const char *filename, char *data, int len) {
	FILE *pAMRFile = fopen(filename, "rb+");

	int size = fread(data, 1, len, pAMRFile);

	apollo_printf("aaa file size: %d \n", size);
	fclose(pAMRFile);

	return size;
}

int takeDataPacket(const char *filename,unsigned char *data, char *packet) {
	unsigned char buffer[LEN_BLOCK_BUFFER];
	int i = 0, index = 0, j = 0;
	int size = read_dat(filename, buffer, LEN_BLOCK_BUFFER);
	unsigned char header[LEN_HEADER_LENGTH] = {
		'V', 0x0, 0x86, 0x18,0x64,0x33,0x87,0x42,
		0x90, 0x15,0x09,0x00,0x02, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0,0x0, 0x0, 0x0,0x0, 0x0, 
		0x0, 0x0, 0x0,0x0, 0x0, 0x0,0x0, 0x0
	};

	for (i = 0;i < LEN_DEVICE_ID;i ++) {
		header[i+1] = packet[i+1];
	}

	if (size % LEN_BLOCK_DATA) {
		header[29] = size / LEN_BLOCK_DATA + 1;
	} else {
		header[29] = size / LEN_BLOCK_DATA;
	}

	for (i = 0;i < size/LEN_BLOCK_DATA;i ++) {
		header[13] = i;
		header[30] = LEN_BLOCK_DATA / 256;
		header[31] = LEN_BLOCK_DATA % 256;

		apollo_printf("header[30]: %d, header[31]:%d, total:%d\n", header[30], header[31], LEN_BLOCK_DATA);
		for (j = 0;j < LEN_HEADER_LENGTH;j ++) {
			data[index++] = header[j];
		}

		for (j = 0;j < LEN_BLOCK_DATA ;j ++) {
			data[index++] = buffer[i * LEN_BLOCK_DATA + j];
		}
	}

	if (size % LEN_BLOCK_DATA) {
		header[13] = i;
		header[30] = (size % LEN_BLOCK_DATA) / 256;
		header[31] = (size % LEN_BLOCK_DATA) % 256;

		apollo_printf("header[30]: %d, header[31]:%d, total:%d\n", header[30], header[31], (size % LEN_BLOCK_DATA));

		for (j = 0;j < LEN_HEADER_LENGTH;j ++) {
			data[index++] = header[j];
		}

		for (j = 0;j < (size % LEN_BLOCK_DATA);j ++) {
			data[index++] = buffer[i * LEN_BLOCK_DATA + j];
		}
	} 

	return index;
}

int Separation(char ch, char *sequence, char ***pChTable, int *Count) {
	int i = 0, j = 0;
	int len = strlen(sequence);
	*Count = 0;
	char ChArray[32] = {0};
	char **pTable = *pChTable;

	for (i = 0;i < len;i ++) {
		if (sequence[i] == ch) {
			pTable[*Count] = (char*)malloc((j+1) * sizeof(char));
			memcpy(pTable[*Count], ChArray, j+1);
			(*Count) ++;

			pTable = (char**)realloc(pTable, ((*Count)+1) * sizeof(char**));
			j = 0;
			memset(ChArray, 0, 32);

			continue;
		} 
		ChArray[j++] = sequence[i];
	}
	
	pTable[*Count] = (char*)malloc((j+1) * sizeof(char));
	memcpy(pTable[*Count], ChArray, j+1);
	(*Count) ++;
	
	memset(ChArray, 0, 32);

	*pChTable = pTable;

	return 0;
}

int apollo_atoi(char* pstr)  
{  
    int Ret_Integer = 0;  
    int Integer_sign = 1;  

    if(pstr == NULL)  {  
        printf("Pointer is NULL\n");  
        return 0;  
    }  
 
    while(isspace(*pstr) == 0) {  
        pstr++;  
    }  
   
    if(*pstr == '-') {  
        Integer_sign = -1;  
    }  
    if(*pstr == '-' || *pstr == '+') {  
        pstr++;  
    }  
  
    while(*pstr >= '0' && *pstr <= '9') {  
        Ret_Integer = Ret_Integer * 10 + *pstr - '0';  
        pstr++;  
    }  
    Ret_Integer = Integer_sign * Ret_Integer;  
      
    return Ret_Integer;  
}  


