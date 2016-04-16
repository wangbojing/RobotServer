


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_LENGTH		255

int main() {
	struct sockaddr_in addr, myaddr;
	int sockfd;
	char recmsg[BUFFER_LENGTH];
	unsigned int socklen;

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

	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr_in)) == -1) {
		printf("Bind error\n");
		exit(0);
	}
	
	strcpy (recmsg, "abcderfgadfsdf\n");
	while (1) {
		//bzero(recmsg, BUFFER_LENGTH);
		//if (fgets(recmsg, BUFFER_LENGTH, ))
		if (sendto(sockfd, recmsg, strlen(recmsg), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
			printf("sendto error!\n");
			exit(3);
		}
		printf("'%s' send ok", recmsg);

		bzero(recmsg, BUFFER_LENGTH);
		if (fgets(recmsg, BUFFER_LENGTH, stdin) == (char*)EOF) {
			exit(0);
		}
	}
	
}



