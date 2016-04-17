


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_LENGTH		255

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


#define apollo_printf	printf

int read_dat(const char *filename, char *data, int len) {
	FILE *pAMRFile = fopen(filename, "rb+");

	int size = fread(data, 1, len, pAMRFile);

	apollo_printf("aaa file size: %d \n", size);
	fclose(pAMRFile);

	return size;
}


void genAgpsFilePathName(char *filename, char *pathname) {
	sprintf(pathname, "./agps_data/%s_agps.dat", filename);
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
		header[i+1] = packet[i];
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

int takeDataPacketFlag(unsigned char *data, int length, char flag) {
	int i = 0;
	while (i*LEN_BLOCK_PACKET < length) {
		data[i*LEN_BLOCK_PACKET + 20] = flag;
		i ++;
	}
}

int main() {
	struct sockaddr_in addr, myaddr;
	int sockfd;
	char recmsg[BUFFER_LENGTH];
	char sendBuf[LEN_BLOCK_BUFFER];
	char fileName[128] = {0};
	char pathName[128] = {0};
	char devId[LEN_DEVICE_ID] = {0x20, 0x16, 0x01, 0x05, 0x00, 0x00, 0x00, 0x14};
	unsigned int socklen;
	int totalLength = 0, sendLength = 0;
	int yes = 0;
	unsigned char ttl = 0xFF;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket creating error\n");
		exit(1);
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8882);
	inet_pton(AF_INET, "224.0.243.12", &addr.sin_addr);

	memset(&myaddr, 0, sizeof(struct sockaddr_in));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(8881);
	myaddr.sin_addr.s_addr = INADDR_ANY;

	setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_LOOP,&yes,sizeof(yes));
	setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(unsigned char));

	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr_in)) == -1) {
		printf("Bind error\n");
		exit(0);
	}

	strcpy(fileName, "2016010500000014");
	genAgpsFilePathName(fileName, pathName);
	printf("%s\n", pathName);
	totalLength = takeDataPacket(pathName, sendBuf, devId);
	printf("total:%d, sendBuf[0]:%x\n", totalLength, sendBuf[0]);
	takeDataPacketFlag(sendBuf, totalLength, 0x32);
	
	//strcpy (recmsg, "abcderfgadfsdf\n");
	while (sendLength < totalLength) {
		//bzero(recmsg, BUFFER_LENGTH);
		//if (fgets(recmsg, BUFFER_LENGTH, ))
		printf(" %x\n", *(sendBuf+sendLength));
		if ((totalLength - sendLength) >= LEN_BLOCK_PACKET) {
			//evbuffer_add(client->output_buffer, client->pBlock->buffer+sendLength, LEN_BLOCK_PACKET);
			sendto(sockfd, sendBuf+sendLength, LEN_BLOCK_PACKET, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
			sendLength += LEN_BLOCK_PACKET;
		} else {
			//evbuffer_add(client->output_buffer, client->pBlock->buffer+sendLength, (totalLength - sendLength));
			sendto(sockfd, sendBuf+sendLength, (totalLength - sendLength), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
			sendLength = totalLength;
		}
		#if 0
		if (sendto(sockfd, recmsg, strlen(recmsg), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
			printf("sendto error!\n");
			exit(3);
		}
		printf("'%s' send ok", recmsg);

		bzero(recmsg, BUFFER_LENGTH);
		if (fgets(recmsg, BUFFER_LENGTH, stdin) == (char*)EOF) {
			exit(0);
		}
		#endif
	}
	
}



