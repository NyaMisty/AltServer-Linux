CC := clang
CXX := clang++

ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(ROOT_DIR)/../main.mak

ALTSIGN_ROOT := $(UPSTREAM_DIR)/AltSign
ALTSIGN_NEWROOT := $(BUILD_DIR)/AltSign_patched
MINIZIP_ROOT := $(UPSTREAM_DIR)/AltSign/Dependencies/minizip
LDID_ROOT := $(UPSTREAM_DIR)/ldid
LDID_NEWROOT := $(BUILD_DIR)/ldid_patched

include $(MAIN_DIR)/makefiles/AltWindowsShim.mak

CFLAGS += -I$(ALTSIGN_ROOT) -I$(MINIZIP_ROOT) -I$(LDID_ROOT) -mno-sse
#CFLAGS += -DLDID_NOTOOLS # will lose some symbol if enable this

CXXFLAGS = $(CFLAGS) -std=c++17

altsign_orifiles := $(wildcard $(ALTSIGN_ROOT)/*.*)
altsign_newfiles := $(altsign_orifiles:$(ALTSIGN_ROOT)/%=$(ALTSIGN_NEWROOT)/%)

$(ALTSIGN_NEWROOT)/%: $(ALTSIGN_ROOT)/%
	mkdir -p `dirname "$@"`
	python3 $(ROOT_DIR)/rewrite_altsign_source.py "$<" > $@

$(altsign_newfiles) : $(altsign_orifiles)
altsign_src := $(filter %.cpp,$(altsign_newfiles))


ldid_orifiles := $(LDID_ROOT)/ldid.cpp $(LDID_ROOT)/lookup2.c
ldid_newfiles := $(ldid_orifiles:$(LDID_ROOT)/%=$(LDID_NEWROOT)/%)

$(LDID_NEWROOT)/%: $(LDID_ROOT)/%
	mkdir -p `dirname "$@"`
	python3 $(ROOT_DIR)/rewrite_ldid_source.py "$<" > $@

$(ldid_newfiles) : $(ldid_orifiles)
ldid_src := $(ldid_newfiles)


preprocess : $(altsign_newfiles) $(ldid_newfiles)
.PHONY : preprocess

minizip_src := $(MINIZIP_ROOT)/ioapi.c $(MINIZIP_ROOT)/zip.c $(MINIZIP_ROOT)/unzip.c

objs += $(altsign_src:$(BUILD_DIR)/%=$(BUILD_DIR)/objs/%.o)
objs += $(ldid_src:$(BUILD_DIR)/%=$(BUILD_DIR)/objs/%.o)
objs += $(minizip_src:$(MAIN_DIR)/%=$(BUILD_DIR)/objs/%.o)

#ldid/%.o : CC := gcc
#ldid/%.o : CXX := g++

$(BUILD_DIR)/objs/%.c.o : $(BUILD_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

$(BUILD_DIR)/objs/%.cpp.o : $(BUILD_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

$(BUILD_DIR)/objs/%.c.o : $(MAIN_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

$(BUILD_DIR)/objs/%.cpp.o : $(MAIN_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

#python3 $(ROOT_DIR)/rewrite_altsign_source.py "$<" | $(CC) -x c -I`dirname $<` $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c -
#python3 $(ROOT_DIR)/rewrite_altsign_source.py "$<" | $(CXX) -x c++ -I`dirname $<` $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c -

$(BUILD_DIR)/AltSign.a : $(objs)
	ar rcs $@ $^

clean::
	rm -rf $(ALTSIGN_NEWROOT)
	rm -f $(objs) AltSign.a

all :: preprocess $(BUILD_DIR)/AltSign.a
.PHONY : all

.DEFAULT_GOAL := all
