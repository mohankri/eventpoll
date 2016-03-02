#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <util.h>

#include "list.h"
#include "ipc.h"
#include "server.h"
#include "event.h"

static LIST_HEAD(event_fd_list);

static int
socket_accept(int fd)
{
        struct sockaddr addr;
        socklen_t len;
        int newfd;

        len = sizeof(addr);
        newfd = accept(fd, (struct sockaddr *) &addr, &len);
        if (fd < 0)
                printf("can't accept a new connection, %m\n");
	close(fd);
	printf("close %d newfd %d\n", fd, newfd);
        return newfd;
}

static int
socket_nodelay(int fd)
{
        int ret, opt;

        opt = 1;
        ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        return ret;
}

static int
socket_keepalive(int fd)
{
        int ret, opt;

        opt = 1;
        ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
        if (ret)
                return ret;

        opt = 1800;
        ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
        if (ret)
                return ret;

        opt = 6;
        ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt));
        if (ret)
                return ret;

        opt = 300;
        ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt));
        if (ret)
                return ret;

        return 0;
}


static int
socket_perm(int fd)
{
        struct ucred cred;
        socklen_t len = sizeof(cred);
        int     err;

        err = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, (void *)&cred, &len);
        if (err) {
                printf("can't get sockopt %m\n");
                return (-1);
        }
        if (cred.uid != getuid() || cred.gid != getgid())
                return -EPERM;
        return 0;
}

static int
socket_non_blocking(int fd)
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


struct event_data {
	callback_t	handler;
	void		*data;
	int		fd;
	struct	list_head	e_list;
};

struct event_data *
event_lookup(int fd)
{
	struct event_data *data;
	list_for_each_entry(data, &event_fd_list, e_list) {
		if (data->fd == fd) {
			return (data);
		}
	}
	return NULL;
}

int
remove_event_handler(int fd)
{
	struct event_data *data = event_lookup(fd);
	epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, NULL);
	list_del(&data->e_list);
	free(data);
	return (0);
}

int
modify_event_handler(int fd, int events)
{
	struct epoll_event ev;
	struct event_data *data;
	data = event_lookup(fd);
	memset(&ev, 0, sizeof(ev));
	ev.events |= events;
	ev.data.ptr = data;
	epoll_ctl(ep_fd, EPOLL_CTL_MOD, fd, &ev);
	return (0);
}

int
add_event_handler(int fd, int events, callback_t handler, void *arg)
{
	struct	epoll_event ev;
	struct	event_data  *data;
	int	err;

	data = malloc(sizeof(struct event_data));
	if (!data) {
		return -ENOMEM;
	}

	data->data = arg;
	data->handler = handler;
	data->fd = fd; 
	
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = data;
	//printf("add event handler ep_fd %d fd %d\n", ep_fd, fd);	
	err = epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev);
	if (err) {
		printf("Cannot add fd, %m\n");
		free(data);
	} else {
		list_add(&data->e_list, &event_fd_list);
	}
	return err;
}

void
unix_recv_handler(int fd, int events, void *data)
{
	char buf[128];
	int ret;
	ret = read(fd, buf, sizeof(buf));
	printf("recv handler...%d\n", ret);
	sleep(1);
	remove_event_handler(fd);
	close(fd);
	return;
}

void
unix_event_handler(int fd, int events, void *data)
{
	int err;
	int newfd = socket_accept(fd);
	if (newfd < 0) {
		printf("failed to accept socket..\n");
	}
	err = socket_perm(newfd);
	if (err) {
	}
	err = socket_non_blocking(newfd);
	printf("add event received ..\n");
        err = add_event_handler(newfd, EPOLLIN, unix_recv_handler, NULL);
	return;
}

static void
stream_tcp_event_handler(int fd, int events, void *data)
{
	char buf[128];
	if (events & EPOLLIN) {
		(void)read(fd, buf, sizeof(buf));
		modify_event_handler(fd, EPOLLOUT);
	}
	if (events & EPOLLOUT) {
		modify_event_handler(fd, EPOLLIN);
		(void)write(fd, buf, sizeof(buf));
	}
}

void
stream_event_handler(int fd, int events, void *data)
{
        struct sockaddr_storage from;
        socklen_t namesize;
        //struct iscsi_connection *conn;
        //struct iscsi_tcp_connection *tcp_conn;
        int newfd, ret;

        //dprintf("%d\n", fd);

        namesize = sizeof(from);
	//printf("stream event handler...\n");
        newfd = accept(fd, (struct sockaddr *) &from, &namesize);
        if (newfd < 0) {
                printf("can't accept, %m\n");
                return;
        }
	//printf("accepted ... event handler...\n");

        ret = socket_keepalive(newfd);
        if (ret)
                goto out;

        ret = socket_nodelay(newfd);
        if (ret)
                goto out;
        socket_non_blocking(newfd);

	//printf("stream add event received ..\n");
	add_event_handler(newfd, EPOLLIN, stream_tcp_event_handler, NULL);
	return;
out:
	close(newfd);
	return;
}

int
event_loop(void)
{
	int i;	
	int nevent, timeout;
	struct epoll_event events[1024];
	struct	event_data	*data;
	printf("ep_fd %d\n", ep_fd);	
retry:
	timeout = -1;
	nevent = epoll_wait(ep_fd, events, ARRAY_SIZE(events), timeout);
	if (nevent < 0) {
		if (errno != EINTR) {
			printf("Failed .... %m\n");
			exit(1);
		}
	} else if (nevent) {
		for (i = 0; i < nevent; i++) {
			data = (struct event_data *) events[i].data.ptr;
			if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
				close(data->fd);
				printf("fd is %d\n", data->fd);
				remove_event_handler(data->fd);
			} else {
				data->handler(data->fd, events[i].events, data->data);
			}
		}
	}
	goto retry;
}


__attribute__((constructor)) static void transport_init(void)
{
	printf("transport init...\n");
}
