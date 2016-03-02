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

#include "server.h"

#define NO_LOGGING
//#include "log.h"

#define BUFSIZE 4096

static const char program_name[] = "client";
static int debug = 1;

/* default port to use for the mgmt channel */
//static short int control_port;

int main(int argc, char **argv)
{
#if 0
	int ch, longindex, rc;
	int op, tid, mode, dev_type, ac_dir;
	uint32_t cid, hostno;
	uint64_t sid, lun, force;
	char *name, *value, *path, *targetname, *address, *iqnname, *targetOps;
	char *portalOps, *bstype, *bsopts;
	char *bsoflags;
	char *blocksize;
	char *user, *password;
	struct tgtadm_req adm_req = {0}, *req = &adm_req;
	struct concat_buf b;
	char *op_name;

	op = tid = mode = -1;
	cid = hostno = sid = 0;
	lun = UINT64_MAX;

	rc = 0;
	dev_type = TYPE_DISK;
	ac_dir = ACCOUNT_TYPE_INCOMING;
	name = value = path = targetname = address = iqnname = NULL;
	targetOps = portalOps = bstype = bsopts = NULL;
	bsoflags = blocksize = user = password = op_name = NULL;
	force = 0;
	dprintf("tgtd %s\n", __FUNCTION__);
	optind = 1;
	while ((ch = getopt_long(argc, argv, short_options,
				 long_options, &longindex)) >= 0) {
		errno = 0;
		switch (ch) {
		case 'L':
			strncpy(req->lld, optarg, sizeof(req->lld));
			break;
		case 'o':
			op = str_to_op(optarg);
			op_name = optarg;
			break;
		case 'm':
			mode = str_to_mode(optarg);
			break;
		case 't':
			rc = str_to_int_ge(optarg, tid, 0);
			if (rc)
				bad_optarg(rc, ch, optarg);
			break;
		case 's':
			rc = str_to_int(optarg, sid);
			if (rc)
				bad_optarg(rc, ch, optarg);
			break;
		case 'c':
			rc = str_to_int(optarg, cid);
			if (rc)
				bad_optarg(rc, ch, optarg);
			break;
		case 'l':
			rc = str_to_int(optarg, lun);
			if (rc)
				bad_optarg(rc, ch, optarg);
			break;
		case 'P':
			if (mode == MODE_PORTAL)
				portalOps = optarg;
			else
				targetOps = optarg;
			break;
		case 'n':
			name = optarg;
			break;
		case 'v':
			value = optarg;
			break;
		case 'b':
			path = optarg;
			break;
		case 'T':
			targetname = optarg;
			break;
		case 'I':
			address = optarg;
			break;
		case 'Q':
			iqnname = optarg;
			break;
		case 'u':
			user = optarg;
			break;
		case 'p':
			password = optarg;
			break;
		case 'B':
			hostno = bus_to_host(optarg);
			break;
		case 'H':
			rc = str_to_int_ge(optarg, hostno, 0);
			if (rc)
				bad_optarg(rc, ch, optarg);
			break;
		case 'F':
			force = 1;
			break;
		case 'f':
			bsoflags = optarg;
			break;
		case 'y':
			blocksize = optarg;
			break;
		case 'E':
			bstype = optarg;
			break;
		case 'S':
			bsopts = optarg;
			break;
		case 'Y':
			dev_type = str_to_device_type(optarg);
			break;
		case 'O':
			ac_dir = ACCOUNT_TYPE_OUTGOING;
			break;
		case 'C':
			rc = str_to_int_ge(optarg, control_port, 0);
			if (rc)
				bad_optarg(rc, ch, optarg);
			break;
		case 'V':
			version();
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
		}
	}

	if (optind < argc) {
		eprintf("unrecognized option '%s'\n", argv[optind]);
		usage(1);
	}

	if (op < 0) {
		eprintf("specify the operation type\n");
		exit(EINVAL);
	}

	if (mode < 0) {
		eprintf("specify the mode\n");
		exit(EINVAL);
	}

	if (mode == MODE_SYSTEM) {
		switch (op) {
		case OP_UPDATE:
			rc = verify_mode_params(argc, argv, "LmonvC");
			if (rc) {
				eprintf("system mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			if ((!name || !value)) {
				eprintf("update operation requires 'name'"
					" and 'value' options\n");
				exit(EINVAL);
			}
			break;
		case OP_SHOW:
		case OP_DELETE:
		case OP_STATS:
			break;
		default:
			eprintf("operation %s not supported in system mode\n",
				op_name);
			exit(EINVAL);
			break;
		}
	}

	if (mode == MODE_TARGET) {
		if ((tid <= 0 && (op != OP_SHOW))) {
			if (tid == 0)
				eprintf("'tid' cannot be 0\n");
			else
				eprintf("'tid' option is necessary\n");

			exit(EINVAL);
		}
		switch (op) {
		case OP_NEW:
			rc = verify_mode_params(argc, argv, "LmotTC");
			if (rc) {
				eprintf("target mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!targetname) {
				eprintf("creating new target requires "
					"a name, use --targetname\n");
				exit(EINVAL);
			}
			break;
		case OP_DELETE:
			rc = verify_mode_params(argc, argv, "LmotCF");
			if (rc) {
				eprintf("target mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			break;
		case OP_SHOW:
		case OP_STATS:
			rc = verify_mode_params(argc, argv, "LmotC");
			if (rc) {
				eprintf("target mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			break;
		case OP_BIND:
		case OP_UNBIND:
			rc = verify_mode_params(argc, argv, "LmotIQBHC");
			if (rc) {
				eprintf("target mode: option '-%c' is not "
					  "allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!address && !iqnname && !hostno) {
				eprintf("%s operation requires"
					" initiator-address, initiator-name"
					"or bus\n", op_name);
				exit(EINVAL);
			}
			break;
		case OP_UPDATE:
			rc = verify_mode_params(argc, argv, "LmotnvC");
			if (rc) {
				eprintf("target mode: option '-%c' is not " \
					  "allowed/supported\n", rc);
				exit(EINVAL);
			}
			if ((!name || !value)) {
				eprintf("update operation requires 'name'" \
						" and 'value' options\n");
				exit(EINVAL);
			}
			break;
		default:
			eprintf("operation %s not supported in target mode\n",
				op_name);
			exit(EINVAL);
			break;
		}
	}

	if (mode == MODE_ACCOUNT) {
		switch (op) {
		case OP_NEW:
			rc = verify_mode_params(argc, argv, "LmoupfC");
			if (rc) {
				eprintf("account mode: option '-%c' is "
					"not allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!user || !password) {
				eprintf("'user' and 'password' options "
					"are required\n");
				exit(EINVAL);
			}
			break;
		case OP_SHOW:
			rc = verify_mode_params(argc, argv, "LmoC");
			if (rc) {
				eprintf("account mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			break;
		case OP_DELETE:
			rc = verify_mode_params(argc, argv, "LmouC");
			if (rc) {
				eprintf("account mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			break;
		case OP_BIND:
			rc = verify_mode_params(argc, argv, "LmotuOC");
			if (rc) {
				eprintf("account mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!user) {
				eprintf("'user' option is necessary\n");
				exit(EINVAL);
			}
			if (tid == -1)
				tid = GLOBAL_TID;
			break;
		case OP_UNBIND:
			rc = verify_mode_params(argc, argv, "LmotuOC");
			if (rc) {
				eprintf("account mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!user) {
				eprintf("'user' option is necessary\n");
				exit(EINVAL);
			}
			if (tid == -1)
				tid = GLOBAL_TID;
			break;
		default:
			eprintf("operation %s not supported in account mode\n",
				op_name);
			exit(EINVAL);
			break;
		}
	}

	if (mode == MODE_DEVICE) {
		if (tid <= 0) {
			if (tid == 0)
				eprintf("'tid' must not be 0\n");
			else
				eprintf("'tid' option is necessary\n");
			exit(EINVAL);
		}
		if (lun == UINT64_MAX) {
			eprintf("'lun' option is necessary\n");
			exit(EINVAL);
		}
		switch (op) {
		case OP_NEW:
			rc = verify_mode_params(argc, argv, "LmofytlbEYCS");
			if (rc) {
				eprintf("logicalunit mode: option '-%c' is not "
					  "allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!path && dev_type != TYPE_MMC
			    && dev_type != TYPE_TAPE
			    && dev_type != TYPE_DISK) {
				eprintf("'backing-store' option "
						"is necessary\n");
				exit(EINVAL);
			}
			break;
		case OP_DELETE:
		case OP_STATS:
			rc = verify_mode_params(argc, argv, "LmotlC");
			if (rc) {
				eprintf("logicalunit mode: option '-%c' is not "
					  "allowed/supported\n", rc);
				exit(EINVAL);
			}
			break;
		case OP_UPDATE:
			rc = verify_mode_params(argc, argv, "LmofytlPC");
			if (rc) {
				eprintf("option '-%c' not supported in "
					"logicalunit mode\n", rc);
				exit(EINVAL);
			}
			break;
		default:
			eprintf("operation %s not supported in "
				"logicalunit mode\n", op_name);
			exit(EINVAL);
			break;
		}
	}

	if (mode == MODE_PORTAL) {
		switch (op) {
		case OP_NEW:
			rc = verify_mode_params(argc, argv, "LmoCP");
			if (rc) {
				eprintf("portal mode: option '-%c' is not "
					  "allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!portalOps) {
				eprintf("you must specify --param "
					  "portal=<portal>\n");
				exit(EINVAL);
			}
			break;
		case OP_DELETE:
			rc = verify_mode_params(argc, argv, "LmoCP");
			if (rc) {
				eprintf("portal mode: option '-%c' is not "
					  "allowed/supported\n", rc);
				exit(EINVAL);
			}
			if (!portalOps) {
				eprintf("you must specify --param "
					  "portal=<portal>\n");
				exit(EINVAL);
			}
			break;
		case OP_SHOW:
			rc = verify_mode_params(argc, argv, "LmoC");
			if (rc) {
				eprintf("option '-%c' not supported in "
					"portal mode\n", rc);
				exit(EINVAL);
			}
			break;
		default:
			eprintf("operation %s not supported in portal mode\n",
				op_name);
			exit(EINVAL);
			break;
		}
	}

	if (mode == MODE_LLD) {
		switch (op) {
		case OP_START:
		case OP_STOP:
		case OP_SHOW:
			rc = verify_mode_params(argc, argv, "LmoC");
			if (rc) {
				eprintf("lld mode: option '-%c' is not "
					"allowed/supported\n", rc);
				exit(EINVAL);
			}
			break;
		default:
			eprintf("option %d not supported in lld mode\n", op);
			exit(EINVAL);
			break;
		}
	}

	req->op = op;
	req->tid = tid;
	req->sid = sid;
	req->cid = cid;
	req->lun = lun;
	req->mode = mode;
	req->host_no = hostno;
	req->device_type = dev_type;
	req->ac_dir = ac_dir;
	req->force = force;

	concat_buf_init(&b);

	if (name)
		concat_printf(&b, "%s=%s", name, value);
	if (path)
		concat_printf(&b, "%spath=%s", concat_delim(&b, ","), path);

	if (req->device_type == TYPE_TAPE)
		concat_printf(&b, "%sbstype=%s", concat_delim(&b, ","),
			      "ssc");
	else if (bstype)
		concat_printf(&b, "%sbstype=%s", concat_delim(&b, ","),
			      bstype);
	if (bsopts)
		concat_printf(&b, "%sbsopts=%s", concat_delim(&b, ","),
			      bsopts);
	if (bsoflags)
		concat_printf(&b, "%sbsoflags=%s", concat_delim(&b, ","),
			      bsoflags);
	if (blocksize)
		concat_printf(&b, "%sblocksize=%s", concat_delim(&b, ","),
			      blocksize);
	if (targetname)
		concat_printf(&b, "%stargetname=%s", concat_delim(&b, ","),
			      targetname);
	if (address)
		concat_printf(&b, "%sinitiator-address=%s",
			      concat_delim(&b, ","), address);
	if (iqnname)
		concat_printf(&b, "%sinitiator-name=%s", concat_delim(&b, ","),
			      iqnname);
	if (user)
		concat_printf(&b, "%suser=%s", concat_delim(&b, ","),
			      user);
	if (password)
		concat_printf(&b, "%spassword=%s", concat_delim(&b, ","),
			      password);
	/* Trailing ',' makes parsing params in modules easier.. */
	if (targetOps)
		concat_printf(&b, "%stargetOps %s,", concat_delim(&b, ","),
			      targetOps);
	if (portalOps)
		concat_printf(&b, "%sportalOps %s,", concat_delim(&b, ","),
			      portalOps);

	if (b.err) {
		eprintf("BUFSIZE (%d bytes) isn't long enough\n", BUFSIZE);
		return EINVAL;
	}

	rc = concat_buf_finish(&b);
	if (rc) {
		eprintf("failed to create request, errno:%d\n", rc);
		exit(rc);
	}

	return ipc_mgmt_req(req, &b);
#endif
	server_connect();	
	return (0);
}
