#include <sys/types.h>
#include <net/if.h>
#include <net/netmap.h>
#include <net/netmap_user.h>
#include <sys/ioctl.h> 
#include <sys/mman.h>
#include <fcntl.h> // open
#include <stdio.h> // fprintf, perror
#include <string.h> // strcpy
#include <poll.h>
#include <unistd.h> // sleep, close


void print_nmreq(struct nmreq *req);
void print_netmap_if(struct netmap_if *nif);
void print_ring(struct netmap_ring *ring, uint32_t ridx);
void print_ring2(struct netmap_ring *ring, uint32_t ridx);
void print_buf(char *buf, uint16_t len);
void print_buf2(char *buf, uint16_t len); 


int main() {
	struct netmap_if *nifp;
	struct netmap_ring *ring;
	struct nmreq req;
	struct pollfd pfd;
	int fd, retval;
	uint32_t i;
	char *buf;
	void *mem;

	fd = open("/dev/netmap", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error opening /dev/netmap: %d\n", fd);
		close(fd);
		return 1;
	}
	
	pfd.fd = fd;
	pfd.events = (POLLIN);

	bzero(&req, sizeof(req));
	strcpy(req.nr_name, "em0");
	req.nr_version = NETMAP_API;
	req.nr_ringid = NETMAP_NO_TX_POLL;

	/* register the NIC for netmap mode */
	retval = ioctl(fd, NIOCREGIF, &req);
	if (retval < 0) {
		perror("NIOCREGIF failed");
	}

	/* give the NIC some time to unregister from the host stack */
	sleep(2);

	printf("After registration\n");
	print_nmreq(&req);

	mem = mmap(0, req.nr_memsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (mem == MAP_FAILED) {
		perror("mmap failed");
	}

	nifp = NETMAP_IF(mem, req.nr_offset);
	print_netmap_if(nifp);

	ring = NETMAP_RXRING(nifp, 0);
	print_ring(ring, 0);

	/* main poll loop */
	for(;;) {
		retval = poll(&pfd, 1, INFTIM);
		if (retval < 0) {
			perror("poll failed");
			continue;
		}

		ring = NETMAP_RXRING(nifp, 0);
		for (; ring->avail > 0; ring->avail--) {
 			print_ring2(ring, 0);
 			i = ring->cur;
 			buf = NETMAP_BUF(ring, ring->slot[i].buf_idx);	
 			print_buf2(buf, ring->slot[i].len);
 			ring->cur = NETMAP_RING_NEXT(ring, i);
		}
	}

	return 0;
}


void print_nmreq(struct nmreq *req) {
	printf("nr_name: %s\n", req->nr_name);
	printf("nr_version: %u\n", req->nr_version);
	printf("nr_offset: %u\n", req->nr_offset);
	printf("nr_memsize: %u\n", req->nr_memsize);
	printf("nr_tx_slots: %u\n", req->nr_tx_slots);
	printf("nr_rx_slots: %u\n", req->nr_rx_slots);
	printf("nr_tx_rings: %hu\n", req->nr_tx_rings);
	printf("nr_rx_rings: %hu\n", req->nr_rx_rings);
	printf("nr_ringid: %hu\n", req->nr_ringid);
	printf("nr_cmd: %hu\n", req->nr_cmd);
	printf("nr_arg1: %hu\n", req->nr_arg1);
	printf("nr_arg2: %hu\n", req->nr_arg2);
	printf("spare2: %u %u %u\n", req->spare2[0], req->spare2[1], req->spare2[2]);
}

void print_netmap_if(struct netmap_if *nif) {
	printf("ni_name: %s\n", nif->ni_name);
	printf("ni_version: %u\n", nif->ni_version);
	printf("ni_rx_rings: %u\n", nif->ni_rx_rings);
	printf("ni_tx_rings: %u\n", nif->ni_tx_rings);
	printf("ring_ofs: %zd\n", nif->ring_ofs[0]);
}

void print_ring(struct netmap_ring *ring, uint32_t ridx) {
	printf("ring: %u\n", ridx);
	printf("buf_ofs: %zd\n", ring->buf_ofs);
	printf("num_slots: %u\n", ring->num_slots);
	printf("avail: %u\n", ring->avail);
	printf("cur: %u\n", ring->cur);
	printf("reserved: %u\n", ring->reserved);
	printf("flags: %hu\n", ring->flags);
	/* printf("ts: unknown\n"); */
}

void print_ring2(struct netmap_ring *ring, uint32_t ridx) {
	    printf("ring: %u avail: %u cur: %u res: %u\n", 
				ridx, ring->avail, ring->cur, ring->reserved);
}

void print_buf(char *buf, uint16_t len) {
	uint16_t i;
	printf("*****buf*****\n");

	for(i = 0; i + 4 < len; i+=4) {
		printf("%.2X %.2X %.2X %.2X\n", buf[i] & 0xFF, buf[i+1] & 0xFF, buf[i+2] & 0xFF, buf[i+3] & 0xFF);
	}
	for(; i < len; i++) {
		printf("%.2X ", buf[i] & 0xFF);
	}
	printf("\n");

}

void print_buf2(char *buf, uint16_t len) {
	uint16_t i;
	printf("*****buf(%hu)*****\n", len);
	if ( len > 53)
		len = 53;
	for(i = 0; i < len; i++)
		printf("%.2x ", buf[i] & 0xFF);
	printf("\n");	
}
