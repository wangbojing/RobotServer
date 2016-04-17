
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <event.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


#include "ApolloUtil.h"
#include "ApolloRedis.h"
#include "ApolloProtocol.h"
#include "ApolloSqlOperator.h"
#include "ApolloSyner.h"


int checkPhoneCommand(unsigned char Cmd) {
	if (Cmd == CMD_PHONE_BUZZER_DEVICE_OPEN
		|| Cmd == CMD_PHONE_BUZZER_DEVICE_CLOSE
		|| Cmd == CMD_PHONE_LIGHT1_DEVICE_OPEN
		|| Cmd == CMD_PHONE_LIGHT1_DEVICE_CLOSE
		|| Cmd == CMD_PHONE_LIGHT2_DEVICE_OPEN
		|| Cmd == CMD_PHONE_LIGHT2_DEVICE_CLOSE) {
		return 8;
	} else if (Cmd == CMD_PHONE_REGISTER_USER
		|| Cmd == CMD_PHONE_SIGNIN_USER
		|| Cmd == CMD_PHONE_UPDATE_PWD) {
		return 7;
	} else if (Cmd == CMD_PHONE_DEVICE_ALARMMUSIC_OPEN
		|| Cmd == CMD_PHONE_DEVICE_ALARMMUSIC_CLOSE
		|| Cmd == CMD_PHONE_DEVICE_SAFEZONEMUSIC_OPEN
		|| Cmd == CMD_PHONE_DEVICE_SAFEZONEMUSIC_CLOSE) {
		return 6;
	} else if (Cmd == CMD_PHONE_DEVICE_SOS_MSGREMAINER_OPEN
		|| Cmd == CMD_PHONE_DEVICE_SOS_MSGREMAINER_CLOSE
		|| Cmd == CMD_PHONE_DEVICE_POWER_MSGREMAINDER_OPEN
		|| Cmd == CMD_PHONE_DEVICE_POWER_MSGREMAINDER_CLOSE) {
		return 5;
	} else if (Cmd == CMD_PHONE_DEVICE_FAMILYNUMBER_SET 
		|| Cmd == CMD_PHONE_DEVICE_CONTRACT_SET
		|| Cmd == CMD_PHONE_DEVICE_PHONEBOOK_SET
		|| Cmd == CMD_PHONE_DEVICE_FREQ_SET
		) {
		return 4;
	} else if (Cmd == CMD_PHONE_BIND_DEVICE
		|| Cmd == CMD_PHONE_UNBIND_DEVICE) {
		return 3;
	} else if (Cmd == CMD_PHONE_DEVICE_SOS_CLEAR) {
		return 2;
	} else if (Cmd == CMD_PHONE_DEVICE_RESET) {
		return 1;
	} else if (Cmd == CMD_PHONE_DEVICE_DOWNLOAD
		|| Cmd == CMD_PHONE_DEVICE_REALTIME_LOCATION
		|| Cmd == CMD_PHONE_DEVICE_COUNTSTEP_OPEN
		|| Cmd == CMD_PHONE_DEVICE_COUNTSTEP_CLOSE
		|| Cmd == CMD_PHONE_DEVICE_SLEEPQUILTY_OPEN
		|| Cmd == CMD_PHONE_DEVICE_SLEEPQUILTY_CLOSE
		|| Cmd == CMD_PHONE_DEVICE_ALARM
		|| Cmd == CMD_PHONE_DEVICE_SAFEZONE_ALARM
		|| Cmd == CMD_PHONE_DEVICE_LONGSAT_ALARM
		|| Cmd == CMD_PHONE_DEVICE_LOWPOWER_ALARM) {
		return 0;
	} else {
		return -1;
	}
}
/*
 * hex: 48 20 16 01 05 00 00 00 07 08 34 56 67 78 21 48 20 16 01 05 00 00 00 00 07 08 34 56 67 78 21 98
 */
