ifdef NO_USBMUXD_STUB
	PROGRAM := AltServer
else
	ifdef NO_UPNP_STUB
		PROGRAM := AltServerNet
	else
		PROGRAM := AltServerUPnP
	endif
endif


%.c.o : %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

%.cpp.o : %.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

CFLAGS := -DHAVE_CONFIG_H -DDEBUG -O0 -g -mno-default
CXXFLAGS = $(CFLAGS) -std=c++17

main_src := $(wildcard src/*.c) $(wildcard src/*.cpp)

libimobiledevice_src := $(wildcard libraries/libimobiledevice/src/*.c) $(wildcard libraries/libimobiledevice/common/*.c)
ifdef NO_USBMUXD_STUB
	CFLAGS += -DNO_USBMUXD_STUB
	libimobiledevice_src += $(wildcard libraries/libusbmuxd/src/*.c)
	libimobiledevice_src += $(wildcard libraries/libusbmuxd/common/*.c)
else
	main_src += src/phone/libusbmuxd-stub.c
	ifdef NO_UPNP_STUB
		CFLAGS += -DNO_UPNP_STUB
	else
		libimobiledevice_src := $(filter-out libraries/libimobiledevice/src/idevice.c,  $(libimobiledevice_src)) 
		main_src += src/phone/idevice-stub.c
	endif
endif

libimobiledevice_include := -Ilibraries/libimobiledevice/include -Ilibraries/libimobiledevice -Ilibraries/libusbmuxd/include

libplist_src := $(wildcard libraries/libplist/src/*.c) libraries/libplist/libcnary/node.c libraries/libplist/libcnary/node_list.c
libplist_include := -Ilibraries/libplist/include

miniupnpc_src = minissdpc.c miniwget.c minixml.c igd_desc_parse.c minisoap.c \
		  miniupnpc.c upnpreplyparse.c upnpcommands.c upnperrors.c \
		  connecthostport.c portlistingparse.c receivedata.c upnpdev.c \
		  addr_is_reserved.c
miniupnpc_src := $(miniupnpc_src:%.c=libraries/miniupnpc/%.c)
miniupnpc_include := -Ilibraries/miniupnpc

INC_CFLAGS := -Ilibraries
INC_CFLAGS += $(libimobiledevice_include)
INC_CFLAGS += $(libplist_include)
INC_CFLAGS += $(miniupnpc_include)
INC_CFLAGS += -Ilibraries/AltSign

allsrc := $(main_src) 
allsrc += $(libimobiledevice_src) 
allsrc += $(libplist_src)
allsrc += $(miniupnpc_src)

allobj = $(addsuffix .o, $(allsrc))

$(addsuffix .o, $(libimobiledevice_src)) : EXTRA_FLAGS := -Ilibraries $(libimobiledevice_include) $(libplist_include) -Ilibraries/libimobiledevice/common -Ilibraries/libusbmuxd/common
libraries/libimobiledevice.a : $(addsuffix .o, $(libimobiledevice_src))
	ar rcs $@ $^

$(addsuffix .o, $(libplist_src)) : EXTRA_FLAGS := -Ilibraries $(libplist_include) -Ilibraries/libplist/libcnary/include -Ilibraries/libplist/src
libraries/libplist.a : $(addsuffix .o, $(libplist_src))
	ar rcs $@ $^

# $(miniupnpc_src:.c=.o) : $(miniupnpc_src)
# 	$(CC) $(CFLAGS) -Ilibraries -c $<
$(addsuffix .o, $(miniupnpc_src)) : EXTRA_FLAGS := -Ilibraries $(miniupnpc_include)
libraries/miniupnp.a : $(addsuffix .o, $(miniupnpc_src))
	ar rcs $@ $^

$(addsuffix .o, $(main_src)) : EXTRA_FLAGS := -Ilibraries $(INC_CFLAGS)

#%.o : %.c
#	$(CC) $(CFLAGS) $(INC_CFLAGS) -c $< -o $@

lib_AltSign:
	$(MAKE) -C libraries/AltSign

LDFLAGS = libraries/AltSign/AltSign.a -static -lssl -lcrypto -lpthread -lcorecrypto_static -lzip -lm -lz -lcpprest -lboost_system -lboost_filesystem -lstdc++ -lssl -lcrypto -luuid
$(PROGRAM):: lib_AltSign

$(PROGRAM):: $(addsuffix .o, $(main_src)) libraries/miniupnp.a libraries/libimobiledevice.a libraries/libplist.a 
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean all lib_AltSign
clean:
	rm -f $(allobj) libraries/*.a $(PROGRAM)

all: $(PROGRAM)
.DEFAULT_GOAL := all