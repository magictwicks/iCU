#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/wireless.h>
#include <time.h>

#define PORT 28900
#define BUFFER_SIZE 1024
#define IFNAME "wlp0s20f3"
#define TIMER_LENGTH 900


void get_bssid(char *bssid_buffer, size_t buffer_size) {
    FILE *fp;
    char line[512];

    // Adjust "wlan0" to your actual wireless interface if necessary
    fp = popen("iw dev wlp0s20f3 link", "r");
    if (fp == NULL) {
        perror("popen");
        snprintf(bssid_buffer, buffer_size, "Unknown");
        return;
    }

    // Read line-by-line
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "Connected to") != NULL) {
            // Example: "Connected to 12:34:56:78:9A:BC (on wlan0)"
            char *start = strstr(line, "Connected to ");
            if (start != NULL) {
                start += strlen("Connected to ");
                char *end = strchr(start, ' '); // Find first space after MAC
                if (end != NULL) {
                    *end = '\0'; // Null-terminate the MAC address
                    snprintf(bssid_buffer, buffer_size, "%s", start);
                    pclose(fp);
                    return;
                }
            }
        }
    }

    pclose(fp);
    snprintf(bssid_buffer, buffer_size, "Unknown");
}

int main(int argc, char *argv[]) {
    
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char buffer[BUFFER_SIZE];
    int bytes_read;
    time_t last_accessed;

    // Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // listen on all network interfaces
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
      perror("bind failed");
      close(server_fd);
      exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 5) < 0) {
      perror("listen failed");
      close(server_fd);
      exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
      // Accept an incoming connection
      client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
      if (client_fd < 0) {
        perror("accept failed");
        // Goes back to beginning of loop to check again.
        continue;

    }
      printf("Connection accepted from %s:%d\n",
      inet_ntoa(address.sin_addr), ntohs(address.sin_port));

      // Read data from client
      while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; // null-terminate the received data
        printf("Received: %s\n", buffer); // print for debugging
        
        // first check if the  
        if (last_accessed) {
          time_t current_time = time(NULL);
          if (difftime(current_time, last_accessed) > TIMER_LENGTH) {
            if(strcmp(buffer, "Who are you?\n") == 0) {
              char bssid[IW_ESSID_MAX_SIZE + 1] = {0};
              get_bssid(bssid, sizeof(bssid));
              // so as to easily format a string to send to client          
              char response[256];
              snprintf(response, sizeof(response), "%s %s\n", argv[1], bssid);
              write(client_fd, response, strlen(response));
            }
          }
        }

      }

      if (bytes_read == 0) {
        printf("Client disconnected.\n");
      } else if (bytes_read < 0) {
        perror("read error");
      }

      close(client_fd); 
    }
  return 0; // technically unreachable... i think the return looks nice.
}

