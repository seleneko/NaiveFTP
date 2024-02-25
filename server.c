#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "inc/commons.h"

#define MAGIC_PORT 1234
#define LEN_LISTEN_QUEUE 5

#define SUDO 1  // 客户端是否可以远程关闭服务器端

static FILE *lg;                   // 日志文件
static char server_dir[STR_SIZE];  // 服务器当前打开的目录
static char file_dir[STR_SIZE];    // 日志文件所在的目录

struct client {
  int sockfd;
  struct sockaddr_in addr;
};

void printlog(const char *format, ...) {
  lg = fopen(file_dir, "a+");
  if (lg == NULL) {
    exit(-1);
  }
  va_list args;
  va_start(args, format);
  vfprintf(lg, format, args);
  va_end(args);
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  fclose(lg);
}

void monitor(char *req_name, struct sockaddr_in client_addr) {
  // Request, Address, Port, Date, Time
  char *ip = inet_ntoa(client_addr.sin_addr);
  printlog("%s, %s, %u, ", req_name, ip, client_addr.sin_port);

  struct tm now;
  time_t lt = time(&lt);
  localtime_r(&lt, &now);
  printlog("%d-%02d-%02d, ", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);
  printlog("%02d:%02d:%02d\n", now.tm_hour, now.tm_min, now.tm_sec);
}

void do_cd(int sockfd, char *where) {
  char result[STR_SIZE];
  if (chdir(where) == -1) {
    send_str(sockfd, "No such directory.");
    return;
  }
  getcwd(server_dir, sizeof(server_dir));
  snprintf(result, sizeof(result), "==> %s\n", server_dir);
  send_str(sockfd, result);
}

void do_ls(int sockfd) {
  DIR *dp;
  char result[STR_SIZE];
  struct dirent *entry;
  clear(result);
  if ((dp = opendir(server_dir)) == NULL) {
    send_str(sockfd, "No such directory.");
    return;
  }
  entry = readdir(dp);  // d_name 是 `.`
  entry = readdir(dp);  // d_name 是 `..`, 从这之后开始.
  int counter = 0;
  while ((entry = readdir(dp)) != NULL) {
    char *color = entry->d_type == DT_DIR ? CYAN : "";
    snprintf(result, sizeof(result), "%s%s%-24s%s", result, color, entry->d_name, RESET);
    if (++counter % 3 == 0) {
      snprintf(result, sizeof(result), "%s\n", result);
    }
  }
  if (result[strlen(result) - 1] != '\n') {
    snprintf(result, sizeof(result), "%s\n", result);
  }
  send_str(sockfd, result);
}

void do_pwd(int sockfd) {
  char result[STR_SIZE];
  getcwd(server_dir, sizeof(server_dir));
  snprintf(result, sizeof(result), "%s\n", server_dir);
  send_str(sockfd, result);
}

void do_getfile(int sockfd, char *filename) { send_file(sockfd, filename); }

void do_putfile(int sockfd, char *filename) { receive_file(sockfd, filename); }

void server_exec(void *cliptr) {
  struct client cli = *(struct client *)cliptr;
  int sockfd = cli.sockfd;
  struct sockaddr_in client_addr = cli.addr;
  char request[STR_SIZE];
  char cur_dir[STR_SIZE];
  getcwd(cur_dir, sizeof(cur_dir));
  while (true) {
    while (receive_str(sockfd, request, STR_SIZE)) {
      if (cmdequ(request, "login")) {
        monitor(request, client_addr);
        // do login

      } else if (cmdequ(request, "cd")) {
        monitor(request, client_addr);
        do_cd(sockfd, request + sizeof("cd"));

      } else if (cmdequ(request, "ls")) {
        monitor(request, client_addr);
        do_ls(sockfd);

      } else if (cmdequ(request, "pwd")) {
        monitor(request, client_addr);
        do_pwd(sockfd);

      } else if (cmdequ(request, "get")) {
        monitor(request, client_addr);
        do_getfile(sockfd, request + sizeof("get"));

      } else if (cmdequ(request, "put")) {
        monitor(request, client_addr);
        do_putfile(sockfd, request + sizeof("put"));

      } else if (cmdequ(request, "logout")) {
        monitor(request, client_addr);
        close(sockfd);
        pthread_exit(NULL);
        return;

      } else if (cmdequ(request, "server delenda est")) {
        monitor(request, client_addr);
        if (SUDO) {
          send_str(sockfd, GREEN "Server is closed." RESET);
          close(sockfd);
          pthread_exit(NULL);
          exit(EXIT_SUCCESS);
        } else {
          send_str(sockfd, RED "Not enough permissions." RESET);
        }
      }
    }
  }
  close(sockfd);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  // 初始化服务器地址.
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);
  server_addr.sin_port = htons(MAGIC_PORT);

  getcwd(server_dir, sizeof(server_dir));
  snprintf(file_dir, sizeof(file_dir), "%s/server.log", server_dir);

  // 创建套接字, 选择 Internet 协议族 + 字节流服务.
  int server_sockfd = 0;
  if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printerr("Socket creation failed.\n");
    exit(EXIT_FAILURE);
  }
  // bind: 将创建的 socket 与 address 绑定.
  if ((bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
    printerr("Bind failed.\n");
    exit(EXIT_FAILURE);
  }
  // listen
  if (listen(server_sockfd, LEN_LISTEN_QUEUE) < 0) {
    printerr("Listen failed.\n");
    exit(EXIT_FAILURE);
  }

  while (true) {
    pthread_t client_thread;
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    // 接受客户端连接请求.
    int client_sockfd = 0;
    if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &length)) < 0) {
      printerr("Accept failed.\n");
      exit(EXIT_FAILURE);
    }

    struct client cli;
    cli.sockfd = client_sockfd;
    cli.addr = client_addr;
    pthread_create(&client_thread, NULL, (void *)&server_exec, (void *)&cli);
  }

  return 0;
}