int deviceHeartPacket(char *packet, int length, char* info) {
	char u8DeviceId[LEN_DEVICE_ID*2+1] = {0};
	char u8RelationShipDevice[LEN_DEVICE_ID*2+6] = {0};
	char u8RedisUserValue[REDIS_COMMAND_LENGTH] = {0};
	char u8RedisDeviceInfo[REDIS_COMMAND_LENGTH] = {0};
	char u8DeviceInfo[REDIS_COMMAND_LENGTH] = {0};

	if(length != LEN_HEART_PACKET_DEVICE) {
		return -3;
	}
	
	//device id hex to string				
	hextostring(u8DeviceId ,packet+IDX_DEVICE_ID, LEN_DEVICE_ID);
	//get phoneid by device id	
	sprintf(u8RelationShipDevice, "R_%s", u8DeviceId);
	apollo_printf(" u8RelationShipDevice:%s\n", u8RelationShipDevice);
	if (-1 == get_value_fromredis(u8RelationShipDevice, u8RedisUserValue)) { // device id don't bind
		return -1;
	}
	apollo_printf(" timeDiff: %d\n", *(int*)(packet+24));
	
#if ENABLE_FALLDOWN_HTTPPOST
	if((*(packet+10) & 0x0F)) {
		extern void *http_device_falldown(void *data);
		pthread_t tid = -1;
		int err = pthread_create(&tid, NULL, http_device_falldown, (void *)packet);
		if(0 != err)
			fprintf(stderr, "Couldn't run thread numbe, errno %d\n", err);
		else
			fprintf(stderr, "Thread\n");
	}
#endif
	//set device info {device->User: 1->1}
	hextostring(u8DeviceInfo, packet+IDX_DEVICE_POWER, LEN_DEVICE_INFO);
	sprintf(u8RedisDeviceInfo, "I_%s_%d", u8DeviceInfo, *((int*)(packet+IDX_DEVICE_HEARTPACKET_TIMESTAMP)));
	apollo_printf(" u8RedisDeviceInfo:%s, u8RedisUserValue:%s\n", u8RedisDeviceInfo, u8RedisUserValue);
	if (-1 == set_key_toredis(u8RedisUserValue, u8RedisDeviceInfo)) {
		return -2;
	}

	//get User Cmd info
	memset(u8RedisDeviceInfo, 0, REDIS_COMMAND_LENGTH);
	if (-1 == get_value_fromredis(u8DeviceId, u8RedisDeviceInfo)) {
		return 0; //No Cmd 
	} else {
		int len = strlen(u8RedisDeviceInfo);
		if (strlen(u8RedisDeviceInfo) < 4) {
			int cmd = atoi(u8RedisDeviceInfo);
			apollo_printf("cmd:%s\n", u8RedisDeviceInfo);
			del_redis_key(u8DeviceId);
			return (cmd < 0 || cmd > 254) ? -4 : cmd;
		} else {
			int Count = 0, i;
			int cmd = 0;
			int index = 0;
			char u8PhoneNumber[LEN_PHONE_NUMBER+1];
			char **Param = (char**)malloc(sizeof(char**));
			
			Separation('_', u8RedisDeviceInfo, &Param, &Count);
			
			//printf(" Count:%d\n", Count);
			cmd = atoi(Param[0]); //cmd
			apollo_printf(" int cmd:%d", cmd);
			info[IDX_DEVICE_RETURN_COMMAND] = (char)cmd;
			
			if ((info[IDX_DEVICE_RETURN_COMMAND] & 0xF0) == 0x40) {
				index = atoi(Param[1]);
				info[IDX_DEVICE_RETURN_INDEX] = (char)index;
				if (Count == 3){ //phone number
					//int phoneLen = strlen(Param[2]);
					int i = 0;
					for (i = 0;i < LEN_PHONE_NUMBER;i ++) {
						u8PhoneNumber[i] = STRINGTOHEX(Param[2][2*i], Param[2][2*i+1]);
						//printf("%c%c %x \n",Param[2][2*i], Param[2][2*i+1], u8PhoneNumber[i]);
					}
					memcpy(info+IDX_DEVICE_RETURN_PHNUM, u8PhoneNumber, LEN_PHONE_NUMBER);
				}
			}
			del_redis_key(u8DeviceId);

			for (i = 0;i < Count;i ++) {
				free(Param[i]);
			}

			free(Param);
			apollo_printf(" aaaaa ");
			return cmd & 0xF0;
		}
	}
	return 0;
}

/*
 * 50 00 00 00 07 25 25 25 
 */
int phoneHeaderPacket(char *packet, int length, char *info) {
	char u8PhoneId[LEN_PHONE_ID*2+1] = {0};
	char PhoneId[LEN_PHONE_ID*2+1] = {0};
	
	if (length != LEN_HEART_PACKET_PHONE) {
		return -3;
	}
	//phone id hex to string				
	hextostring(u8PhoneId ,packet+IDX_PHONE_ID, LEN_PHONE_ID);
	//get device gps and high info or device power info
	sprintf(PhoneId, "%d", atoi(u8PhoneId));
	//printf("PhoneId:%s", PhoneId);
	get_value_fromredis(PhoneId, info);

	del_redis_key(PhoneId);

	return strlen(info);
}

