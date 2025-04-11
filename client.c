#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <curl/curl.h>

/*
* Constants
*/
#define SEARCH_PORT 28900
#define HEADER_SKIP_MAGIC 14
#define NETWORK_INTERFACE "en0"

/*
 * Function Prototypes
 */
void report_finding(char *their_user_id, char *their_connection);
void query_server(char *ip_address);

/*
 * Helper Methods
 */
void query_server(char *ip_address) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    int bytes_received;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    // Set timeout for the connection
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(sock);
        return;
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SEARCH_PORT);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection to %s failed\n", ip_address);
        close(sock);
        return;
    }

    printf("Connected to %s:%d\n", ip_address, SEARCH_PORT);

    // Send the query
    char *query = "Who are you?\n";
    if (send(sock, query, strlen(query), 0) < 0) {
        perror("Send failed");
        close(sock);
        return;
    }

    // Recieve response
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Server response: %s\n", buffer);

        // Parse under ID and connection point
        char user_id[32] = {0};
        char connection[128] = {0};

        if (sscanf(buffer, "%s %s", user_id, connection) == 2) {
            // Report the finding to vmwardrobe
            report_finding(user_id, connection);
        }
    } else if (bytes_received == 0) {
        printf("Server closed connection without response\n");
    } else {
        perror("Receive failed");
    }

    close(sock);
}
void report_finding(char *their_user_id, char *their_connection) {
    CURL *curl;
    CURLcode res;
    char url[512];

    // Replace with your user ID
    const char *my_user_id = "YOUR_USER_ID";  // Update this with your user ID

    // Construct the URL
    snprintf(url, sizeof(url),
             "http://vmwardrobe.westmont.edu:28900?i=%s&u=%s&where=%s",
             my_user_id, their_user_id, their_connection);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Perform the request
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        } else {
            printf("Successfully reported finding %s at %s\n",
                   their_user_id, their_connection);
        }

        curl_easy_cleanup(curl);
    }
}
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

            // Call oyr helper method to query the server
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(src_ip), ip_str, INET_ADDRSTRLEN);
            query_server(ip_str);
        }
    }
}

void seek() {
    // Main method for identifying iCU traffic

    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    handle = pcap_open_live(NETWORK_INTERFACE, BUFSIZ, 1, 1000, errbuf);  // NOTE: may need to change "eth0" depending on your interface?
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
    // NOTE: gcc -Wall -pedantic client.c -o client -lpcap
    curl_global_init(CURL_GLOBAL_DEFAULT);

    printf("Starting packet monitoring...\n");
    seek();

    return 0;
}
