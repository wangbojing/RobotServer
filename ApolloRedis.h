

#ifndef __APOLLO_REDIS_H__
#define __APOLLO_REDIS_H__


/* ** **** ********  ****************  Redis Server Info  ****************  ******** **** ** */
#define REDIS_HOST		"112.93.116.188" //"120.76.25.198"
#define REDIS_PORT		6379
#define REDIS_COMMAND_LENGTH	256



/* ** **** ********  ****************  Function  ****************  ******** **** ** */
int get_value_fromredis(const char *key, char *value);
int set_key_toredis(const char *key, const char *value);
char del_redis_key(const char *key);



#endif

