

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

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




#define LOGFILE_NAME		".log"
#define PRINTF_CHAR_MAX		256

static FILE * pLogFiles = NULL;
static FILE* get_logs_handler(void) {
	if (pLogFiles == NULL) {
		pLogFiles = fopen(LOGFILE_NAME, "w");
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
	char szPrint[PRINTF_CHAR_MAX] = {0};
	FILE *fp = get_logs_handler();

	va_list ap;
	va_start(ap, fmt);
	iret = vsnprintf(szPrint, PRINTF_CHAR_MAX, fmt, ap);
	va_end(ap);

	i = fwrite(szPrint, 1, iret, fp);
	fflush(fp);

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


