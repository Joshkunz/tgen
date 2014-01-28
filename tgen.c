#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

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

useconds_t ms_to_us(long millis) { 
    return millis * 1000; 
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

int main() {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    if (getaddrinfo("localhost", "5001", &hints, &res) != 0) {
        perror("getaddrinfo"); exit(1);
    }

    int sock = socket(PF_INET, SOCK_DGRAM, PR_UDP);
    if (sock == -1) { perror("socket"); exit(1); }

    if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect"); exit(1);
    }

    {
        char host[NI_MAXHOST], service[NI_MAXSERV];
        if (getnameinfo(res->ai_addr, res->ai_addrlen, host, 
            sizeof(host), service, sizeof(service), NI_NUMERICSERV) == 0) {
            printf("Serving on %s:%s\n", host, service);
        }
    }
    freeaddrinfo(res);

    long per_s = (1024 * 1024) * 10;
    //long per_s = (1024 * 100);
    long real_s = per_s + (per_s / 2);
    long packet_size = 1;
    useconds_t sleep = 0;

    sleep = ms_to_us(1000) / (real_s / packet_size);
    while (sleep < 100) {
        packet_size *= 2;
        sleep = ms_to_us(1000) / (real_s / packet_size);
    }
    char * packet = xmalloc(packet_size);
    memset(packet, 0, packet_size);

    printf("Serving traffic at: ");
    print_rate(per_s);
    printf("\n");
    while (true) {
        sendall(sock, packet, packet_size);
        usleep(sleep);
    }

    return 0;
}