/*
 * 43 00 00 00 07 02 00 00 00 00 00 00 23 45 43 32 //download blockdata
 * 43 00 00 00 07 41 01 00 86 18 87 41 87 42 90 43 //set family number
 * 43 00 00 00 07 42 01 00 86 18 87 56 87 42 90 43 //set contact number
 * 43 00 00 00 16 43 01 00 86 18 87 41 87 42 90 43 //set phone book
 * 43 00 00 00 07 44 01 00 86 18 87 41 87 42 90 43 //set location freq
 * 43 00 00 00 07 08 00 86 18 87 41 87 42 90 43 01//single cmd open 
 * 43 00 00 00 07 09 00 86 18 87 41 87 42 90 43 01//single cmd open 
 * 43 00 00 00 07 0A 00 86 18 87 41 87 42 90 43 01//single cmd open 
 * 43 00 00 00 07 0B 00 86 18 87 41 87 42 90 43 01//single cmd open 
 * 43 00 00 00 07 03 00 86 18 87 41 87 42 90 43 01//single cmd open 
 * 43 00 00 00 07 05 00 86 18 87 41 87 42 90 43 01//single cmd open 
 */
int phoneCommandPacket(char *packet, int length) {
	char u8PhoneId[LEN_PHONE_ID*2+1] = {0};
	char u8DeviceId[LEN_DEVICE_ID*2+1] = {0};
	char u8RelationShipPhone[LEN_PHONE_ID*2+6] = {0};
	char u8Cmd[6] = {0}, i,j = 0;
	
	if (length != LEN_COMMAND_PACKET) {
		return -3;
	}
	//phone id hex to string				
	hextostring(u8PhoneId ,packet+IDX_PHONE_ID, LEN_PHONE_ID);
	//get device id from phoneid
	sprintf(u8RelationShipPhone, "RP_%d", atoi(u8PhoneId));
	if (-1 == get_value_fromredis(u8RelationShipPhone, u8DeviceId)) { // device id don't bind
		return -1;
	}
	//set phone cmd
	apollo_printf(" cmd:%d, %x\n", checkPhoneCommand((unsigned char)*(packet+IDX_PHONE_COMMAND)), 
		((unsigned char)*(packet+IDX_PHONE_COMMAND)));
	if (4 == checkPhoneCommand(*(packet+IDX_PHONE_COMMAND))) { //set contract number
		char u8Value[48] = {0};
		char u8PhoneNumber[LEN_PHONE_NUMBER*2+2] = {0};
		if (*(packet+IDX_PHONE_COMMAND) != CMD_PHONE_DEVICE_FREQ_SET) {
			hextostring(u8PhoneNumber ,packet+IDX_PHONE_CMD_NUMBER, LEN_PHONE_NUMBER);
			sprintf(u8Value, "%d_%d_%s", *(packet+IDX_PHONE_COMMAND), *(packet+IDX_PHONE_CMD_INDEX), u8PhoneNumber);
		} else {
			sprintf(u8Value, "%d_%d", *(packet+IDX_PHONE_COMMAND), *(packet+IDX_PHONE_CMD_INDEX));
		}
		printf("key:%s, value:%s\n",u8DeviceId, u8Value);
		if (-1 == set_key_toredis(u8DeviceId, u8Value)) {
			return -2;
		}
	} else if (7 == checkPhoneCommand(*(packet+IDX_PHONE_COMMAND))) { //register user
		//set 
	} else if (3 == checkPhoneCommand(*(packet+IDX_PHONE_COMMAND))) { //bind device
		//
	} else if (0 <= checkPhoneCommand(*(packet+IDX_PHONE_COMMAND))) { //single cmd
		sprintf(u8Cmd, "%d", (unsigned char)*(packet+IDX_PHONE_COMMAND));
		if (*(packet+IDX_PHONE_COMMAND) == CMD_PHONE_DEVICE_REALTIME_LOCATION) {
			apollo_printf("u8DeviceId:%s, u8PhoneId:%s\n", u8DeviceId, u8PhoneId);
			selectLocationNewItem(u8DeviceId, atoi(u8PhoneId));
		}
		if (-1 == set_key_toredis(u8DeviceId, u8Cmd)) {
			return -2;
		}
	} 
	
	return 0;
}

