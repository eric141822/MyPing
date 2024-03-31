#include "myping.h"

static inline ping_pkt_t prepare_pkt()
{
    ping_pkt_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.hdr.icmp_type = ICMP_ECHO;
    pkt.hdr.icmp_hun.ih_idseq.icd_id = getpid(); // identifier
    for (int i = 0; i < sizeof(pkt.msg); i++)
        pkt.msg[i] = '0';

    pkt.hdr.icmp_hun.ih_idseq.icd_seq = transmitted++;
    pkt.hdr.icmp_cksum = checksum(&pkt, sizeof(pkt));
    return pkt;
};

void intHandler(int dummy)
{
    pingloop = 0;
    printf("\n--- %s ping statistics ---\n", host);
    printf("%d packets transmitted, %d received, %d%\% packet loss\n", transmitted, received, (transmitted - received) * 100 / transmitted);
}

static inline unsigned short checksum(void *b, int len)
{
    unsigned short *buf = (unsigned short *)b;
    unsigned int sum = 0;
    unsigned short result;
    for (; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, intHandler);

    host = argv[1];
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;
    hints.ai_flags = AI_CANONNAME;

    if (getaddrinfo(host, NULL, &hints, &res) != 0)
    {
        fprintf(stderr, "Error resolving hostname %s\n", host);
        return 1;
    }

    struct sockaddr_in *dest = (struct sockaddr_in *)res->ai_addr;
    socklen_t destlen = res->ai_addrlen;

    int sockfd = socket((res->ai_family == AF_INET) ? AF_INET : AF_INET6, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error creating socket\n");
        freeaddrinfo(res);
        return 1;
    }

    struct timeval timeout = {MAX_TIMEOUT, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    printf("PING %s (%s) %d bytes of data.\n", res->ai_canonname, inet_ntoa(dest->sin_addr), PACKET_SIZE);
    while (pingloop)
    {
        ping_pkt_t pkt = prepare_pkt();

        struct timeval start_time, end_time;
        int duration = 0;
        gettimeofday(&start_time, NULL);

        if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)dest, destlen) <= 0)
        {
            fprintf(stderr, "Error sending packet to %s\n", res->ai_canonname);
            freeaddrinfo(res);
            return 1;
        }

        char buf[1024];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);

        if ((recv_pkt_size = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen)) <= 0)
        {
            // timeout
            printf("Request timeout for icmp_seq %d\n", transmitted);
        }
        else
        {
            gettimeofday(&end_time, NULL);
            duration = (end_time.tv_usec - start_time.tv_usec) / 1000;
            printf("%zd bytes received from %s (%s): icmp_seq=%d time(RTT)=%dms\n", recv_pkt_size, inet_ntoa(from.sin_addr), res->ai_canonname, received++, (duration >= 0) ? duration : 0);
        }
        sleep(PING_SLEEP_RATE);
    }

    close(sockfd);
    freeaddrinfo(res);
    return 0;
}