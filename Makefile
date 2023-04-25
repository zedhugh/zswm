MAKEFILE_ABS_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_ABS_PATH))
PWD_DIR := $(CURDIR)/
BUILD_DIR := $(addprefix $(MAKEFILE_DIR),build)

TARGET_NAME := zswm
TARGET := $(addprefix ./,$(TARGET_NAME))
SRC := zswm.c utils.c event.c config.h

ifneq ($(PWD_DIR),$(MAKEFILE_DIR))
	SRC := $(addprefix $(MAKEFILE_DIR),$(SRC))
	TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))
endif


$(TARGET_NAME): $(SRC)
	$(MAKE) build

clean:
	${RM} $(TARGET)
	${RM} -r $(BUILD_DIR)

run: $(TARGET_NAME)
	-$(shell Xephyr :1 &)

wm: $(TARGET_NAME)
	DISPLAY=:1 $(TARGET)

prepare:
	@cmake -S $(MAKEFILE_DIR) -B $(BUILD_DIR)
	@cp $(BUILD_DIR)/compile_commands.json $(MAKEFILE_DIR)

build: prepare
	@cmake --build $(BUILD_DIR)
	@cp $(BUILD_DIR)/$(TARGET_NAME) $(TARGET_NAME)

CFLAGS := $(shell pkg-config pangocairo -libs -cflags)
test:
	$(CC) -g test.c -lcairo -lfontconfig $(CFLAGS); ./a.out; file test.png

.PHONY: clean run wm prepare build test