/*
 * 47 20 16 01 05 00 00 00 07 11 11 30 91 27 16 02 20 64 64 26 03 45 65 23 00 34 32 25 25 25 25 25
 * 47 20 16 01 05 00 00 00 07 12 12 30 56 65 23 03 43 56 32 67 03 45 65 23 00 34 32 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25
 */
extern void* ubloxDownloadApgs(void* arg);

int deviceLocationPacket(char *packet, int length) {
	char u8DeviceId[LEN_DEVICE_ID*2+1] = {0};
	char u8RelationShipDevice[LEN_DEVICE_ID*2+6] = {0};
	char u8RedisUserValue[REDIS_COMMAND_LENGTH] = {0};
	char u8RedisDeviceGpsInfo[REDIS_COMMAND_LENGTH] = {0};

	char u8Longitude[2*LEN_LONGITUDE_DATA+2] = {0};
	char u8Latitude[2*LEN_LATITUDE_DATA+2] = {0};
	char u8High[12] = {0};

	apollo_printf(" length : %d, %x\n", length, packet[IDX_DEVICE_LOCATIONTYPE]);
	if ((length != LEN_GPS_LOCATION_DATA) && (length != LEN_LAB_LOCATION_DATA)) {
		return -3;
	}
	//device id hex to string				
	hextostring(u8DeviceId ,packet+IDX_DEVICE_ID, LEN_DEVICE_ID);
	//get phoneid by device id	
	sprintf(u8RelationShipDevice, "R_%s", u8DeviceId);
	apollo_printf("u8RelationShipDevice:%s\n", u8RelationShipDevice);
	if (-1 == get_value_fromredis(u8RelationShipDevice, u8RedisUserValue)) { // device id don't bind
		return -1;
	}
	if ((length == LEN_GPS_LOCATION_DATA)) {		
		hextostring(u8Longitude+1, packet+IDX_DEVICE_GPS_LONGITUDE, LEN_LONGITUDE_DATA);
		u8Longitude[0] = 'E';
		u8Longitude[4] = '.';

		hextostring(u8Latitude+1, packet+IDX_DEVICE_GPS_LATITUDE, LEN_LATITUDE_DATA);
		u8Latitude[0] = 'N';
		u8Latitude[4] = '.';

		sprintf(u8High, "%d", *(int*)(packet+IDX_DEVICE_GPS_HIGH));

		if ((*(packet+IDX_DEVICE_LOCATIONTYPE)) & 0xF0) {
			int err;
			pthread_t tid = -1;
			LocationInfo *pLocationInfo = (LocationInfo*)malloc(sizeof(LocationInfo));
	
			strcpy(pLocationInfo->u8Longitude, u8Longitude);
			strcpy(pLocationInfo->u8Latitude, u8Latitude);
			strcpy(pLocationInfo->u8DeviceId, u8DeviceId);

			err = pthread_create(&tid, NULL, ubloxDownloadApgs, (void *)pLocationInfo);
			if(0 != err)
				fprintf(stderr, "Couldn't run thread numbe, errno %d\n", err);
			else
				fprintf(stderr, "Thread\n");
		}
		
		//insert mysql 
		insertDeviceGps(u8Longitude, u8Latitude, u8High, u8DeviceId);
		//printf(" aaa %x %x %x %x\n", packet[IDX_DEVICE_GPS_HIGH], packet[IDX_DEVICE_GPS_HIGH+1], 
		//	packet[IDX_DEVICE_GPS_HIGH+2], packet[IDX_DEVICE_GPS_HIGH+3]);
		sprintf(u8RedisDeviceGpsInfo, "G_%s_%s_%d_0_%ld", u8Longitude, u8Latitude, 
		*(int*)(packet+IDX_DEVICE_GPS_HIGH), *((long*)(packet+IDX_DEVICE_GPS_TIMESTAMP)));
		apollo_printf("key:%s, GPS : %s\n",u8RedisUserValue, u8RedisDeviceGpsInfo);
		if (-1 == set_key_toredis(u8RedisUserValue, u8RedisDeviceGpsInfo)) {
			return -2;
		}
	}else if ((length == LEN_LAB_LOCATION_DATA)) {  
		pthread_t tid = -1;

		extern void *http_post(void *data);
		int err = pthread_create(&tid, NULL, http_post, (void *)packet);
		if(0 != err)
			fprintf(stderr, "Couldn't run thread numbe, errno %d\n", err);
		else
			fprintf(stderr, "Thread\n");
	}

	return 0;
}

