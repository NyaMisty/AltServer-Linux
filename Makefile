libimobiledevice_src := $(wildcard libraries/libimobiledevice/src/*.c) $(wildcard libraries/libimobiledevice/common/*.c) 
libimobiledevice_src := $(filter-out libraries/libimobiledevice/src/idevice.c,  $(libimobiledevice_src)) 

miniupnpc_src = minissdpc.c miniwget.c minixml.c igd_desc_parse.c minisoap.c \
          miniupnpc.c upnpreplyparse.c upnpcommands.c upnperrors.c \
          connecthostport.c portlistingparse.c receivedata.c upnpdev.c \
          addr_is_reserved.c
miniupnpc_src := $(miniupnpc_src:%.c=libraries/miniupnpc/%.c)

src := $(wildcard src/*.c) 
src += $(libimobiledevice_src)
src += $(wildcard libraries/libplist/src/*.c) libraries/libplist/libcnary/node.c libraries/libplist/libcnary/node_list.c
src += $(miniupnpc_src)

obj = $(src:.c=.o)

CFLAGS := -DHAVE_CONFIG_H -DDEBUG -O0 -g
CFLAGS += -Ilibraries -Ilibraries/libimobiledevice/include -Ilibraries/libimobiledevice 
CFLAGS += -Ilibraries/libplist/src -Ilibraries/libplist/include -Ilibraries/libplist/libcnary/include -Ilibraries/libusbmuxd/include
CFLAGS += -Ilibraries/miniupnpc

LDFLAGS = -lm -lcrypto -lpthread -lssl -lzip

test: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) test