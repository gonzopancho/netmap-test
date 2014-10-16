PROGS = reader arp_daemon service
CLEANFILES = $(PROGS) arp.o arpd.o ip4.o dispatcher.o ethernet.o receiver.o worker.o cqueue/cqueue.o tqueue.o
NO_MAN=
CFLAGS = -g -O3 -std=c11 -Wall -Werror -Wextra -pipe
LDFLAGS = -pthread

.PHONY = clean

all: $(PROGS)

reader: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
arp_daemon: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
service: arp.o ip4.o ethernet.o worker.o dispatcher.o arpd.o receiver.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
tqueue.o: tqueue.c cqueue/cqueue.o
	$(CC) $(CFLAGS) -Icqueue -c tqueue.c -o tqueue.o
clean:
	rm -f $(CLEANFILES)
