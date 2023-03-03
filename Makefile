SRC = zswm.c utils.c

zswm: ${SRC}
	${CC} -o $@ -lxcb -lxcb-xinerama -lX11 -lX11-xcb -lXrandr -lXinerama ${SRC}
