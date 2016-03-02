#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "util.h"
#include "ipc.h"
#include "event.h"
#include "server.h"

unsigned long pagesize, pageshift;

static void
signal_catch(int signo)
{
	printf("signal catch ...%d\n", signo);
	//signal(signo, SIG_IGN);
	server_cleanup();	
}

int main(int argc, char **argv)
{
	struct sigaction sa_old;
	struct sigaction sa_new;
	int err;
#if 0
	int is_daemon = 1, is_debug = 0;
	int ret;
#endif
	sa_new.sa_handler = signal_catch;
	sigemptyset(&sa_new.sa_mask);
	sa_new.sa_flags = 0;
	sigaction(SIGPIPE, &sa_new, &sa_old);
	sigaction(SIGTERM, &sa_new, &sa_old);
	sigaction(SIGINT, &sa_new, &sa_old);

	pagesize = sysconf(_SC_PAGESIZE);
	for (pageshift = 0;; pageshift++)
		if (1UL << pageshift == pagesize)
			break;
#if 0
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'f':
			is_daemon = 0;
			break;
		case 'C':
			ret = str_to_int_ge(optarg, control_port, 0);
			if (ret)
				bad_optarg(ret, ch, optarg);
			break;
		case 't':
			ret = str_to_int_gt(optarg, nr_iothreads, 0);
			if (ret)
				bad_optarg(ret, ch, optarg);
			break;
		case 'd':
			ret = str_to_int_range(optarg, is_debug, 0, 1);
			if (ret)
				bad_optarg(ret, ch, optarg);
			break;
		case 'V':
			version();
			break;
		case 'h':
			usage(0);
			break;
		default:
			if (strncmp(argv[optind - 1], "--", 2))
				usage(1);

			ret = parse_params(argv[optind - 1] + 2, argv[optind]);
			if (ret)
				usage(1);

			break;
		}
	}
#endif
#if 0
	ep_fd = epoll_create(4096);
	if (ep_fd < 0) {
		fprintf(stderr, "can't create epoll fd, %m\n");
		exit(1);
	}
	spare_args = optind < argc ? argv[optind] : NULL;

	if (is_daemon && daemon(0, 0))
		exit(1);
#endif
	err = server_init();
	if (err)
		exit(1);

	stream_socket(NULL, 8888);
#if 0
	err = log_init(program_name, LOG_SPACE_SIZE, is_daemon, is_debug);
	if (err)
		exit(1);

	nr_lld = lld_init();
	if (!nr_lld) {
		fprintf(stderr, "No available low level driver!\n");
		exit(1);
	}

	err = oom_adjust();
	if (err && (errno != EACCES) && getuid() == 0)
		exit(1);

	err = nr_file_adjust();
	if (err)
		exit(1);

	err = work_timer_start();
	if (err)
		exit(1);

	bs_init();
#endif
#ifdef USE_SYSTEMD
	sd_notify(0, "READY=1\nSTATUS=Starting event loop...");
#endif

	event_loop();
#if 0
	lld_exit();

	work_timer_stop();

	ipc_exit();

	log_close();
#endif
	return 0;
}