int phoneAddDevice(char *packet, int length) {
	return 0;
}

int phoneOperatorUserInfo(struct bufferevent *bev, client_t *client, char *packet, int length) {
	char u8PhoneId[LEN_PHONE_ID*2+1] = {0};
	char u8UserName[LEN_PHONE_USERNAME+1] = {0};
	//phone id hex to string				
	hextostring(u8PhoneId ,packet+IDX_PHONE_ID, LEN_PHONE_ID);
	memcpy(u8UserName, packet+IDX_PHONE_USERNAME, LEN_PHONE_USERNAME);
			
	switch (packet[IDX_PHONE_COMMAND]) {
		case CMD_PHONE_REGISTER_USER: {
			int u32UserId = 0;
			char u8Password[LEN_PHONE_PASSWORD+1] = {0};
			char retBuf[LEN_DEVICE_RETURN_BUFFER*2] = {0};
			retBuf[0] = 'R';
			retBuf[1] = 'E';
			retBuf[2] = 'T';
			retBuf[3] = 'O';
			retBuf[4] = 'K';
			retBuf[5] = ':';
			
			memcpy(u8Password, packet+IDX_PHONE_PASSWORD, LEN_PHONE_PASSWORD);
			
			if (OK == insertUser(u8UserName, u8Password)) {
				//return userid			
				if (OK == selectUserId(u8UserName, u8Password, &u32UserId)) {					
					memcpy(retBuf+IDX_PHONE_RETURN_USERID, &u32UserId, LEN_PHONE_ID);
					apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
				} else {					
					retBuf[3] = 'N';
					retBuf[4] = 'O';
					retBuf[IDX_PHONE_RETURN_USERID] = RET_PHONE_GETUSERID_FAILED;
					apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
				}
			} else {
				//return sign in failed
				retBuf[3] = 'N';
				retBuf[4] = 'O';
				retBuf[IDX_PHONE_RETURN_USERID] = RET_PHONE_SIGNIN_FAILED;
				apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
			}
			break;
		}
		case CMD_PHONE_SIGNIN_USER: {
			int u32UserId = 0;
			char u8Password[LEN_PHONE_PASSWORD+1] = {0};
			char retBuf[LEN_DEVICE_RETURN_BUFFER*2] = {0};
			retBuf[0] = 'R';
			retBuf[1] = 'E';
			retBuf[2] = 'T';
			retBuf[3] = 'O';
			retBuf[4] = 'K';
			retBuf[5] = ':';

			memcpy(u8Password, packet+IDX_PHONE_PASSWORD, LEN_PHONE_PASSWORD);
			if (OK == selectUserId(u8UserName, u8Password, &u32UserId)) {					
				memcpy(retBuf+IDX_PHONE_RETURN_USERID, &u32UserId, LEN_PHONE_ID);
				apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
			} else {					
				retBuf[3] = 'N';
				retBuf[4] = 'O';
				retBuf[IDX_PHONE_RETURN_USERID] = RET_PHONE_USERID_NOEXIST;
				apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
			}
			
			break;
		}
		case CMD_PHONE_UPDATE_PWD: {
			char u8Password[LEN_PHONE_PASSWORD+1] = {0};
			char retBuf[LEN_DEVICE_RETURN_BUFFER*2] = {0};
			retBuf[0] = 'R';
			retBuf[1] = 'E';
			retBuf[2] = 'T';
			retBuf[3] = 'O';
			retBuf[4] = 'K';
			retBuf[5] = ':';

			memcpy(u8Password, packet+IDX_PHONE_PASSWORD, LEN_PHONE_PASSWORD);

			if (OK == updatePassword(u8Password, u8UserName)) {	
				retBuf[IDX_PHONE_RETURN_USERID] = RET_PHONE_SUCCESS;
				apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
			} else {					
				retBuf[3] = 'N';
				retBuf[4] = 'O';
				retBuf[IDX_PHONE_RETURN_USERID] = RET_PHONE_UPDATE_PWD_FAILED;
				apolloReturnPacket(bev, client, retBuf, LEN_DEVICE_RETURN_BUFFER*2);
			}
			
			break;
		}
		case CMD_PHONE_BIND_DEVICE: {
			break;
		}
		case CMD_PHONE_UNBIND_DEVICE: {
			//delete mysql

			//delete redis key
			break;
		}
	}
	return 0;
}

