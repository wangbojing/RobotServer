
/*
 *  gcc -o SyncDataToRedis SyncDataToRedis.c ApolloRedis.c -lgearman -ljson-c -lhiredis
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include <json-c/json_tokener.h>
#include <libgearman/gearman.h>

#include "ApolloRedis.h"

#define EAGLE_USER_ID		"UserId"
#define EAGLE_USER_NAME		"UserName"
#define EAGLE_DEVICE_ID		"DeviceId"


#define CMD_LENGTH			128

enum {
	USERINFO = 0x0,
	USERANDDEVICERELATIONSHIP = 0x1,
};

int parserJsonTokener(char *json, char *valueId, char *valueName, char type) {
	uint8_t i = 0;
	struct json_object *json_object = NULL;
	struct json_object *IdTokener = NULL;
	struct json_object *NameTokener = NULL;

	
	json_object = json_tokener_parse(json);
	if (is_error(json_object)) {
		printf("error\n");
		return -1;
	}

	if (type == USERINFO) {
		NameTokener = json_object_object_get(json_object, EAGLE_USER_NAME);
		IdTokener = json_object_object_get(json_object, EAGLE_USER_ID);
	} else if (type == USERANDDEVICERELATIONSHIP) {
		NameTokener = json_object_object_get(json_object, EAGLE_DEVICE_ID);
		IdTokener = json_object_object_get(json_object, EAGLE_USER_ID);
	}
	
	strcpy(valueName, json_object_to_json_string(NameTokener));
	strcpy(valueId, json_object_to_json_string(IdTokener));
	for (i = 1;i < 20 && valueName[i] != '"';i ++) {
		valueName[i-1] = valueName[i];
	}
	valueName[i-1] = 0;
	valueName[i] = 0;

	return 0;
}

// sync_UserInfo
/*
 * Create one pair(key, value), (U_{UserName}, U_Id)
 */
void *sync_UserInfo(gearman_job_st *job, void *context, size_t *result_size, gearman_return_t *ret_ptr) {
	uint8_t *result, i;
	const uint8_t *workload;
	char userId[20] = {0}, userName[20] = {0};
	char UUserName[20] = {0};

	char json_buffer[CMD_LENGTH] = {0};
	
	workload = gearman_job_workload(job);
	*result_size = gearman_job_workload_size(job);
	result = malloc(*result_size);

	printf(" sync_UserInfo : %s\n", workload);
	for (i = 0;i < *result_size;i ++) {
		json_buffer[i] = (char)workload[i];
	}

	parserJsonTokener(json_buffer, userId, userName, USERINFO);

	printf("userId:%s, userName:%s\n", userId, userName);
	sprintf(UUserName, "U_%s", userName);
	if (-1 == set_key_toredis(UUserName, userId)) {
		//return -2;
		return result;
	}

	*ret_ptr = GEARMAN_SUCCESS;
	return result;
}

//sync_DeviceInfo
/*
 * {D_{DeviceId}, }
 */

//sync_UserAndDeviceRelationShip 
/*
 * Create two pairs(key, value), (R_{DeviceId}, UserId) and (RP_{UserId}, DeviceId)
 */
void *sync_UserAndDeviceRelationShip(gearman_job_st *job, void *context, size_t *result_size, gearman_return_t *ret_ptr) {
	uint8_t *result, i;
	const uint8_t *workload;
	char userId[20] = {0}, deviceId[20] = {0};
	char RPUserId[20] = {0}, RDeviceId[20] = {0};
	
	char json_buffer[CMD_LENGTH] = {0};
	struct json_object *json_object = NULL;
	struct json_object *userIdTokener = NULL;
	struct json_object *deviceIdTokener = NULL;

	workload = gearman_job_workload(job);
	*result_size = gearman_job_workload_size(job);
	result = malloc(*result_size);

	printf(" sync_UserAndDeviceRelationShip : %s\n", workload);
	for (i = 0;i < *result_size;i ++) {
		json_buffer[i] = (char)workload[i];
	}

	parserJsonTokener(json_buffer, userId, deviceId, USERANDDEVICERELATIONSHIP);

	printf("userId:%s, deviceId:%s\n", userId, deviceId);
	sprintf(RPUserId, "RP_%s", userId);
	if (-1 == set_key_toredis(RPUserId, deviceId)) {
		//return -2;
		return result;
	}

	sprintf(RDeviceId, "R_%s", deviceId);
	if (-1 == set_key_toredis(RDeviceId, userId)) {
		//return -2;
		return result;
	}

	*ret_ptr = GEARMAN_SUCCESS;
	return result;
}


void *syncToRedis(gearman_job_st *job, void *context, size_t *result_size, gearman_return_t *ret_ptr) {
	uint8_t *result;
	const uint8_t *workload;

	workload = gearman_job_workload(job);
	*result_size = gearman_job_workload_size(job);
	result = malloc(*result_size);

	printf(" aaa : %s\n", workload);

	return result;
}


int main() {
	gearman_return_t ret;
	gearman_worker_st *worker = gearman_worker_create(NULL);
	gearman_worker_add_server(worker,"127.0.0.1",0);

	gearman_worker_add_function(worker,"syncToRedis",30, syncToRedis, NULL);
	gearman_worker_add_function(worker,"sync_UserInfo",30, sync_UserInfo, NULL);
	gearman_worker_add_function(worker,"sync_UserAndDeviceRelationShip",30, sync_UserAndDeviceRelationShip, NULL);

	while(1) {
		ret = gearman_worker_work(worker);
		if (ret != GEARMAN_SUCCESS) {
			printf("%s\n", gearman_worker_error(worker));
			break;
		}
	}
}


