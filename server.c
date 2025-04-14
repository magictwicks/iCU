#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/wireless.h>
#include <time.h>
#include <curl/curl.h>
#include <pthread.h>
#include "client.h"

#define PORT 28900
#define BUFFER_SIZE 1024
#define IFNAME "wlp0s20f3"
#define TIMER_LENGTH 900


/*
 * Global Variables 
*/
int server_fd, client_fd;
char userId[16];

void update_time_alive() {
  CURL *curl_handle;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl_handle = curl_easy_init();

  if (curl_handle) {
      char url[256];
      snprintf(url, sizeof(url), "http://vmwardrobe.westmont.edu:28900?i=%s&uptime=%s", userId, "60");
    
      curl_easy_setopt(curl_handle, CURLOPT_URL, url);
      curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
      res = curl_easy_perform(curl_handle);

      if (res != CURLE_OK)
          fprintf(stderr, "HTTPS request failed: %s\n", curl_easy_strerror(res));
      else
          printf("\n[+] HTTPS request completed successfully!\n");

      curl_easy_cleanup(curl_handle);
  }

  curl_global_cleanup();
}

void *sixty_sec_timer_thread(void *arg) {
    while (1) {
        sleep(60); // Wait for 60 seconds
        update_time_alive();
    }
    return NULL;
}

void handle_exit(int sig) {
  printf("\niCU Server exited (CTRL+C). Updating points on VM Wardrobe.\n");
  update_time_alive();
  close(server_fd);
  close(client_fd);
  exit(0);
}

void get_bssid(char *bssid_buffer, size_t buffer_size) {
    FILE *fp;
    char line[512];

    fp = popen("iw dev wlp0s20f3 link", "r");
    if (fp == NULL) {
        perror("popen");
        snprintf(bssid_buffer, buffer_size, "Unknown");
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "Connected to") != NULL) {
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
  time_t last_accessed = 0;
  pthread_t tid;
  
  // load userId into global var
  snprintf(userId, sizeof(userId), "%s", argv[1]); 
  signal(SIGINT, handle_exit);

  // Create timer thread to update time alive (program is always alive ATM)
  if (pthread_create(&tid, NULL, sixty_sec_timer_thread, NULL) != 0) {
      perror("Failed to create timer thread");
      exit(EXIT_FAILURE);
  }

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

  pthread_t seekid;
  // Create timer thread to update time alive (program is always alive ATM)
  if (pthread_create(&seekid, NULL, seek, NULL) != 0) {
      perror("Failed to create timer thread");
      exit(EXIT_FAILURE);
  }

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
      
        if(strcmp(buffer, "Who are you?\n") == 0) {
          char bssid[IW_ESSID_MAX_SIZE + 1] = {0};
          get_bssid(bssid, sizeof(bssid));
          time_t current_time = time(NULL);
          if (difftime(current_time, last_accessed) > TIMER_LENGTH) {
            char response[256];
            // so as to easily format a string to send to client          
            snprintf(response, sizeof(response), "%s %s\n", argv[1], bssid);
            write(client_fd, response, strlen(response));
            last_accessed = time(NULL);
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

