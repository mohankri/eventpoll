#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <util.h>

#include "ipc.h"
#include "server.h"
#include "event.h"

static int control_port = 1;
static int ipc_fd;

static int
socket_connect(int *fd)
{
	int err;
        struct sockaddr_un addr;
        char sock_path[256], *path;

        *fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (*fd < 0) {
                printf("can't create a socket, %m\n");
                return errno;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_LOCAL;
        if ((path = getenv("IPC_SOCKET")) == NULL)
                path = IPC_SOCKET;
        snprintf(sock_path, sizeof(sock_path), "%s.%d",
                         path, control_port);
	printf("socket_path %s\n", sock_path);
        strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path));

        err = connect(*fd, (struct sockaddr *) &addr, sizeof(addr));
        if (err < 0)
                return errno;
        return 0;
}

static int lock_fd;
int ep_fd;

int
server_init(void)
{
        int fd = 0, err;
        struct sockaddr_un addr;
        struct stat st = {0};
        char sock_path[256], *path;

        if ((path = getenv("IPC_SOCKET")) == NULL) {
                path = IPC_SOCKET;
                if (stat(IPC_DIR, &st) == -1) {
                        mkdir(IPC_DIR, 0755);
                }
        }
        sprintf(sock_path, "%s.%d.lock", path, control_port);
        lock_fd = open(sock_path, O_WRONLY | O_CREAT,
                           S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (lock_fd < 0) {
                printf("failed to open lock file for management IPC\n");
                return -1;
        }

        if (lockf(lock_fd, F_TLOCK, 1) < 0) {
                if (errno == EACCES || errno == EAGAIN)
                        printf("another tgtd is using %s\n", sock_path);
                else
                        printf("unable to get lock of management IPC: %s"\
                                " (errno: %m)\n", sock_path);
                goto close_lock_fd;
        }
        fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (fd < 0) {
                printf("can't open a socket, %m\n");
                goto close_lock_fd;
        }
	snprintf(sock_path, sizeof(sock_path), "%s.%d", path, control_port);
        unlink(sock_path);
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path));

        err = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
        if (err) {
                printf("can't bind a socket, %m\n");
                goto close_ipc_fd;
        }

        err = listen(fd, 32);
        if (err) {
                printf("can't listen a socket, %m\n");
                goto close_ipc_fd;
        }
	ep_fd = epoll_create(4096);
	printf("add event handler unix ...%d\n", fd);
        err = add_event_handler(fd, EPOLLIN, unix_event_handler, NULL);
        if (err)
                goto close_ipc_fd;
        ipc_fd = fd;

        return 0;

close_ipc_fd:
	close(fd);
close_lock_fd:
        close(lock_fd);
        return -1;
}

int
server_connect(void)
{
	int fd, err;
	char req[128];
	socket_connect(&fd);
	err = write(fd, req, sizeof(req));
	if (err < 0) {
		printf("write failed..\n");
	}
	printf("wrote data..\n");
	close(fd);
	return (0);
}

int
server_cleanup(void)
{
	printf("server cleanup ...\n");
	close(ep_fd);
	return (0);
}
