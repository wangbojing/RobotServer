

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "ApolloProtocol.h"
#include "ApolloRedis.h"
#include "ApolloUtil.h"

#define POSTURL    "http://api.haoservice.com/api/viplbs"
#define POSTFIELDS "requestdata={\"celltowers\":[{\"cell_id\":\"26353\",\"lac\":\"24802\",\"mcc\":\"460\",\"mnc\":\"0\",\"signalstrength\":\"-60\"},{\"cell_id\":\"14454\",\"lac\":\"22831\",\"mcc\":\"460\",\"mnc\":\"0\",\"signalstrength\":\"-71\"},{\"cell_id\":\"11433\",\"lac\":\"22831\",\"mcc\":\"460\",\"mnc\":\"0\",\"signalstrength\":\"-75\"},{\"cell_id\":\"10326\",\"lac\":\"22831\",\"mcc\":\"460\",\"mnc\":\"0\",\"signalstrength\":\"-76\"}],\"wifilist\":[{\"macaddress\":\"00:0b:0e:7d:17:82\",\"time\":\"0\",\"singalstrength\":\"-8\"},{\"macaddress\":\"00:0b:0e:7d:17:84\",\"time\":\"0\",\"singalstrength\":\"-8\"}],\"mnctype\":\"gsm\"}&type=0&key=ade0044a7a6b49adbbf1e490bf8f0e9c"
#define FILENAME   "curlposttest.log"

#define LONGITUDE_KEY	 	"longitude"
#define LATITUDE_KEY		"latitude"


#define POST_DATA_LENGTH 	1024
#define LBS_DATA_LENGTH	34
#define LBS_COUNT			6
#define WIFI_COUNT			4
#define PATTERN_LENGTH		16

#define LBS_MCC_MNC_INDEX	10
#define LBS_STATION_INFO	14

void compute_prefix(unsigned char *pattern, unsigned char *array, int length) {
	int k = 0, q = 0;
	array[0] = 0;

	for (q = 1;q < length;q ++) {
		while (k > 0 && pattern[k+1] != pattern[q]) {
			k = array[k];
			
			if (pattern[k+1] == pattern[q])
				k ++;
			array[q] = k;
		}
	}
}

int kmp_matcher(unsigned char *t, unsigned char *pattern, int t_length, int pattern_length) {
	unsigned char array[PATTERN_LENGTH] = {0};
	int q = 0, i = 0;

	compute_prefix(pattern, array, pattern_length);
	for (i = 0;i < t_length;i ++) {
		while (q > 0 && pattern[q+1] != t[i]) {
			q = array[q];

			if (pattern[q+1] == t[i]) {
				q ++;
			}
			if (q == pattern_length) {
				//q = array[q];
				return i;
			}
		}
	}
	return -1;
}

int get_key_index(const char* buffer, char *key) {
	int i = 0, j = 0;
	int buffer_length = strlen(buffer);
	int key_length = strlen(key);
	for (i = 0;i < buffer_length-key_length;i ++) {
		for (j = 0;j < key_length;j ++) {
			if (*(buffer+i+j) != *(key+j)) break;
		}
		if (j == key_length) return i+j;
	}
	return -1;
}

void get_key(const char* buffer, char *value) {
	int i = 0;
	while(*(buffer+i) != ',') {
		value[i] = buffer[i];
		i ++;
	}
}

extern const char* get_value_fromjson(const char *key, const char *json) ;
extern void* ubloxDownloadApgs(void* arg);

