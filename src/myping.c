#include "myping.h"

ping_pkt_t prepare_pkt()
{
    ping_pkt_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.hdr.icmp_type = ICMP_ECHO;
    pkt.hdr.icmp_hun.ih_idseq.icd_id = getpid(); // identifier
    for (int i = 0; i < sizeof(pkt.msg) - 1; i++)
        pkt.msg[i] = i + '0';
    pkt.msg[sizeof(pkt.msg) - 1] = 0; // null end.
    pkt.hdr.icmp_hun.ih_idseq.icd_seq = transmitted++;
    pkt.hdr.icmp_cksum = checksum(&pkt, sizeof(pkt));
    return pkt;
};

void intHandler(int dummy)
{
    pingloop = 0;
}

unsigned short checksum(void *b, int len)
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
        printf("Usage: %s <ip>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, intHandler);

    char *ip = argv[1];
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    if (getaddrinfo(ip, NULL, &hints, &res) != 0)
    {
        fprintf(stderr, "Error resolving hostname %s\n", ip);
        return 1;
    }

    struct sockaddr_in *dest = (struct sockaddr_in *) res->ai_addr;
    socklen_t destlen = res->ai_addrlen;

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error creating socket\n");
        freeaddrinfo(res);
        return 1;
    }

    while (pingloop)
    {
        ping_pkt_t pkt = prepare_pkt();

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)dest, destlen) <= 0)
        {
            fprintf(stderr, "Error sending packet to %s\n", ip);
            freeaddrinfo(res);
            return 1;
        }

        char buf[1024];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        if (recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen) <= 0)
        {
            fprintf(stderr, "Error receiving packet from %s\n", ip);
            freeaddrinfo(res);
            return 1;
        }
        else
        {
            gettimeofday(&end_time, NULL);
            printf("Received from %s (%s): icmp_seq=%d time=%dms\n", inet_ntoa(from.sin_addr), ip, received++, (end_time.tv_usec - start_time.tv_usec) / 1000);
        }
        sleep(PING_SLEEP_RATE);
    }

    close(sockfd);
    freeaddrinfo(res);
    return 0;
}