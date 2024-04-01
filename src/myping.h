#ifndef MYPING_H
#define MYPING_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#include <math.h>
#include <float.h>

#define PACKET_SIZE 64
#define PING_SLEEP_RATE 1
#define MAX_TIMEOUT 1

#define max(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

int pingloop = 1;
int transmitted = 0;
int received = 0;
ssize_t recv_pkt_size = 0;
char *host;
float total = .0, max_time = .0, min_time = FLT_MAX;

typedef struct ping_pkt
{
    struct icmp hdr;
    char msg[PACKET_SIZE - sizeof(struct icmp)];
} ping_pkt_t;

static inline unsigned short checksum(void *b, int len);

void intHandler(int dummy);

static inline ping_pkt_t prepare_pkt();

#endif // MYPING_H
