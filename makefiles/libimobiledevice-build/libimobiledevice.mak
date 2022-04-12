ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(ROOT_DIR)/../main.mak

%.c.o : %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

%.cpp.o : %.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

CFLAGS += -DHAVE_CONFIG_H -DDEBUG -O0 -g


$(BUILD_DIR)/objs/%.c.o : $(MAIN_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

$(BUILD_DIR)/objs/%.cpp.o : $(MAIN_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

include $(ROOT_DIR)/libimobiledevice-files.mak

libimobiledevice_obj := $(libimobiledevice_src:$(MAIN_DIR)/%=$(BUILD_DIR)/objs/%.o)
$(libimobiledevice_obj) : EXTRA_FLAGS := -I$(ROOT_DIR) $(libimobiledevice_include) $(libplist_include) -I$(LIB_DIR)/libimobiledevice/common -I$(LIB_DIR)/libusbmuxd/common
$(BUILD_DIR)/libimobiledevice.a : $(libimobiledevice_obj)
	ar rcs $@ $^

libplist_obj := $(libplist_src:$(MAIN_DIR)/%=$(BUILD_DIR)/objs/%.o)
$(libplist_obj) : EXTRA_FLAGS := -I$(ROOT_DIR) $(libplist_include) -I$(LIB_DIR)/libplist/libcnary/include -I$(LIB_DIR)/libplist/src
$(BUILD_DIR)/libplist.a : $(libplist_obj)
	ar rcs $@ $^


#allsrc += $(libimobiledevice_src) 
#allsrc += $(libplist_src)
#allobj = $(addsuffix .o, $(allsrc))


clean::
	rm -f $(libimobiledevice_obj)
	rm -f $(libplist_obj)
	rm -f $(BUILD_DIR)/libplist.a $(BUILD_DIR)/libimobiledevice.a
.PHONY : clean

all :: $(BUILD_DIR)/libplist.a
all :: $(BUILD_DIR)/libimobiledevice.a
.PHONY : all

.DEFAULT_GOAL := all
