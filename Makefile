# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

USER_HOME ?= $(shell getent passwd $(or $(SUDO_USER),$(USER)) 2>/dev/null | cut -d: -f6)
OWNER     := $(or $(SUDO_USER),$(USER))
DATA_DIR  := ${USER_HOME}/.local/share/dwm
CFG_DIR   := ${USER_HOME}/.config

SRC = drw.c dwm.c util.c tomlparser.c effects.c
OBJ = ${SRC:.c=.o}

all: dwm

test:
	@./scripts/run-tests.sh

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f dwm ${OBJ} *.orig *.rej

install: all
	# Binary + man page
	install -Dm755 dwm ${DESTDIR}${PREFIX}/bin/dwm
	sed "s/VERSION/${VERSION}/g" dwm.1 | install -Dm644 /dev/stdin ${DESTDIR}${MANPREFIX}/man1/dwm.1
	# Session entry
	test -f ${DESTDIR}/usr/share/xsessions/dwm.desktop || install -Dm644 dwm.desktop ${DESTDIR}/usr/share/xsessions/dwm.desktop
	# Scripts to PATH
	for f in scripts/*; do \
		case "$$(basename $$f)" in autostart*|.xinitrc) continue;; esac; \
		install -Dm755 "$$f" ${DESTDIR}${PREFIX}/bin/$$(basename $$f); \
	done

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm \
		${DESTDIR}${MANPREFIX}/man1/dwm.1 \
		${DESTDIR}/usr/share/xsessions/dwm.desktop
	for f in scripts/*; do \
		case "$$(basename $$f)" in autostart*|.xinitrc) continue;; esac; \
		rm -f ${DESTDIR}${PREFIX}/bin/$$(basename $$f); \
	done

release: dwm
	mkdir -p release
	cp -f dwm dwm.desktop scripts/.xinitrc release/
	cp -rf config scripts release/
	tar -czf release/Omitus-${VERSION}.tar.gz -C release dwm dwm.desktop .xinitrc config scripts

.PHONY: all clean install uninstall release
