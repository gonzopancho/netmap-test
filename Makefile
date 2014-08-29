PROGS = reader arpd
CLEANFILES = $(PROGS) arp.o ip4.o ethernet.o
NO_MAN=
CFLAGS += -Werror -Wall -nostdinc -I/usr/include 
CFLAGS += -Wextra

LDFLAGS += 

.include <bsd.prog.mk>
.include <bsd.lib.mk>

all: $(PROGS)

reader: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o $(LDFLAGS)
arpd: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o $(LDFLAGS)
