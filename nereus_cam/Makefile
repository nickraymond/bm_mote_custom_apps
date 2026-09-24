MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(dir $(MKFILE_PATH))

BUILD_DIR := $(CURRENT_DIR)build

# Any goal that isn't a known target is the bm_protocol path, e.g. `make ../bm_protocol`
BM_PATH := $(abspath $(or $(filter-out setup build all,$(MAKECMDGOALS)),../bm_protocol))

.PHONY: setup build all $(filter-out setup build all,$(MAKECMDGOALS))
.DEFAULT_GOAL := build

setup:
	cmake -S $(BM_PATH) -B $(BUILD_DIR) -DCMAKE_TOOLCHAIN_FILE=$(BM_PATH)/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_BOOTLOADER=1 -DBSP=bm_mote_bristleback_v1_0 -DCMAKE_APP_TYPE=bristleback -DAPP=$(CURRENT_DIR) -DCMAKE_VERBOSE_MAKEFILE=OFF

# `make ../bm_protocol` runs setup; `make setup ../bm_protocol` runs it once
$(filter-out setup build all,$(MAKECMDGOALS)): setup
	@:

build: setup
	$(MAKE) -C $(BUILD_DIR) -j

all: build
