
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

PROGRAMS += client stream_client
CLIENT_OBJS += client.o 
STREAM_OBJS += stream_client.o

DAEMON_DEP = $(CLIENT_OBJS:.o=.d)

LDFLAGS = -Wl,-E,-rpath=$(libdir) -L../lib

.PHONY:all
all: $(PROGRAMS) $(MODULES)

client: $(CLIENT_OBJS)
	echo $(CC) $^ -o $@ $(LIBS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

stream_client: $(STREAM_OBJS)
	echo $(CC) $^ -o $@ $(LIBS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) -MF $*.d -MT $*.o $*.c

%.so: %.c
	$(CC) -shared $(CFLAGS) $*.c -o $*.so

clean:
	rm -f *.[od] *.so $(PROGRAMS) iscsi/*.[od] ibmvio/*.[od] fc/*.[od]