size_t handle_data(void* buffer,size_t size,size_t nmemb,void *stream){
	//FILE *fptr = (FILE*)stream;	
	//unsigned char longitude[12] = {}
	int i = 0;
	LocationInfo *locationInfo = (LocationInfo*)stream;
	char *p1 = "longitude";
	char *p2 = "latitude";
	char log[16] = {0};
	char lat[16] = {0};	
	int u8High = 0;
	char u8RelationShipDevice[LEN_DEVICE_ID*2+6] = {0};
	char u8RedisUserValue[REDIS_COMMAND_LENGTH] = {0};
	char u8RedisDeviceGpsInfo[REDIS_COMMAND_LENGTH] = {0};
	pthread_t tid = -1;
	int err;
	//extern void *http_post(void *data);

	//printf("%2x%2x%2x%2x\n", pgps->apollo_id[0], pgps->apollo_id[1], pgps->apollo_id[2], pgps->apollo_id[3]);

	printf("buffer:%s\n", (char*)buffer);
	int index = get_key_index(buffer, p1);
	get_key((char*)(buffer+index+2), log);
	

	index = get_key_index(buffer, p2);
	get_key((char*)(buffer+index+2), lat);
	printf(" result longitude: %s, latitude:%s\n", log, lat);

	if (!strcmp(log, "null") || !strcmp(lat,"null") 
		|| !strcmp(log, "location") || !strcmp(lat, "location")
		|| !strcmp(log, "\"location\":null")|| !strcmp(lat, "\"location\":null")) {
		printf("data format error\n");
		return size*nmemb;
	}
	//strcpy(pgps->longitude, )
	memset(locationInfo->u8Longitude, 0, 2*LEN_LONGITUDE_DATA+2);
	memset(locationInfo->u8Latitude, 0, 2*LEN_LONGITUDE_DATA+2);
	locationInfo->u8Longitude[0] = 'E';
	locationInfo->u8Latitude[0] = 'N';
	locationInfo->u8Latitude[1] = '0';
		
	for (i = 1;i < 11;i ++) {
		locationInfo->u8Longitude[i] = log[i-1];
		if (i == 11) break;
		locationInfo->u8Latitude[i+1] = lat[i-1];
	}
	locationInfo->u8Longitude[i] = 0;
	locationInfo->u8Latitude[i+1] = 0;

	sprintf(u8RelationShipDevice, "R_%s", locationInfo->u8DeviceId);
	apollo_printf("u8RelationShipDevice:%s\n", u8RelationShipDevice);
	if (-1 == get_value_fromredis(u8RelationShipDevice, u8RedisUserValue)) { // device id don't bind
		return -1;
	}

	if (locationInfo->u8UbloxFlag) {
		err = pthread_create(&tid, NULL, ubloxDownloadApgs, (void *)locationInfo);
		if(0 != err)
			fprintf(stderr, "Couldn't run thread numbe, errno %d\n", err);
		else
			fprintf(stderr, "Thread\n");
	}
	//sprintf(u8High, "%d", 0);
	u8High = 0;
	//insert mysql 
	insertDeviceGps(locationInfo->u8Longitude, locationInfo->u8Latitude, u8High, locationInfo->u8DeviceId);
	
	sprintf(u8RedisDeviceGpsInfo, "G_%s_%s_%d_1_%ld", locationInfo->u8Longitude, locationInfo->u8Latitude, 
	u8High, *((long*)(locationInfo->u8TimeStamp)));
	apollo_printf(" GPS : %s\n", u8RedisDeviceGpsInfo);
	if (-1 == set_key_toredis(u8RedisUserValue, u8RedisDeviceGpsInfo)) {
		return -2;
	}
	//insert_gps(*pgps);
	//fwrite(buffer,size,nmemb,fptr);	
	return size*nmemb;
}


