src := $(wildcard src/*.c) $(wildcard libraries/libimobiledevice/src/*.c) $(wildcard libraries/libimobiledevice/common/*.c) $(wildcard libraries/libplist/src/*.c) libraries/libplist/libcnary/node.c libraries/libplist/libcnary/node_list.c
src := $(filter-out libraries/libimobiledevice/src/lockdown.c, $(src))

obj = $(src:.c=.o)

CFLAGS = -DHAVE_CONFIG_H -DDEBUG -O0 -g -Ilibraries -Ilibraries/libimobiledevice/include -Ilibraries/libimobiledevice -Ilibraries/libplist/src -Ilibraries/libplist/include -Ilibraries/libplist/libcnary/include -Ilibraries/libusbmuxd/include

LDFLAGS = -lm -lcrypto -lpthread -lssl -lzip

test: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) test