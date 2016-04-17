


#ifndef __APOLLO_PROTOCOL_H__
#define __APOLLO_PROTOCOL_H__

#define FRAME_LENGTH		1024

/* ** **** ********  ****************  Header  ****************  ******** **** ** */
#define HDR_HEART_PACKET_DEVICE		0x48 //'H'
#define HDR_HEART_PACKET_PHONE		0x50 //'P'
#define HDR_COMMAND_PACKET			0x43 //'C'
#define HDR_LOCATION_DATA			0x47 //'G'
#define HDR_INFO_RETURN				0x52 //'R'
#define HDR_ADD_DEVICE				0x41 //'A'
#define HDR_OPERATOR_USER_INFO		0x55 //'U'
#define HDR_BASE_STATION_DEVICE		0x42 //'B'
#define HDR_BLOCK_DATA_RECV			0x56 //'V'
#define HDR_BLOCK_DATA_SEND			0x44 //'D'

/* ** **** ********  ****************  Index  ****************  ******** **** ** */
#define IDX_PHONE_ID				1
#define IDX_PHONE_COMMAND			5
#define IDX_DEVICE_ID				1
#define IDX_DEVICE_POWER			9
#define IDX_DEVICE_HEARTPACKET_TIMESTAMP	24
#define IDX_DEVICE_LOCATIONTYPE		9
#define IDX_DEVICE_GPS_LONGITUDE	10
#define IDX_DEVICE_GPS_LATITUDE		15
#define IDX_DEVICE_GPS_TIMESTAMP	20
#define IDX_DEVICE_GPS_HIGH			24
#define IDX_DEVICE_RETURN_COMMAND	3
#define IDX_DEVICE_RETURN_INDEX		4
#define IDX_DEVICE_RETURN_PHNUM		5


#define IDX_PHONE_CMD_INDEX			6
#define IDX_PHONE_CMD_NUMBER		7

#define IDX_PHONE_USERNAME			6
#define IDX_PHONE_PASSWORD			23
#define IDX_PHONE_RETURN_USERID		6


/* ** **** ********  ****************  Length  ****************  ******** **** ** */
#define LEN_DEVICE_ID				8
#define LEN_PHONE_ID				4
#define LEN_PHONE_NUMBER			8

#define LEN_HEART_PACKET_DEVICE		32
#define LEN_HEART_PACKET_PHONE		8
#define LEN_GPS_LOCATION_DATA		32
#define LEN_LAB_LOCATION_DATA		48
#define LEN_COMMAND_PACKET			16
#define LEN_DEVICE_DOWNLOAD			16

#define LEN_DEVICE_INFO				12

#define LEN_LONGITUDE_DATA			5
#define LEN_LATITUDE_DATA			5

#define LEN_PHONE_USERNAME			16
#define LEN_PHONE_PASSWORD			8

#define LEN_DEVICE_RETURN_BUFFER	6
#define LEN_DEVICE_HEARTPACKET_RETURN_BUFFER	20
#define LEN_BLOCK_BUFFER			32*1024
#define LEN_BLOCK_PACKET			1024
#define LEN_HEADER_LENGTH			32
#define LEN_BLOCK_DATA			(LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)

/* ** **** ********  ****************  Command  ****************  ******** **** ** */
#define CMD_PHONE_DEVICE_RESET			0x1A

#define CMD_PHONE_REGISTER_USER			0x71
#define CMD_PHONE_SIGNIN_USER			0x72
#define CMD_PHONE_UPDATE_PWD			0x73

#define CMD_PHONE_BIND_DEVICE			0x31
#define CMD_PHONE_UNBIND_DEVICE			0x32


#define CMD_PHONE_DEVICE_REALTIME_LOCATION	0x01
#define CMD_PHONE_DEVICE_DOWNLOAD			0x02
#define CMD_PHONE_DEVICE_COUNTSTEP_OPEN		0x03
#define CMD_PHONE_DEVICE_COUNTSTEP_CLOSE	0x04
#define CMD_PHONE_DEVICE_SLEEPQUILTY_OPEN	0x05
#define CMD_PHONE_DEVICE_SLEEPQUILTY_CLOSE	0x06
#define CMD_PHONE_DEVICE_ALARM				0x08
#define CMD_PHONE_DEVICE_SAFEZONE_ALARM		0x09
#define CMD_PHONE_DEVICE_LONGSAT_ALARM		0x0A
#define CMD_PHONE_DEVICE_LOWPOWER_ALARM		0x0B

