#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

#include "server.h"

#define BUFSIZE 4096

static const char program_name[] = "stream_client";


int
main(int argc, char **argv)
{
	int sockfd;
	char buf[128];
	struct sockaddr_in remote_addr;
	struct	hostent *host;
	int ret;
	int i;

	host = gethostbyname("ceph12");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(8888);
	remote_addr.sin_addr = *((struct in_addr *)host->h_addr);

	connect(sockfd, (struct sockaddr *)&remote_addr,
					sizeof(struct sockaddr));

	for(i = 0; i < 10; i++) {
		ret = write(sockfd, buf, 128); 
		if (ret < 0) {
			printf("write %d\n", ret);
		}
		//sleep(1);
		ret = read(sockfd, buf, 128);
		if (ret < 0) {
			printf("read failed %d\n", ret);
		}
		//printf("read %d\n", ret);
	}
	close(sockfd);
	return (0);
}
