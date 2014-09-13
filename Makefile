PROGS = reader arpd service
CLEANFILES = $(PROGS) arp.o ip4.o ethernet.o worker.o dispatcher.o
NO_MAN=
CFLAGS += -Werror -Wall -nostdinc -I/usr/include 
CFLAGS += -Wextra

LDFLAGS += -pthread

.include <bsd.prog.mk>
.include <bsd.lib.mk>

all: $(PROGS)

reader: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o $(LDFLAGS)
arpd: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o $(LDFLAGS)
service: arp.o ip4.o ethernet.o worker.o dispatcher.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o worker.o dispatcher.o $(LDFLAGS)
