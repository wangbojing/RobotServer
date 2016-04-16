


#ifndef __APOLLO_SQLOPERATOR_H__
#define __APOLLO_SQLOPERATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>
#include <pthread.h>
#include <zdb.h>


#define MYSQL_HOST		"120.76.25.198" //"192.168.1.150" //localhost
#define MYSQL_USER		"root"
#define MYSQL_PWD		"admin"
#define MYSQL_DB		"APOLLO_I"

//#define MYSQL_DB_CONN_STRING		"mysql://120.76.25.198:3306/APOLLO_I?user=wangbojing&password=zhaomeiping"
#define MYSQL_DB_CONN_STRING		"mysql://112.93.116.188:3306/APOLLO_I?user=watch_server&password=123456"

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

#define TYPE_APOLLO_MYSQL_CONNECTIONPOOL	1
#define TYPE_ZDB_MYSQL_CONNECTIONPOOL		2
#define CONNECTIONPOOL_SELECT				TYPE_ZDB_MYSQL_CONNECTIONPOOL

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

#if (CONNECTIONPOOL_SELECT == TYPE_APOLLO_MYSQL_CONNECTIONPOOL)
static MySqlConnectionPool *ConnectionPool_Instance = NULL;
MySqlConnectionPool *getMySqlConnectionPool_Instance(void) ;

//struct location_package_data;
MYSQL* get_connection_from_pool(void);
char return_connection_to_pool(MYSQL *Conn);

#elif (CONNECTIONPOOL_SELECT==TYPE_ZDB_MYSQL_CONNECTIONPOOL)

static ConnectionPool_T ConnectionPool_Instance = 0;
//ConnectionPool_T getMySqlConnectionPool_Instance(void);
void ConnectionPoolInstanceInit(void);


#endif

int insertDeviceGps(const char* longitude, const char* latitude, const char* high, const char* device_id);
int selectLocationNewItem(char *u8DeviceId, int UserId);
void initUserInfoList(void);
void initUserAndDeviceRelationshipList(void) ;
int insertUser(const char* userName, const char* password);
int selectUserId(const char* userName, const char* password, unsigned int *userId);
int updatePassword(const char* userName, const char* password) ;



#endif


