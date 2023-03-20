MAKEFILE_ABS_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_ABS_PATH))
PWD_DIR := $(CURDIR)/

LOG_FILE = /home/zedhugh/.zswm-log.txt
LOG_FLAG = -DLOG_FILE=\"${LOG_FILE}\"
CFLAGS = -lxcb -lxcb-util -lxcb-xinerama -lxcb-keysyms -Wall ${LOG_FLAG}
SRC := zswm.c utils.c event.c

TARGET_NAME := zswm
TARGET := $(addprefix ./,$(TARGET_NAME))

ifneq ($(PWD_DIR),$(MAKEFILE_DIR))
	SRC := $(addprefix $(MAKEFILE_DIR),$(SRC))
	TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))
endif


$(TARGET_NAME): $(SRC)
	${CC} -o $(TARGET) $(CFLAGS) $(SRC)

clean:
	${RM} $(TARGET)

install: $(TARGET_NAME)
	cp $(TARGET) ~/.local/bin/

uninstall:
	${RM} ~/.local/bin/${TARGET_NAME}

run: $(TARGET_NAME)
	-$(shell Xephyr :1 &)
	-$(shell sleep 1)
	-$(shell DISPLAY=:1 $(TARGET) &)

wm: $(TARGET_NAME)
	DISPLAY=:1 $(TARGET)


.PHONY: clean install uninstall startx run
