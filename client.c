#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

/*
* Constants
*/
#define SEARCH_PORT 28900
#define HEADER_SKIP_MAGIC 14

/*
 * Helper Methods
 */
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    // Per pcap_* struct

    struct ip *ip_hdr = (struct ip *)(packet + HEADER_SKIP_MAGIC); // skip header
    // if (ip_hdr->ip_v != 4) return;

    if (ip_hdr->ip_p == IPPROTO_TCP) {
        struct tcphdr *tcp_hdr = (struct tcphdr *)(packet + HEADER_SKIP_MAGIC + (ip_hdr->ip_hl * 4)); // skip header
        struct in_addr src_ip = ip_hdr->ip_src;
        printf("Packet Source: %s:%d\n", inet_ntoa(src_ip), ntohs(tcp_hdr->th_sport));

        if (ntohs(tcp_hdr->th_sport) == SEARCH_PORT) {
            printf("~~~~~~~~~~~~~~~~~~~~~~\nGotcha! %s\n~~~~~~~~~~~~~~~~~~~~~~\n", inet_ntoa(src_ip));
            // TODO: Do something with the IP.
            // Send "Who are you?\n" tp inet_ntoa(src_ip)
            // Make this a helper method.
        }
    }
}

void seek() {
    // Main method for identifying iCU traffic

    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    handle = pcap_open_live("eth0", BUFSIZ, 1, 1000, errbuf);  // NOTE: may need to change "eth0" depending on your interface?
    if (handle == NULL) {
        fprintf(stderr, "Error opening device: %s\n", errbuf);
        return;
    }

    pcap_loop(handle, 0, packet_handler, NULL);
    pcap_close(handle);
}

/*
 * Main
 */
int main() {
    printf("Starting packet monitoring...\n");
    seek();

    return 0;
}
