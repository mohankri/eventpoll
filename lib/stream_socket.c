#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "event.h"

int
non_blocking(int fd)
{
        int err;
        err = fcntl(fd, F_GETFL);
        if (err < 0) {
                printf("unable to get fd flags %m\n");
        } else {
                err = fcntl(fd, F_SETFL, err | O_NONBLOCK);
                if (err == -1) {
                        printf("unable to set fd flags %m\n");
                } else {
                        err = 0;
                }
        }
        return err;
}


int
stream_socket(char *addr, int port)
{
        struct addrinfo hints, *res, *res0;
        char servname[64];
        int ret, fd, opt, nr_sock = 0;
	int  events;
        //struct iscsi_portal *portal = NULL;
        //char addrstr[64];
        //void *addrptr = NULL;

        port = port ? port : 8888;

        memset(servname, 0, sizeof(servname));
        snprintf(servname, sizeof(servname), "%d", port);

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        ret = getaddrinfo(addr, servname, &hints, &res0);
        if (ret) {
                printf("unable to get address info, %m\n");
                return -errno;
        }

        for (res = res0; res; res = res->ai_next) {
                fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                if (fd < 0) {
                        if (res->ai_family == AF_INET6)
                                printf("IPv6 support is disabled.\n");
                        else
                                printf("unable to create fdet %d %d %d, %m\n",
                                        res->ai_family, res->ai_socktype,
                                        res->ai_protocol);
                        continue;
                }

                opt = 1;
                ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                                 sizeof(opt));
                if (ret)
                        printf("unable to set SO_REUSEADDR, %m\n");

                opt = 1;
                if (res->ai_family == AF_INET6) {
                        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt,
                                         sizeof(opt));
                        if (ret) {
                                close(fd);
                                continue;
                        }
                }

                ret = bind(fd, res->ai_addr, res->ai_addrlen);
                if (ret) {
                        close(fd);
                        printf("unable to bind server socket, %m\n");
                        continue;
                }

                ret = listen(fd, SOMAXCONN);
                if (ret) {
                        printf("unable to listen to server socket, %m\n");
                        close(fd);
                        continue;
                }
               	non_blocking(fd);
		printf("add event handler...stream fd %d\n", fd);
		events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
		ret = add_event_handler(fd, events, stream_event_handler, NULL);
		if (ret) {
			printf("close stream fd...\n");
			close(fd);
		}
        }

        freeaddrinfo(res0);

        return !nr_sock;
}

