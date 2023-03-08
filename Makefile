CFLAGS = -lxcb -lxcb-util -lxcb-xinerama -lxcb-keysyms -Wall
SRC = zswm.c utils.c event.c

zswm: ${SRC}
	${CC} -o $@ ${CFLAGS} ${SRC}
