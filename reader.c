#include <sys/types.h>
#include <net/if.h>
#include <net/netmap.h>
#include <net/netmap_user.h>
#include <sys/ioctl.h> 
#include <sys/mman.h>
#include <fcntl.h> // open
#include <stdio.h> // fprintf, perror
#include <string.h> // strcpy

void print_nmreq(struct nmreq *req);

int main() {
	struct netmap_if *nifp;
	struct nmreq req;
	int fd, retval, i, len;
	char *buf;
	void *mem;

	fd = open("/dev/netmap", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error opening /dev/netmap: %d\n", fd);
		return 1;
	}

	bzero(&req, sizeof(req));
	printf("Before registration\n");
	print_nmreq(&req);

	strcpy(req.nr_name, "em0");
	req.nr_version = NETMAP_API;
	retval = ioctl(fd, NIOCREGIF, &req); // register the interface
	if (retval < 0) {
		perror("NIOCREGIF failed");
	}
	printf("After registration\n");
	print_nmreq(&req);

	retval = ioctl(fd, NIOCGINFO, &req); // get info
	if (retval < 0) {
		perror("NIOCGINFO failed");
	}

	printf("After info\n");
	print_nmreq(&req);

	mem = mmap(NULL, req.nr_memsize, PROT_READ|PROT_WRITE, 0, fd, 0);
	nifp = NETMAP_IF(mem, req.nr_offset);



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
