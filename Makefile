TARGETS	=all clean clobber diff distclean import install uninstall
TARGET	=all

SUBDIRS	=

.PHONY:	${TARGETS} ${SUBDIRS}

PREFIX	=${HOME}
PLUGDIR	=${PREFIX}/.purple/plugins

INSTALL	=install

CC 	=gcc -march=native
INCS 	=`pkg-config pidgin --cflags` `pkg-config purple --cflags`
CFLAGS 	=-Os -Wall -pedantic ${INCS} -shared -fPIC
LDFLAGS =-Wl,--export-dynamic
LDLIBS 	=

all::	pidgin-vim.so

pidgin-vim.so: pidgin-vim.c
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} pidgin-vim.c ${LDLIBS}

${TARGETS}::

clobber distclean:: clean
	${RM} pidgin-vim.so

clean::
	${RM} lint tags core.* a.out *.o

install:: pidgin-vim.so
	install -D pidgin-vim.so ${PLUGDIR}/pidgin-vim.so

uninstall::
	${RM} ${PLUGDIR}/pidgin-vim.so
