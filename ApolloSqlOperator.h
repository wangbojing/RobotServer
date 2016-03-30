


#ifndef __APOLLO_SQLOPERATOR_H__
#define __APOLLO_SQLOPERATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>
#include <pthread.h>


#define MYSQL_HOST		"localhost" //"192.168.1.150" //localhost
#define MYSQL_USER		"root"
#define MYSQL_PWD		"admin"
#define MYSQL_DB		"APOLLO_I"

#define SQL_LANGUAGE_LENGTH			512
#define CONNECTION_POOL_SIZE		50


#define SQL_INSERT_DEVICE_GPS		"INSERT TBL_DEVICE_LOCATION VALUE('%s', '%s', '%s', '%s', NOW())"
#define SQL_INSERT_USER				"INSERT TBL_USER(U_NAME, U_PWD) VALUES('%s', '%s')"

#define SQL_SELECT_USERINFO_LIST	"SELECT * FROM TBL_USER"
#define SQL_SELECT_RELATIONSHIP_LIST	"SELECT * FROM TBL_USER_RELATIONSHIP"
#define SQL_SELECT_USERID 			"SELECT U_ID FROM TBL_USER WHERE U_NAME='%s' AND U_PWD = '%s'"

#define SQL_SELECT_UPDATE_PASSWORD	"UPDATE TBL_USER SET U_PWD = '%s' WHERE U_NAME='%s'"

#define SQL_SELECT_DEVICEID			"SELECT R_DEVICE_ID  FROM TBL_USER_RELATIONSHIP WHERE R_USER_ID=%d"
#define SQL_SELECT_LOCATION_NEWINFO	"SELECT  * FROM TBL_DEVICE_LOCATION WHERE L_DEVICE_ID = '%s' ORDER BY L_TIME DESC"

typedef enum _ConnectionType {
	CONNECT_CLOSE = 0,
	CONNECT_ON = 1,
	CONNECT_OFF = 2,
} ConnectionType;

typedef enum _CONNECTIONPOOL_RESULT {
	OK = 0,
	NOEXIST = -1,
	NOINSERT = -2,
	NOUPDATE = -3,
} CONNECTIONPOOL_RESULT;

typedef struct _MySqlConnectionItem {
	MYSQL* Conn;
	ConnectionType Status;
} MySqlConnectionItem;


typedef struct _MySqlConnectionPool {
	MySqlConnectionItem ConnQueue[CONNECTION_POOL_SIZE];
	pthread_mutex_t PoolLock;
} MySqlConnectionPool;


static MySqlConnectionPool *ConnectionPool_Instance = NULL;
MySqlConnectionPool *getMySqlConnectionPool_Instance(void) ;

//struct location_package_data;
MYSQL* get_connection_from_pool(void);
char return_connection_to_pool(MYSQL *Conn);
int insertDeviceGps(const char* longitude, const char* latitude, const char* high, const char* device_id);
int selectLocationNewItem(char *u8DeviceId, int UserId);


#endif


