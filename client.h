#ifndef CLIENT_H
#define CLIENT_H

#include <pcap.h>

/*
 * Constants
 */
#define SEARCH_PORT 28900
#define HEADER_SKIP_MAGIC 14
#define NETWORK_INTERFACE "wlp0s20f3"

/*
 * Function Prototypes
 */
void report_finding(char *their_user_id, char *their_connection);
void query_server(char *ip_address);
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet);
void *seek(void *arg);

#endif // CLIENT_H

