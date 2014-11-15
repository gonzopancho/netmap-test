PROGS = reader arp_daemon service
TESTS = tqueue_test_spsc bitmap_test
CLEANFILES = $(PROGS) arp.o arpd.o bitmap.o dispatcher.o ethernet.o ip4.o message.o receiver.o sender.o tqueue.o worker.o cqueue/cqueue.o squeue/squeue.o
NO_MAN=
CFLAGS = -g -O3 -march=native -std=c11 -Wall -Werror -Wextra -Wpedantic -pipe
LDFLAGS = -pthread

.PHONY = clean

all: $(PROGS)
tests: $(TESTS)
reader: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
arp_daemon: arp.o ip4.o ethernet.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
service: arp.o arpd.o bitmap.o dispatcher.o ethernet.o ip4.o message.o receiver.o sender.o tqueue.o worker.o cqueue/cqueue.o squeue/squeue.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
cqueue/cqueue.o: cqueue/cqueue.c
	$(CC) $(CFLAGS) -c ${.IMPSRC} -o ${.TARGET}
squeue/squeue.o: squeue/squeue.c
	$(CC) $(CFLAGS) -c ${.IMPSRC} -o ${.TARGET}
tqueue.o: tqueue.c cqueue/cqueue.o
	$(CC) $(CFLAGS) -c tqueue.c -o tqueue.o
tqueue_test_spsc: tqueue.o cqueue/cqueue.o
	$(CC) $(CFLAGS) -I. -o test/${.TARGET} test/${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
bitmap_test: bitmap.o
	$(CC) $(CFLAGS) -I. -o test/${.TARGET} test/${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)

clean:
	rm -f $(CLEANFILES)
	cd test; rm -rf $(TESTS)
