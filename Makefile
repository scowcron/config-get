EXEC = config-get
LDFLAGS = -lconfig
CFLAGS = --std=c99 -Wall
SOURCES = src/config-get.c
DESTDIR ?= /usr/bin

default: dist

dist: CFLAGS += -O2
dist: ${EXEC}

devel: CFLAGS += -Werror -g
devel: ${EXEC}

${EXEC}: ${SOURCES}
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o $@

install: dist
	mkdir -p ${DESTDIR}
	install ${EXEC} ${DESTDIR}

clean:
	-rm ${EXEC}
