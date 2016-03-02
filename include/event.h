
typedef void (*callback_t)(int fd, int events, void *data);

extern int ep_fd;

int
add_event_handler(int fd, int events, callback_t, void *);

int
remove_event_handler(int fd);

void
unix_recv_handler(int fd, int events, void *);

void
unix_event_handler(int fd, int events, void *data);

void
stream_event_handler(int fd, int events, void *data);

int
event_loop(void);
