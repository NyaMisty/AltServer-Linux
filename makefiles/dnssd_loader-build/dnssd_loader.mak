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

include $(ROOT_DIR)/dnssd_loader-files.mak

dnssd_loader_obj := $(dnssd_loader_src:$(MAIN_DIR)/%=$(BUILD_DIR)/objs/%.o)
$(dnssd_loader_obj) : EXTRA_FLAGS := -I$(ROOT_DIR) $(dnssd_loader_include)
$(BUILD_DIR)/dnssd_loader.a : $(dnssd_loader_obj)
	ar rcs $@ $^

clean::
	rm -f $(dnssd_loader_obj)
	rm -f $(BUILD_DIR)/dnssd_loader.a
.PHONY : clean

all :: $(BUILD_DIR)/dnssd_loader.a
.PHONY : all

.DEFAULT_GOAL := all
