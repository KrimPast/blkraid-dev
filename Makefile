KVER ?= $(shell uname -r)
LK_SRC_DIR_RHEL := /usr/src/kernels/$(KVER)

ROOT_DIR := $(PWD)
BUILD_DIR := "$(ROOT_DIR)/build"
SRC_DIR := "$(ROOT_DIR)/src"

all: build

.PHONY: build
build:
	$(MAKE) -j -C $(LK_SRC_DIR_RHEL) M=$(ROOT_DIR) MO=$(BUILD_DIR) modules 
clean:
	rm $(BUILD_DIR) -r 