void parser_Lbs(unsigned char *buffer, Lbs* lbs) {
	int i = 0;

	
	for (i = 0;i < LBS_COUNT;i ++) {
		lbs[i].mcc = buffer[LBS_MCC_MNC_INDEX] *256 + buffer[LBS_MCC_MNC_INDEX+1];//*((unsigned short*)(buffer+14)); //
		lbs[i].mnc = buffer[LBS_MCC_MNC_INDEX+2] *256 + buffer[LBS_MCC_MNC_INDEX+3];//*((unsigned short*)(buffer+16));//

		lbs[i].lac = buffer[LBS_STATION_INFO+5*i] * 256 + buffer[LBS_STATION_INFO+5*i+1];//*((unsigned short*)(buffer+18+5*i));//
		lbs[i].cid = buffer[LBS_STATION_INFO+5*i+2] * 256 + buffer[LBS_STATION_INFO+5*i+3];//*((unsigned short*)(buffer+18+5*i+2));//
		if (lbs[i].cid == 0xFFFF || lbs[i].lac == 0xFFFF) continue;
		lbs[i].signal = buffer[LBS_STATION_INFO+5*i+4]; 
		if (lbs[i].lac == 0x0 || lbs[i].lac == 0xFF || lbs[i].cid == 0x0 || lbs[i].cid == 0xFF || lbs[i].signal == 0x0 || lbs[i].signal == 0xFF) {
			lbs[i].lac = 0x0;
			lbs[i].cid = 0x0;
			lbs[i].signal = 0x0;
			lbs[i].mcc = 0x0;
			lbs[i].mnc = 0x0;
		}
		printf("mcc:%d, mnc:%d, lac:%d, cid:%d, signal:-%d\n", lbs[i].mcc, lbs[i].mnc, lbs[i].lac , lbs[i].cid, lbs[i].signal );
	}
	return ;
}

int parser_wifi(unsigned char *buffer, unsigned char *wifi) {
	
}

char IsLbs(Lbs lbs) {
	if (lbs.mcc == 0x0 && lbs.mnc== 0x0 && lbs.cid == 0x0 && lbs.lac == 0x0 && lbs.signal == 0x0) {
		return 0;
	} else {
		return 1;
	}
}

char lsWifi(unsigned char *wifi) {
	int i = 0;
	for (i = 0;i < 7;i ++) {
		if (wifi[i] != 0x0) {
			return 1;
		}
	}
	return 0;
}

int encode_lbs(unsigned char *buffer, Lbs *lbs) {
	unsigned char lbs_item[128] = {0};
	int i = 0, index = 0;

	for (i = 0;i < LBS_COUNT;i ++) {
		if (!IsLbs(lbs[i])) continue;
		
		if (i == 0)
			sprintf(lbs_item, "{\"cell_id\":\"%d\",\"lac\":\"%d\",\"mcc\":\"%d\",\"mnc\":\"%d\",\"signalstrength\":\"-%d\"}", lbs[i].cid, lbs[i].lac, lbs[i].mcc, lbs[i].mnc, lbs[i].signal);
		else
			sprintf(lbs_item, ",{\"cell_id\":\"%d\",\"lac\":\"%d\",\"mcc\":\"%d\",\"mnc\":\"%d\",\"signalstrength\":\"-%d\"}", lbs[i].cid, lbs[i].lac, lbs[i].mcc, lbs[i].mnc, lbs[i].signal);
		strcpy(buffer+index, lbs_item);

		index += strlen(lbs_item);
		memset(lbs_item, 0, 128);
	}

	return index;
}



int encode_wifi(unsigned char *buffer, unsigned char *wifi) {
	unsigned char wifi_item[128] = {0};
	int i = 0, index = 0;

	for (i = 0;i < WIFI_COUNT;i ++) {
		printf("wifi:%2x\n", *(wifi+7*i));
		if (!lsWifi(wifi+7*i)) continue;
		if (i == 0)
			sprintf(wifi_item, "{\"macaddress\":\"%2x:%2x:%2x:%2x:%2x:%2x\",\"time\":\"0\",\"singalstrength\":\"%d\"}", wifi[i*7], wifi[i*7 + 1], wifi[i*7 + 2], wifi[i*7 + 3], 
				wifi[i*7 + 4], wifi[i*7 + 5], wifi[i*7 + 6]);
		else
			sprintf(wifi_item, ",{\"macaddress\":\"%2x:%2x:%2x:%2x:%2x:%2x\",\"time\":\"0\",\"singalstrength\":\"%d\"}", wifi[i*7], wifi[i*7 + 1], wifi[i*7 + 2], wifi[i*7 + 3], 
				wifi[i*7 + 4], wifi[i*7 + 5], wifi[i*7 + 6]);
		strcpy(buffer+index, wifi_item);

		index += strlen(wifi_item);
		memset(wifi_item, 0, 128);
	}
}

