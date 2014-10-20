PROGS = reader arp_daemon service
TESTS = tqueue_test_spsc
CLEANFILES = $(PROGS) arp.o arpd.o ip4.o dispatcher.o ethernet.o receiver.o worker.o cqueue/cqueue.o tqueue.o squeue/squeue.o
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
service: arp.o ip4.o ethernet.o worker.o dispatcher.o arpd.o receiver.o tqueue.o cqueue/cqueue.o squeue/squeue.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
cqueue/cqueue.o: cqueue/cqueue.c
	$(CC) $(CFLAGS) -c ${.IMPSRC} -o ${.TARGET}
squeue/squeue.o: squeue/squeue.c
	$(CC) $(CFLAGS) -c ${.IMPSRC} -o ${.TARGET}
tqueue.o: tqueue.c cqueue/cqueue.o
	$(CC) $(CFLAGS) -c tqueue.c -o tqueue.o
tqueue_test_spsc: tqueue.o cqueue/cqueue.o
	$(CC) $(CFLAGS) -I. -o test/${.TARGET} test/${.TARGET:=.c} ${.ALLSRC} $(LDFLAGS)
clean:
	rm -f $(CLEANFILES)
	cd test; rm -rf $(TESTS)
