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
	rm -rf release

XSESSIONSDIR ?= /usr/share/xsessions
DATADIR      ?= ${PREFIX}/share

install: all
	# Binary + man page
	install -Dm755 dwm ${DESTDIR}${PREFIX}/bin/dwm
	sed "s/VERSION/${VERSION}/g" dwm.1 | install -Dm644 /dev/stdin ${DESTDIR}${MANPREFIX}/man1/dwm.1
	# Session entry
	test -f ${DESTDIR}${XSESSIONSDIR}/dwm.desktop || install -Dm644 dwm.desktop ${DESTDIR}${XSESSIONSDIR}/dwm.desktop
	# Scripts to PATH
	for f in scripts/*; do \
		case "$$(basename $$f)" in autostart*|.xinitrc|*.pyc|__pycache__) continue;; esac; \
		install -Dm755 "$$f" ${DESTDIR}${PREFIX}/bin/$$(basename $$f); \
	done
	mkdir -p ${DESTDIR}${PREFIX}/libexec/dwm
	sed "s|@PREFIX@|${PREFIX}|g" scripts/dwm-desktop-update-root | install -Dm755 /dev/stdin ${DESTDIR}${PREFIX}/libexec/dwm/dwm-desktop-update-root

install-system: install
	python3 scripts/dwm-desktop-update record-system \
		--source-dir . \
		--destdir "${DESTDIR}" \
		--prefix "${PREFIX}" \
		--manprefix "${MANPREFIX}" \
		--xsessions "${XSESSIONSDIR}" \
		--datadir "${DATADIR}" \
		--commands dwm $$(for f in scripts/*; do case "$$(basename $$f)" in autostart*|.xinitrc|dwm-desktop-update-root|*.pyc|__pycache__) continue;; esac; basename "$$f"; done) \
		--helpers dwm-desktop-update-root \
		--packages $$(scripts/dwm-packages.sh arch build; scripts/dwm-packages.sh arch source-update)

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm \
		${DESTDIR}${MANPREFIX}/man1/dwm.1 \
		${DESTDIR}${XSESSIONSDIR}/dwm.desktop \
		${DESTDIR}${PREFIX}/libexec/dwm/dwm-desktop-update-root \
		${DESTDIR}${PREFIX}/share/dwm/desktop-install.json
	for f in scripts/*; do \
		case "$$(basename $$f)" in autostart*|.xinitrc|*.pyc|__pycache__) continue;; esac; \
		rm -f ${DESTDIR}${PREFIX}/bin/$$(basename $$f); \
	done

release: dwm
	mkdir -p release
	cp -f dwm dwm.desktop scripts/.xinitrc release/
	cp -f install.sh uninstall.sh README.md LICENSE release/ 2>/dev/null || true
	cp -rf config scripts release/
	tar -czf release/dwm-${VERSION}.tar.gz -C release dwm dwm.desktop .xinitrc install.sh uninstall.sh README.md LICENSE config scripts

.PHONY: all clean install install-system uninstall release
