INCLUDES += -I. -I../include

CFLAGS += -D_GNU_SOURCE
CFLAGS += $(INCLUDES)
ifneq ($(DEBUG),)
CFLAGS += -g -O0 -ggdb -rdynamic
else
CFLAGS += -g -O2 -fno-strict-aliasing
endif
CFLAGS += -Wall -Wstrict-prototypes -fPIC -shared
#CFLAGS += -DTGT_VERSION=\"$(VERSION)$(EXTRAVERSION)\"
#CFLAGS += -DBSDIR=\"$(DESTDIR)$(libdir)/backing-store\"

LIBS += -lpthread -ldl

ifneq ($(SD_NOTIFY),)
LIBS += -lsystemd
endif

PROGRAMS += libipcsocket.so
IPC_SOCKET_OBJS += unix_socket.o stream_socket.o event.o

IPC_SOCKET_DEP = $(IPC_SOCKET_OBJS:.o=.d)

LDFLAGS = -Wl,-E,-rpath=$(libdir)

.PHONY:all
all: $(PROGRAMS) 


-include $(IPC_SOCKET_DEP)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) -MF $*.d -MT $*.o $*.c

%.so: %.c
	$(CC) -shared $(CFLAGS) $*.c -o $*.so

bs_rbd.so: bs_rbd.c
	$(CC) -shared $(CFLAGS) bs_rbd.c -o bs_rbd.so -lrados -lrbd

bs_glfs.so: bs_glfs.c
	$(CC) -I/usr/include/glusterfs/api -shared $(CFLAGS) bs_glfs.c -o bs_glfs.so -lgfapi

libipcsocket.so: unix_socket.c event.c stream_socket.c
	$(CC) -shared $(CFLAGS) unix_socket.c stream_socket.c event.c -o libipcsocket.so 

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