int encode_post_data(unsigned char *buffer, unsigned char *data) {
	Lbs lbs[LBS_COUNT];
	unsigned char lbs_buffer[POST_DATA_LENGTH] = {0};
	unsigned char wifi_buffer[POST_DATA_LENGTH] = {0};
	int i = 0;

	parser_Lbs(data, lbs);
//lbs
//{\"cell_id\":\"26353\",\"lac\":\"24802\",\"mcc\":\"460\",\"mnc\":\"0\",\"signalstrength\":\"-60\"}
	encode_lbs(lbs_buffer, lbs);

//wifi
//{\"macaddress\":\"00:0b:0e:7d:17:82\",\"time\":\"0\",\"singalstrength\":\"-8\"}	
#if 0
	encode_wifi(wifi_buffer, data+35);

	sprintf(buffer, "requestdata={\"celltowers\":[%s],\"wifilist\":[%s],\"mnctype\":\"gsm\"}&type=0&key=ade0044a7a6b49adbbf1e490bf8f0e9c", lbs_buffer, wifi_buffer);
#else	
	//encode_wifi(wifi_buffer, data+35);
	sprintf(buffer, "requestdata={\"celltowers\":[%s],\"mnctype\":\"gsm\"}&type=0&key=ade0044a7a6b49adbbf1e490bf8f0e9c", lbs_buffer);
#endif
//requestdata={\"celltowers\":[%s],],\"wifilist\":[%s],\"mnctype\":\"gsm\"}&type=0&key=ade0044a7a6b49adbbf1e490bf8f0e9c"
}

//extern void parse_gps_header_packet(byte *req, gps_data *gps) ;

void *http_post(void *data){	
	CURL *curl;	
	CURLcode res;	
	unsigned char buffer[POST_DATA_LENGTH];
	LocationInfo *pLocationInfo = (LocationInfo*)malloc(sizeof(LocationInfo));
	memset(pLocationInfo, 0, sizeof(LocationInfo));
	
	curl = curl_easy_init();	
	if (!curl)	{		
		fprintf(stderr,"curl init failed\n");		
		return NULL;	
	}	

	pLocationInfo->u8UbloxFlag = ((*(char*)(data+IDX_DEVICE_LOCATIONTYPE)) & 0xF0);
	hextostring(pLocationInfo->u8DeviceId ,(char*)(data+IDX_DEVICE_ID), LEN_DEVICE_ID);
	memcpy(pLocationInfo->u8TimeStamp,  data+44, 4);
	printf("%s\n", pLocationInfo->u8DeviceId);
	encode_post_data(buffer, data);
	printf("url:%s\n", buffer);
	
	curl_easy_setopt(curl,CURLOPT_URL,POSTURL); 
	curl_easy_setopt(curl,CURLOPT_POSTFIELDS,buffer); 
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,handle_data); 
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,pLocationInfo); 
	curl_easy_setopt(curl,CURLOPT_POST,1); 
	
	res = curl_easy_perform(curl);	
	if (res != CURLE_OK)	{		
		switch(res)		{			
			case CURLE_UNSUPPORTED_PROTOCOL:				
				fprintf(stderr,"CURLE_UNSUPPORTED_PROTOCOL\n");			
			case CURLE_COULDNT_CONNECT:				
				fprintf(stderr,"CURLE_COULDNT_CONNECT\n");			
			case CURLE_HTTP_RETURNED_ERROR:				
				fprintf(stderr,"CURLE_HTTP_RETURNED_ERROR\n");			
			case CURLE_READ_ERROR:				
				fprintf(stderr,"CURLE_READ_ERROR\n");			
			default:				
				fprintf(stderr,"default %d\n",res);		
		}		
		return NULL;	
	}	
	curl_easy_cleanup(curl);

	
	return NULL;
}
