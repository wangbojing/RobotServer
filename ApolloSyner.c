

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>



#define BUFFER_LENGTH  	255

int main(void) {
	struct sockaddr_in addr, ia;
	int sockfd;
	char recvmsg[BUFFER_LENGTH];
	unsigned int socklen, n;
	struct ip_mreq mreq;

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
		n = recvfrom(sockfd, recvmsg, BUFFER_LENGTH-1, 0, (struct sockaddr*)&addr, &socklen);
		if (n < 0) {
			printf("recvfrom error in udptalk!\n");
			exit(4);
		} else {
			recvmsg[n] = 0;
			printf("peer:%s\n", recvmsg);
		}
	}
}


