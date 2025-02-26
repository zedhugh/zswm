MAKEFILE_ABS_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_ABS_PATH))
PWD_DIR := $(CURDIR)/
BUILD_DIR := $(addprefix $(MAKEFILE_DIR),build)
SRC_DIR := $(addprefix $(MAKEFILE_DIR),src/)

TARGET_NAME := zswm
TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))
SRC := zswm.c utils.c event.c draw.c config.h window.c

SRC := $(addprefix $(SRC_DIR),$(SRC))
ifneq ($(PWD_DIR),$(MAKEFILE_DIR))
	TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))
endif


$(TARGET_NAME): $(SRC) build

clean:
	${RM} $(TARGET)
	${RM} -r $(BUILD_DIR)

run:
	-$(shell Xephyr :1 &)

wm: $(TARGET_NAME)
	DISPLAY=:1 $(TARGET)

prepare:
	@cmake -S $(SRC_DIR) -B $(BUILD_DIR)
	@cp $(BUILD_DIR)/compile_commands.json $(MAKEFILE_DIR)

build: prepare
	@cmake --build $(BUILD_DIR)
	@cp $(BUILD_DIR)/$(TARGET_NAME) $(TARGET)

install: $(TARGET_NAME)
	@cp $(BUILD_DIR)/$(TARGET_NAME) ~/.local/bin/$(TARGET_NAME)

uninstall:
	${RM} -r ~/.local/bin/$(TARGET_NAME)

reinstall: uninstall install

.PHONY: clean run wm prepare build install uninstall reinstall
