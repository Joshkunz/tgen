#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#define UDP_SIZE 16
/* due to options fields in the IP header, it's
 * size varies, there will always be at least 80 bytes
 * in the header though. */
#define IP_SIZE 80
#define ETH_SIZE 28
#define ETH_DATAMIN 42

#define PR_UDP 17
const char *endings[] = { "b", "kb", "mb", "gb", "tb" };

void * xmalloc(size_t size) {
    void * out = malloc(size);
    if (out == NULL) {
        fputs("Out of Memory.", stderr);
        exit(1);
    }
    return out;
}

suseconds_t ms_to_us(long millis) { 
    return millis * 1000; 
}

struct timeval sleep_for(long rate, char **packet, long *packet_len) { 
    suseconds_t sleep = 0;
    long packet_size = 1;
    long real_s = rate + (rate / 2);

    sleep = ms_to_us(1000) / (real_s / packet_size);
    while (sleep < 100) {
        packet_size *= 2;
        sleep = ms_to_us(1000) / (real_s / packet_size);
    }
    *packet_len = packet_size;
    *packet = xmalloc(packet_size);
    memset(*packet, 0, packet_size);

    struct timeval out = {
        .tv_sec = 0,
        .tv_usec = sleep,
    };
    return out;
}

void print_rate(long bytes) {
    int ending = 0;
    while (bytes >= 1024) { bytes /= 1024; ending++; }
    printf("%ld %s/s", bytes, endings[ending]);
}

void sendall(int sock, const char * buf, int len) {
    int sent = 0;
    int just_sent = 0;
    while (sent < len) {
        just_sent = send(sock, buf + sent, len - sent, 0);
        if (just_sent == -1) {
            perror("send"); exit(1);
        }
        sent += just_sent;
    }
}

void usage(FILE *to) {
    fprintf(to, "usage: tgen HOST PORT BYTES/SEC\n");
}

int main(int argc, char * argv[]) {
    int aoffset = 0;
    struct addrinfo *res;

    if (argc < 4) { usage(stderr); exit(1); }

    long per_s = atol(argv[aoffset + 3]);

    if (getaddrinfo(argv[aoffset + 1], argv[aoffset + 2], NULL, &res) != 0) {
        perror("getaddrinfo"); exit(1);
    }

    {
        char host[NI_MAXHOST], service[NI_MAXSERV];
        if (getnameinfo(res->ai_addr, res->ai_addrlen, host, 
            sizeof(host), service, sizeof(service), NI_NUMERICSERV) == 0) {
            printf("Serving on %s:%s\n", host, service);
        }
    }

    int sock = socket(PF_INET, SOCK_DGRAM, PR_UDP);
    if (sock == -1) { perror("socket"); exit(1); }

    if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect"); exit(1);
    }

    freeaddrinfo(res);
    
    long packet_len;
    char *packet;
    struct timeval sleep, _sleep;
    sleep = sleep_for(per_s, &packet, &packet_len);

    printf("Serving traffic at: ");
    print_rate(per_s);
    printf("\n");
    while (true) {
        sendall(sock, packet, packet_len);
        _sleep = sleep;
        select(0, NULL, NULL, NULL, &_sleep);
    }

    return 0;
}
