# xglock - simple xgl screen locker
# See LICENSE file for copyright and license details.

include config.mk

SRC = xglock.c ${COMPATSRC}
OBJ = ${SRC:.c=.o}

all: options xglock

debug: CFLAGS += -DDEBUG -g
debug: LDFLAGS := $(filter-out -s,$(LDFLAGS))
debug: all

options:
	@echo xglock build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC -c ${CFLAGS} $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk arg.h util.h

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

xglock: ${OBJ}
	@echo @${CC} -o $@ ${OBJ} ${LDFLAGS}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f xglock ${OBJ}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f xglock ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/xglock
	@chmod u+s ${DESTDIR}${PREFIX}/bin/xglock

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/xglock

.PHONY: all options clean dist install uninstall
