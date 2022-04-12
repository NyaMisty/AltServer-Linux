_ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(_ROOT_DIR)/main.mak

SHIM_DIR := $(MAIN_DIR)/shims

CFLAGS += -I$(SHIM_DIR) -include windows_shim.h

shim_src := $(wildcard $(SHIM_DIR)/*.cpp)