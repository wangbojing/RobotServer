

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "ApolloUtil.h"
#include "ApolloProtocol.h"



#define BUFFER_LENGTH  	1048
#define PACKET_LENGTH	1024
#define SYNER_BLOCK_DATA_LENGTH		32*1024

void* ApolloSynerAction(void *arg) {
	struct sockaddr_in addr, myaddr;
	int sockfd;
	char recmsg[BUFFER_LENGTH];
	char sendBuf[LEN_BLOCK_BUFFER];
	char fileName[128] = {0};
	char pathName[128] = {0};
	char devId[LEN_DEVICE_ID] = {0};//{0x20, 0x16, 0x01, 0x05, 0x00, 0x00, 0x00, 0x14};
	unsigned int socklen;
	int totalLength = 0, sendLength = 0;
	int yes = 0, i;
	unsigned char ttl = 0xFF;
	MulticastSynerPacket *pSynPacket = (MulticastSynerPacket*)arg;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		apollo_printf("socket creating error\n");
		goto exit_lab;
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
		apollo_printf("Bind error\n");
		goto exit_lab;
	}

	strcpy(fileName, pSynPacket->u8DeviceId);
	genAgpsFilePathName(fileName, pathName);
	printf("%s\n", pathName);
	
	for (i = 0;i < LEN_DEVICE_ID;i ++) {
		devId[i] = STRINGTOHEX(pSynPacket->u8DeviceId[2*i], pSynPacket->u8DeviceId[2*i+1]);
		//printf("%c%c %x \n",Param[2][2*i], Param[2][2*i+1], u8PhoneNumber[i]);
	}
	totalLength = takeDataPacket(pathName, sendBuf, devId);
	//printf("total:%d, sendBuf[0]:%x\n", totalLength, sendBuf[0]);
	takeDataPacketFlag(sendBuf, totalLength, pSynPacket->u8flag);
	
	while (sendLength < totalLength) {
		if ((totalLength - sendLength) >= LEN_BLOCK_PACKET) {
			//evbuffer_add(client->output_buffer, client->pBlock->buffer+sendLength, LEN_BLOCK_PACKET);
			sendto(sockfd, sendBuf+sendLength, LEN_BLOCK_PACKET, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
			sendLength += LEN_BLOCK_PACKET;
		} else {
			//evbuffer_add(client->output_buffer, client->pBlock->buffer+sendLength, (totalLength - sendLength));
			sendto(sockfd, sendBuf+sendLength, (totalLength - sendLength), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
			sendLength = totalLength;
		}
	}
	
exit_lab:
	close(sockfd);
	free(pSynPacket);
	
	return NULL;
}


void* ApolloSyner(void *arg) {
	struct sockaddr_in addr, ia;
	int sockfd;
	char recvmsg[BUFFER_LENGTH];
	char u8SynerBlockData[SYNER_BLOCK_DATA_LENGTH] = {0};
	char u8DevId[LEN_DEVICE_ID] = {0};
	unsigned int socklen, n;
	struct ip_mreq mreq;
	int yes = 0;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket creating err in udptalk\n");
		exit(1);
	}
	bzero(&mreq, sizeof(struct ip_mreq));
	inet_pton(AF_INET, "224.0.243.12", &ia.sin_addr);
	bcopy(&ia.sin_addr, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));

	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) == -1) {
		perror("setsockopt");
		exit(-1);
	}
	setsockopt(sockfd,IPPROTO_IP, IP_MULTICAST_LOOP,&yes,sizeof(yes));

	socklen = sizeof(struct sockaddr_in);
	memset(&addr, 0, socklen);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8882);
	inet_pton(AF_INET, "224.0.243.12", &addr.sin_addr);

	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
		printf("Bind error\n");
		exit(0);
	}

	while (1) {
		bzero(recvmsg, BUFFER_LENGTH);
		n = recvfrom(sockfd, recvmsg, PACKET_LENGTH, 0, (struct sockaddr*)&addr, &socklen);
		if (n < 0) {
			printf("recvfrom error in udptalk!\n");
			//exit(4);
			continue;
		} else {
			char *data = recvmsg;
			int dataLength = data[30] * 256 + (unsigned char)data[31];
			int pktIndex = data[13];
			int pktTotal = data[29];
			int i = 0;
			
			if (data[0] != 'V') {
				printf(" Bad Packet, PacketHeader is Error %x\n", data[0]);
				continue;
			}
			if (pktIndex == 0) {
				memcpy(u8DevId, data+1, LEN_DEVICE_ID);
			}
			if (!apollo_strcmp(u8DevId, data+1, LEN_DEVICE_ID)) {
				printf(" Bad Packet, DevId is Error\n");
				continue;
			}
			if ((n == PACKET_LENGTH) && ((pktIndex + 1) != pktTotal)) {
				for (i = 0;i < dataLength;i ++) {
					u8SynerBlockData[pktIndex * (LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)+i] = data[LEN_HEADER_LENGTH+i];
				}
			}

			
			if (((pktIndex + 1) == pktTotal)) { //last packet
				char u8DeviceId[LEN_DEVICE_ID*2+1] = {0};
				char u8FileName[LEN_DEVICE_ID*8] = {0};
				
				for (i = 0;i < dataLength;i ++) {
					u8SynerBlockData[pktIndex * (LEN_BLOCK_PACKET-LEN_HEADER_LENGTH)+i] = data[LEN_HEADER_LENGTH+i];
				}
				
				hextostring(u8DeviceId ,data+IDX_DEVICE_ID, LEN_DEVICE_ID);
				printf("Dev:%s\n", u8DeviceId);
				if (data[20] == MULTICAST_TYPE_BLOCK) {
					genBlockFilePathName(u8DeviceId, u8FileName);
				} else if (data[20] == MULTICAST_TYPE_AGPS) {
					genAgpsFilePathName(u8DeviceId, u8FileName);
				} else {
					genBlockFilePathName(u8DeviceId, u8FileName);
				}
				printf(" filename:%s\n", u8FileName);
				write_dat(u8FileName, u8SynerBlockData, pktIndex * (LEN_BLOCK_PACKET - LEN_HEADER_LENGTH) + dataLength);

				memset(u8SynerBlockData, 0, SYNER_BLOCK_DATA_LENGTH);
				printf("save success\n");
			}
		}
	}
}


void StartApolloSyner(void) {
	pthread_t tid = -1;
	int err = pthread_create(&tid, NULL, ApolloSyner, NULL);
	if(0 != err)
		fprintf(stderr, "Couldn't run StartApolloSyner, errno %d\n", err);
	else
		fprintf(stderr, "StartApolloSyner\n");
}

void StartApolloSynerAction(void *arg) {
	pthread_t tid = -1;
	int err = pthread_create(&tid, NULL, ApolloSynerAction, arg);
	if(0 != err)
		fprintf(stderr, "Couldn't run StartApolloSynerAction, errno %d\n", err);
	else
		fprintf(stderr, "StartApolloSynerAction\n");
}


