libdir ?= ../lib
INCLUDES += -I. -I../include

CFLAGS += -D_GNU_SOURCE
CFLAGS += $(INCLUDES)
ifneq ($(DEBUG),)
CFLAGS += -g -O0 -ggdb -rdynamic
else
CFLAGS += -g -O2 -fno-strict-aliasing
endif
CFLAGS += -Wall -Wstrict-prototypes -fPIC
#CFLAGS += -DTGT_VERSION=\"$(VERSION)$(EXTRAVERSION)\"
#CFLAGS += -DBSDIR=\"$(DESTDIR)$(libdir)/backing-store\"

LIBS += -lpthread -ldl -lipcsocket

ifneq ($(SD_NOTIFY),)
LIBS += -lsystemd
endif

PROGRAMS += daemon
DAEMON_OBJS += daemon.o

DAEMON_DEP = $(DAEMON_OBJS:.o=.d)

LDFLAGS = -Wl,-E,-rpath=$(libdir) -L../lib

.PHONY:all
all: $(PROGRAMS) $(MODULES)

daemon: $(DAEMON_OBJS)
	echo $(CC) $^ -o $@ $(LIBS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

-include $(TGTD_DEP)

TGTADM_OBJS = tgtadm.o concat_buf.o
TGTADM_DEP = $(TGTADM_OBJS:.o=.d)

tgtadm: $(TGTADM_OBJS)
	$(CC) $^ -o $@

-include $(TGTADM_DEP)

TGTIMG_OBJS = tgtimg.o libssc.o libcrc32c.o
TGTIMG_DEP = $(TGTIMG_OBJS:.o=.d)

tgtimg: $(TGTIMG_OBJS)
	$(CC) $^ -o $@

-include $(TGTIMG_DEP)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) -MF $*.d -MT $*.o $*.c

%.so: %.c
	$(CC) -shared $(CFLAGS) $*.c -o $*.so

bs_rbd.so: bs_rbd.c
	$(CC) -shared $(CFLAGS) bs_rbd.c -o bs_rbd.so -lrados -lrbd

bs_glfs.so: bs_glfs.c
	$(CC) -I/usr/include/glusterfs/api -shared $(CFLAGS) bs_glfs.c -o bs_glfs.so -lgfapi

.PHONY: install
install: $(PROGRAMS) $(MODULES)
	install -d -m 755 $(DESTDIR)$(sbindir)
	install -m 755 $(PROGRAMS) $(DESTDIR)$(sbindir)
ifneq ($(MODULES),)
	install -d -m 755 $(DESTDIR)$(libdir)/backing-store
	install -m 755 $(MODULES) $(DESTDIR)$(libdir)/backing-store
endif

.PHONY: clean
clean:
	rm -f *.[od] *.so $(PROGRAMS) 
