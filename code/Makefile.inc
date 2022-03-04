CHERIBASE ?= ${HOME}/cheri
SSHPORT ?= 0

.PHONY: all clean

.c:
	${CC} ${CFLAGS} -o $@ $<

TARGETS = signal_break

all: ${TARGETS}

all-copyexec: ${TARGETS:%=copyexec-%}

copyexec-%: %
	scp -P $(SSHPORT) $< root@127.0.0.1:/root
	ssh -p $(SSHPORT) root@127.0.0.1 -t 'su root -c /root/$<'

clean:
	rm -f ${TARGETS}
