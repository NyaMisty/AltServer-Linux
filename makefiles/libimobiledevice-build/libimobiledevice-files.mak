libimobiledevice_src += $(wildcard $(LIB_DIR)/libimobiledevice/src/*.c) $(wildcard $(LIB_DIR)/libimobiledevice/common/*.c)
libimobiledevice_src += $(wildcard $(LIB_DIR)/libimobiledevice-glue/src/*.c)
libimobiledevice_src += $(wildcard $(LIB_DIR)/libusbmuxd/src/*.c)
libimobiledevice_src += $(wildcard $(LIB_DIR)/libusbmuxd/common/*.c)
libimobiledevice_include := -I$(LIB_DIR)/libimobiledevice/include -I$(LIB_DIR)/libimobiledevice-glue/include -I$(LIB_DIR)/libimobiledevice -I$(LIB_DIR)/libusbmuxd/include

libplist_include := -I$(LIB_DIR)/libplist/include
libplist_src := $(wildcard $(LIB_DIR)/libplist/src/*.c) $(LIB_DIR)/libplist/libcnary/node.c $(LIB_DIR)/libplist/libcnary/node_list.c