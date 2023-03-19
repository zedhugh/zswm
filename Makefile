LOG_FILE = /home/zedhugh/.zswm-log.txt
LOG_FLAG = -DLOG_FILE=\"${LOG_FILE}\"
CFLAGS = -lxcb -lxcb-util -lxcb-xinerama -lxcb-keysyms -Wall ${LOG_FLAG}
SRC = zswm.c utils.c event.c

zswm: ${SRC}
	${CC} -o $@ ${CFLAGS} ${SRC}

clean:
	${RM} ./zswm

install: zswm
	cp zswm ~/.local/bin/

uninstall:
	${RM} ~/.local/bin/zswm

.PHONY: clean install uninstall