int deviceBaseStation(char *packet, int length) {
	return 0;
}

// 
int deviceBlockDataRecvPacket(client_t *client, char *packet, int length) {
	int i = 0;
	int dataLength = packet[30] * 256 + (unsigned char)packet[31];
	int pktIndex = packet[13];
	int pktTotal = packet[29];
	char u8DeviceId[LEN_DEVICE_ID*2+1];
	char u8FileName[LEN_DEVICE_ID*4];

	apollo_printf("pkt_13:%d, pkt_29:%d, pkt_30:%d, pkt_31:%d\n", packet[13], packet[29], packet[30], (unsigned char)packet[31]);
	if (((client->pBlock->index + length) == LEN_BLOCK_PACKET) && ((pktIndex + 1) != pktTotal)) {
		apollo_printf(" copy index : %d, dataLength:%d\n", client->pBlock->index, dataLength);
		client->pBlock->index = 0;
		for (i = 0;i < dataLength;i ++) {
			client->pBlock->buffer[pktIndex * (LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)+i] = packet[LEN_HEADER_LENGTH+i];
		}
		memset(packet, 0, LEN_BLOCK_PACKET);
		apollo_printf(" aaaa index : %d, dataLength:%d\n", client->pBlock->index, dataLength);
		
	} else {
		apollo_printf(" index:%d\n", client->pBlock->index);
		client->pBlock->index += length;
	}
	//the last packet
	if (((pktIndex + 1) == pktTotal) && (length >= (dataLength + LEN_HEADER_LENGTH))) {
		for (i = 0;i < dataLength;i ++) {
			client->pBlock->buffer[pktIndex * (LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)+i] = packet[LEN_HEADER_LENGTH+i];
		}
		//save data to file
		hextostring(u8DeviceId ,packet+IDX_DEVICE_ID, LEN_DEVICE_ID);
		genBlockFilePathName(u8DeviceId, u8FileName);
		write_dat(u8FileName, client->pBlock->buffer, pktIndex * (LEN_BLOCK_PACKET - LEN_HEADER_LENGTH) + dataLength);

		//set redis key
		
	}

	return 0;
}

// 44 20 16 03 04 02 03 01 08 25 25 25 25 25 25 25 
int deviceBlockDataSendPacket(struct bufferevent *bev, client_t *client, char *packet, int length) {
	char u8DeviceId[LEN_DEVICE_ID*2+1] = {0};
	char u8FileName[LEN_DEVICE_ID*8] = {0};
	int totalLength = 0, sendLength = 0;

	if (length != LEN_DEVICE_DOWNLOAD) {
		return -3;
	}

	hextostring(u8DeviceId ,packet+IDX_DEVICE_ID, LEN_DEVICE_ID);
	if (*(packet+13) == 0x60) {
		genBlockFilePathName(u8DeviceId, u8FileName);
		apollo_printf("block FileName:%s\n", u8FileName);
	} else if(*(packet+13) == 0x61) {
		genAgpsFilePathName(u8DeviceId, u8FileName);
		apollo_printf("agps FileName:%s\n", u8FileName);
	} else {
		genBlockFilePathName(u8DeviceId, u8FileName);
		apollo_printf("block FileName:%s\n", u8FileName);
	}
	totalLength = takeDataPacket(u8FileName, client->pBlock->buffer, packet+1);
	apollo_printf("totalLength:%d\n", totalLength);
	while (sendLength < totalLength) {
		if ((totalLength - sendLength) >= LEN_BLOCK_PACKET) {
			evbuffer_add(client->output_buffer, client->pBlock->buffer+sendLength, LEN_BLOCK_PACKET);
			sendLength += LEN_BLOCK_PACKET;
		} else {
			evbuffer_add(client->output_buffer, client->pBlock->buffer+sendLength, (totalLength - sendLength));
			sendLength = totalLength;
		}
		
		if (bufferevent_write_buffer(bev, client->output_buffer)) {
			if (client != NULL) {						
				if (client->fd >= 0) {
					close(client->fd);
					client->fd = -1;
				}
			}
		}
	}
	return totalLength;
}

int apolloReturnPacket(struct bufferevent *bev, client_t *client, char *buffer, int length) {
	evbuffer_add(client->output_buffer, buffer, length);
	if (bufferevent_write_buffer(bev, client->output_buffer)) {
		closeClient(client);
	}
	return 0;
}



