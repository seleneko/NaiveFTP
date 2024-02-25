#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "inc/commons.h"

// arg format: username:password@serverip:serverport
#define DEBUG 1

static int sockfd = 0;

void ctrlc_handler(int sig) {
  if (sig == SIGINT) {
    send_str(sockfd, "logout");
    printf("\nso long\n");
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char* argv[]) {
#if DEBUG
  if (argc != 1) {
    printerr("...");
  }
#else
  if (argc != 2) {
    printerr("...");
  }
#endif
  char* arg_ptr = NULL;
  char userinfo[STR_SIZE];
  char server_ip[16];
  int server_port;
#if DEBUG
  char test[] = "root:@127.0.0.1:1234";
  snprintf(userinfo, sizeof(userinfo), "%s", strtok_r(test, "@", &arg_ptr));
#else
  snprintf(username, sizeof(username), "%s", strtok_r(argv[1], "@", &arg_ptr));
#endif
  snprintf(server_ip, sizeof(server_ip), "%s", strtok_r(NULL, ":", &arg_ptr));
  server_port = atoi(strtok_r(NULL, ":", &arg_ptr));

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;                    // TCP
  addr.sin_addr.s_addr = inet_addr(server_ip);  // byte order
  addr.sin_port = htons(server_port);           // byte order

  signal(SIGINT, ctrlc_handler);

  printf("Initializing... ");
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printerr("Failed\n");
    printerr("Socket error.\n");
    exit(EXIT_FAILURE);
  }
  printok("OK\n");

  printf("Connecting... ");
  if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    printerr("Failed\n");
    printerr("Connect error.\n");
    exit(EXIT_FAILURE);
  }
  printok("OK\n");

  char cmd[STR_SIZE];
  snprintf(cmd, sizeof(cmd), "login %s", userinfo);
  send_str(sockfd, cmd);

  int result = 1;

  if (result == 0) {
    printerr("User does not exist or password is incorrect.\n");
  }
  printf("FTP client started up.\n");

  char prompt[] = "ftp > ";

  while (true) {
    clear(cmd);
    printf("%s", prompt);
    fgets(cmd, 256, stdin);
    cmd[strlen(cmd) - 1] = '\0';
    if (cmdequ(cmd, "cd") || cmdequ(cmd, "ls") || cmdequ(cmd, "pwd")) {
      send_str(sockfd, cmd);
      char result[STR_SIZE];
      receive_str(sockfd, result, STR_SIZE);
      printf("%s", result);

    } else if (cmdequ(cmd, "get")) {
      char filename[STR_SIZE];
      snprintf(filename, STR_SIZE, "%s", cmd + sizeof("get"));
      send_str(sockfd, cmd);
      printf("Downloading %s... ", filename);
      receive_file(sockfd, filename);
      printok("OK\n");

    } else if (cmdequ(cmd, "put")) {
      char filename[STR_SIZE];
      snprintf(filename, sizeof(filename), "%s", cmd + sizeof("put"));
      send_str(sockfd, cmd);
      printf("Uploading %s... ", filename);
      send_file(sockfd, filename);
      printok("OK.\n");

    } else if (cmdequ(cmd, "logout") || cmdequ(cmd, "exit")) {
      send_str(sockfd, "logout");
      printf("so long\n");
      break;

    } else if (cmdequ(cmd, "server delenda est")) {
      send_str(sockfd, cmd);
      char result[STR_SIZE];
      receive_str(sockfd, result, STR_SIZE);
      printf("%s\n", result);

    } else if (cmdequ(cmd, "\0")) {
      continue;

    } else {
      printf("command not found: %s\n", cmd);
    }
  }
  return 0;
}
