PROGS = reader arp_daemon service
CLEANFILES = $(PROGS) arp.o ip4.o ethernet.o worker.o dispatcher.o cqueue/cqueue.o tqueue.o
NO_MAN=
CFLAGS += -g -O3 -std=c11 -Wall -Werror -Wextra -nostdinc -I/usr/include 
LDFLAGS += -pthread

#.include <bsd.prog.mk>
#.include <bsd.lib.mk>

all: $(PROGS)

reader: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o $(LDFLAGS)
arp_daemon: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o $(LDFLAGS)
service: arp.o ip4.o ethernet.o worker.o dispatcher.o arpd.o receiver.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o ip4.o ethernet.o worker.o dispatcher.o arpd.o receiver.o $(LDFLAGS)
tqueue.o: tqueue.c cqueue/cqueue.o
	$(CC) $(CFLAGS) -Icqueue -c tqueue.c -o tqueue.o
clean:
	rm -f $(CLEANFILES)
