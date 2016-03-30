

#include <hiredis/hiredis.h>
#include <string.h>

#include "ApolloRedis.h"
#include "ApolloUtil.h"


int set_key_toredis(const char *key, const char *value) {
	char command[REDIS_COMMAND_LENGTH] = {0};
	int ret = 0;
	
	sprintf(command, "set %s %s", key, value);
	redisContext *ctx = redisConnect(REDIS_HOST, REDIS_PORT);
	
	redisReply *reply = (redisReply*)redisCommand(ctx, command);
	if (!(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str, "OK") == 0)) {
		apollo_printf("Failed to execute command[%s]\n", command);		
		ret =  -1;
		goto set_exit;
	}	
	ret = 0;
	
set_exit:	
	freeReplyObject(reply);
	redisFree(ctx);

	return ret;
}

int get_value_fromredis(const char *key, char *value) {
	char command[REDIS_COMMAND_LENGTH] = {0};
	int ret = 0, i = 0;

	sprintf(command, "get %s", key);
	redisContext *ctx = redisConnect(REDIS_HOST, REDIS_PORT);
	
	redisReply *reply = (redisReply*)redisCommand(ctx, command);
	if (reply->type != REDIS_REPLY_STRING) {
		apollo_printf("Failed to execute command[%s]\n", command);		
		ret =  -1;
		goto get_exit;
	}

	for (i = 0;i < strlen(reply->str) && i < REDIS_COMMAND_LENGTH-2;i ++) {
		*(value+i) = *(reply->str+i);
	}
	ret = i;
	
get_exit:	
	freeReplyObject(reply);
	redisFree(ctx);

	return ret;
}


char del_redis_key(const char *key) {
	char command[REDIS_COMMAND_LENGTH] = {0};
	redisContext* ctx = redisConnect(REDIS_HOST, REDIS_PORT);

	sprintf(command, "del %s", key);
	apollo_printf("cmd:%s\n", command);
	redisReply* reply = (redisReply*)redisCommand(ctx, command);

	freeReplyObject(reply);
	redisFree(ctx); 

	return 0;
}