#define CMD_PHONE_DEVICE_FAMILYNUMBER_SET	0x41
#define CMD_PHONE_DEVICE_CONTRACT_SET		0x42
#define CMD_PHONE_DEVICE_PHONEBOOK_SET		0x43
#define CMD_PHONE_DEVICE_FREQ_SET			0x44

#define CMD_PHONE_DEVICE_SOS_MSGREMAINER_OPEN		0x51
#define CMD_PHONE_DEVICE_SOS_MSGREMAINER_CLOSE		0x52
#define CMD_PHONE_DEVICE_POWER_MSGREMAINDER_OPEN	0x53
#define CMD_PHONE_DEVICE_POWER_MSGREMAINDER_CLOSE	0x54

#define CMD_PHONE_DEVICE_ALARMMUSIC_OPEN			0x61
#define CMD_PHONE_DEVICE_ALARMMUSIC_CLOSE			0x62
#define CMD_PHONE_DEVICE_SAFEZONEMUSIC_OPEN			0x63
#define CMD_PHONE_DEVICE_SAFEZONEMUSIC_CLOSE		0x64

#define CMD_PHONE_DEVICE_SOS_CLEAR					0x21
#define CMD_PHONE_DEVICE_AGPS_DOWNLOAD				0x2B

#define CMD_PHONE_BUZZER_DEVICE_OPEN	0x81
#define CMD_PHONE_BUZZER_DEVICE_CLOSE	0x82
#define CMD_PHONE_LIGHT1_DEVICE_OPEN	0x83
#define CMD_PHONE_LIGHT1_DEVICE_CLOSE	0x84
#define CMD_PHONE_LIGHT2_DEVICE_OPEN	0x85
#define CMD_PHONE_LIGHT2_DEVICE_CLOSE	0x86

/* ** **** ********  ****************  Return CMD Define  ****************  ******** **** ** */
#define RET_PHONE_SUCCESS				0x00
#define RET_PHONE_GETUSERID_FAILED		0x02
#define RET_PHONE_SIGNIN_FAILED			0x11
#define RET_PHONE_USERID_NOEXIST		0x03
#define RET_PHONE_UPDATE_PWD_FAILED		0x04


/* ** **** ********  ****************  Structure Define  ****************  ******** **** ** */

typedef struct _BlockBuffer {
	char buffer[LEN_BLOCK_BUFFER];
	char packet[LEN_BLOCK_PACKET];
	unsigned int index;
} BlockBuffer;

typedef struct client {
	int fd;
	struct event_base *evbase;
	struct bufferevent *buf_ev;
	struct evbuffer *output_buffer;

	BlockBuffer *pBlock;
} client_t;


struct ApolloProtocolProcess {
int (*deviceHeartPacket_Proc)(char *packet, int length, char* info);
int (*phoneHeaderPacket_Proc)(char *packet, int length, char* info);
int (*phoneCommandPacket_Proc)(char *packet, int length);
int (*deviceLocationPacket_Proc)(char *packet, int length);
int (*deviceBaseStation_Proc)(char *packet, int length);
int (*phoneAddDevice_Proc)(char *packet, int length);
int (*phoneOperatorUserInfo_Proc)(struct bufferevent *bev, client_t *client, char *packet, int length);
int (*deviceBlockDataRecvPacket_Proc)(client_t *client, char *packet, int length);
int (*deviceBlockDataSendPacket_Proc)(struct bufferevent *bev, client_t *client, char *packet, int length);
};


typedef struct LBS {
	unsigned short mcc;
	unsigned short mnc;
	unsigned short lac;
	unsigned short cid;
	signed char signal;
} Lbs;

typedef struct LOCINFO {
	char u8DeviceId[LEN_DEVICE_ID*2+2];
	char u8Longitude[2*LEN_LONGITUDE_DATA+2];
	char u8Latitude[2*LEN_LATITUDE_DATA+2];
	char u8TimeStamp[6];
	char u8UbloxFlag;
} LocationInfo;

enum {
	MULTICAST_TYPE_BLOCK = 0x31,
	MULTICAST_TYPE_AGPS = 0x32,
} MULTICAST_DATA_TYPE;

typedef struct MULTICAST {
	char u8DeviceId[LEN_DEVICE_ID*2+2];
	char u8flag;
} MulticastSynerPacket;

static struct ApolloProtocolProcess *ApolloProtocolInstance = NULL;



/* ** **** ********  ****************  Function Define  ****************  ******** **** ** */
void apolloParsePacket(struct bufferevent *bev, client_t *client);
void initApolloProtocol(void);
int apolloReturnPacket(struct bufferevent *bev, client_t *client, char *buffer, int length);


extern void closeClient(client_t *client);
extern void closeAndFreeClient(client_t *client);



#endif