void apolloParsePacket(struct bufferevent *bev, client_t *client) {
	char *data = client->pBlock->packet;
	int nbytes, i;
	unsigned int blockIndex = client->pBlock->index;

	if (blockIndex > LEN_BLOCK_PACKET || blockIndex < 0) blockIndex = 0;
#if 0
	while ((nbytes = EVBUFFER_LENGTH(bev->input)) > 0) {
		if (nbytes > FRAME_LENGTH) nbytes = FRAME_LENGTH;
		evbuffer_remove(bev->input, data, nbytes); 
#else
	apollo_printf("cccc index:%d\n", client->pBlock->index);
	while ((nbytes = evbuffer_remove(bev->input, data + blockIndex, LEN_BLOCK_PACKET - blockIndex)) > 0) {
#endif
		apollo_printf(" blockIndex : %d, nbytes:%d, fd:%d, data[0]:0x%x\n", blockIndex, nbytes, client->fd, data[0]);
		
		apollo_printf("\n");
		switch (data[0]) {
			case HDR_HEART_PACKET_DEVICE: {
				char cmd;
				char setValueBuf[LEN_DEVICE_HEARTPACKET_RETURN_BUFFER] = {0};
				char retbuf[LEN_DEVICE_RETURN_BUFFER] = {'R', 'E', 'T', 'O', 'K', '%'};
				int result = ApolloProtocolInstance->deviceHeartPacket_Proc(data, nbytes, setValueBuf);
				if (result > 0) {	
					
					if ((result & 0xF0) == (CMD_PHONE_DEVICE_FAMILYNUMBER_SET & 0xF0)) {
						setValueBuf[0] = 'C';
						apolloReturnPacket(bev, client, setValueBuf, LEN_DEVICE_HEARTPACKET_RETURN_BUFFER);
					} else {
						cmd = (char)result;
						retbuf[0] = 'C';
						retbuf[IDX_DEVICE_RETURN_COMMAND] = cmd;						
						apolloReturnPacket(bev, client, retbuf, LEN_DEVICE_RETURN_BUFFER);
					}
				} else if (result == 0 || result == -1 || result == -2) {
					char retTimeBuf[LEN_DEVICE_RETURN_BUFFER*2+2] = {0};
					strcpy(retTimeBuf, "RETOK");
					writeTimeHeader(retTimeBuf+6);
					apolloReturnPacket(bev, client, retTimeBuf, LEN_DEVICE_RETURN_BUFFER*2+2);			
				} else {
					apollo_printf(" aaa [%s:%d] invaild key \n", __func__, __LINE__);
				}
				break;
			}
			case HDR_HEART_PACKET_PHONE: {
				char u8DeviceInfo[REDIS_COMMAND_LENGTH+4] = {0};
				int length = ApolloProtocolInstance->phoneHeaderPacket_Proc(data, nbytes, u8DeviceInfo+4);
				if (length < 0) {
					apollo_printf(" aaa [%s:%d] invaild key \n", __func__, __LINE__);
					break;
				}
				u8DeviceInfo[0] = 'R';
				u8DeviceInfo[1] = 'E';
				u8DeviceInfo[2] = 'T';
				u8DeviceInfo[3] = ':';

				apolloReturnPacket(bev, client, u8DeviceInfo, length+4);
				break;
			}
			case HDR_COMMAND_PACKET: {
				int length = ApolloProtocolInstance->phoneCommandPacket_Proc(data, nbytes);
				if (length < 0) {
					apollo_printf(" aaa [%s:%d] invaild key \n", __func__, __LINE__);
					break;
				}
				break;
			}
			case HDR_LOCATION_DATA: {
				char retbuf[LEN_DEVICE_RETURN_BUFFER] = {'R', 'E', 'T', 'O', 'K', '%'};
				ApolloProtocolInstance->deviceLocationPacket_Proc(data, nbytes);
				apolloReturnPacket(bev, client, retbuf, LEN_DEVICE_RETURN_BUFFER);	
				break;
			}
			case HDR_INFO_RETURN: {
				break;
			}
			case HDR_ADD_DEVICE: {
				ApolloProtocolInstance->phoneAddDevice_Proc(data, nbytes);
				break;
			}
			case HDR_OPERATOR_USER_INFO: {
				ApolloProtocolInstance->phoneOperatorUserInfo_Proc(bev, client, data, nbytes);
				break;
			}
			case HDR_BASE_STATION_DEVICE: {
				ApolloProtocolInstance->deviceBaseStation_Proc(data, nbytes);
				break;
			}
			case HDR_BLOCK_DATA_RECV: {
				int i = 0;
				int dataLength = data[30] * 256 + (unsigned char)data[31];
				int pktIndex = data[13];
				int pktTotal = data[29];
				char u8DeviceId[LEN_DEVICE_ID*2+1];
				char u8FileName[LEN_DEVICE_ID*4];
				char retbuf[LEN_DEVICE_RETURN_BUFFER] = {'S', 'E', 'T', 'O', 'K', '%'};				
				MulticastSynerPacket *pSynPacket = NULL;
				//ApolloProtocolInstance->deviceBlockDataRecvPacket_Proc(client, data, nbytes);

				apollo_printf("pkt_13:%d, pkt_29:%d, pkt_30:%d, pkt_31:%d\n", data[13], data[29], data[30], (unsigned char)data[31]);
				if (((blockIndex + nbytes) == LEN_BLOCK_PACKET) && ((pktIndex + 1) != pktTotal)) {
					apollo_printf(" copy index : %d, dataLength:%d\n", blockIndex, dataLength);
					for (i = 0;i < dataLength;i ++) {
						client->pBlock->buffer[pktIndex * (LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)+i] = data[LEN_HEADER_LENGTH+i];
					}
					memset(data, 0, LEN_BLOCK_PACKET+4);
					client->pBlock->index = 0;
					blockIndex = 0;
					//printf(" aaaa index : %d, dataLength:%d\n", client->pBlock->index, dataLength);
					
				} else {
					blockIndex += nbytes;
					client->pBlock->index = blockIndex;
					apollo_printf(" index:%d\n", blockIndex);					
				}
				//the last packet
				if (((pktIndex + 1) == pktTotal) && (blockIndex >= (dataLength + LEN_HEADER_LENGTH))) {
					for (i = 0;i < dataLength;i ++) {
						client->pBlock->buffer[pktIndex * (LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)+i] = data[LEN_HEADER_LENGTH+i];
					}
					//save data to file
					hextostring(u8DeviceId ,data+IDX_DEVICE_ID, LEN_DEVICE_ID);
					genBlockFilePathName(u8DeviceId, u8FileName);
					write_dat(u8FileName, client->pBlock->buffer, pktIndex * (LEN_BLOCK_PACKET - LEN_HEADER_LENGTH) + dataLength);

					apollo_printf(" save data \n");

					//return data to device
					apolloReturnPacket(bev, client, retbuf, LEN_DEVICE_RETURN_BUFFER);	
					//set redis key

					//multicast
					pSynPacket = (MulticastSynerPacket*)malloc(sizeof(MulticastSynerPacket));
					memset(pSynPacket, 0, sizeof(MulticastSynerPacket));
					strcpy(pSynPacket->u8DeviceId, u8DeviceId);
					pSynPacket->u8flag = MULTICAST_TYPE_AGPS;
					StartApolloSynerAction((void*)pSynPacket);
				}
				//printf("bbbb index:%d\n", client->pBlock->index);
				break;
			}
			case HDR_BLOCK_DATA_SEND: {
				ApolloProtocolInstance->deviceBlockDataSendPacket_Proc(bev, client, data, nbytes);
				break;
			}
			default : {
				/*
				for (i = 0;i < nbytes;i ++) {
					apollo_printf(" 0x%x", data[i]);
				}
				apollo_printf("\n");
				*/
			}
		}
			
	}

	return ;
}


void initApolloProtocol(void) {
	ApolloProtocolInstance = (struct ApolloProtocolProcess*)malloc(sizeof(struct ApolloProtocolProcess));
	ApolloProtocolInstance->phoneHeaderPacket_Proc = phoneHeaderPacket;
	ApolloProtocolInstance->deviceHeartPacket_Proc = deviceHeartPacket;
	ApolloProtocolInstance->phoneCommandPacket_Proc = phoneCommandPacket;
	ApolloProtocolInstance->deviceLocationPacket_Proc = deviceLocationPacket;
	ApolloProtocolInstance->deviceBaseStation_Proc = deviceBaseStation;
	ApolloProtocolInstance->phoneAddDevice_Proc = phoneAddDevice;
	ApolloProtocolInstance->phoneOperatorUserInfo_Proc = phoneOperatorUserInfo;
	ApolloProtocolInstance->deviceBlockDataRecvPacket_Proc = deviceBlockDataRecvPacket;
	ApolloProtocolInstance->deviceBlockDataSendPacket_Proc = deviceBlockDataSendPacket;

}



