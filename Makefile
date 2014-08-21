PROGS	=	reader

CLEANFILES = $(PROGS) arp.o
NO_MAN=
CFLAGS += -Werror -Wall -nostdinc -I/usr/include 
CFLAGS += -Wextra

LDFLAGS += 

.include <bsd.prog.mk>
.include <bsd.lib.mk>

all: $(PROGS)

reader: arp.o
	$(CC) $(CFLAGS) -o ${.TARGET} ${.TARGET:=.c} arp.o $(LDFLAGS)

