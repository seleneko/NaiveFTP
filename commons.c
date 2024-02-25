#include "inc/commons.h"

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

struct packet *init_packet(short packet_id, short datalen, short flags, char *buffer) {
  struct packet *ret = (struct packet *)malloc(sizeof(struct packet));
  ret->id = id++;
  ret->packet_id = packet_id;
  ret->datalen = datalen;
  ret->flags = flags;
  snprintf(ret->buffer, sizeof(ret->buffer) + 1, "%s", buffer);
  return ret;
}

bool send_str(int sockfd, char *buffer) {
  int num_packets = strlen(buffer) / BUFFER_SIZE + 1;
  short packet_id = 0;
  for (int i = 0; i < num_packets; i++) {
    short endflag = i == num_packets - 1 ? (1 << 0) : 0;
    short datalen = i == num_packets - 1 ? BUFFER_SIZE : strlen(buffer) % BUFFER_SIZE;
    struct packet *cur = init_packet(packet_id++, datalen, (1 << 1) | endflag, buffer + i * BUFFER_SIZE);
    if (write(sockfd, cur, sizeof(struct packet)) < 0) {
      printerr("Write error.");
      return false;
    }
    free(cur);
  }
  return true;
}

bool receive_str(int sockfd, char *buffer, int maxlen) {
  bzero(buffer, maxlen);
  struct packet *recv = (struct packet *)malloc(sizeof(struct packet));
  while (read(sockfd, recv, sizeof(struct packet)) > 0) {
    snprintf(buffer, maxlen, "%s%s", buffer, recv->buffer);
    if (recv->flags | 1) {
      free(recv);
      return true;
    }
  }
  free(recv);
  return false;
}

bool send_file_helper(int sockfd, int fd) {
  int nbytes = 0;
  char buffer[BUFFER_SIZE];
  clear(buffer);
  int packet_id = 0;
  short endflag = 0;
  while ((nbytes = read(fd, buffer, BUFFER_SIZE)) > 0) {
    endflag = nbytes < BUFFER_SIZE ? 1 : 0;
    struct packet *cur = init_packet(packet_id++, nbytes, endflag, buffer);
    if (write(sockfd, cur, sizeof(struct packet)) < 0) {
      printerr("Write error.");
      close(fd);
      return false;
    }
    free(cur);
  }
  if (endflag == 0) {
    struct packet *end = init_packet(packet_id++, 0, 1, NULL);
    write(sockfd, end, sizeof(struct packet));
    free(end);
  }
  clear(buffer);
  return true;
}

bool receive_file_helper(int sockfd, int fd) {
  int nbytes = 0;
  char buffer[BUFFER_SIZE];
  clear(buffer);
  struct packet *recv = (struct packet *)malloc(sizeof(struct packet));
  while ((nbytes = read(sockfd, recv, sizeof(struct packet))) > 0) {
    if (write(fd, recv->buffer, recv->datalen) < 0) {
      printerr("Read error.\n");
      close(fd);
      return false;
    }
    if (recv->flags == 1) {
      break;
    }
  }
  free(recv);

  clear(buffer);
  return true;
}

bool send_file(int sockfd, char *filename) {
  int fd = 0;
  if ((fd = open(filename, O_RDONLY)) < 0) {
    send_str(sockfd, "N");
    printerr("Open error.\n");
    close(fd);
    return false;
  }
  send_str(sockfd, "Y");

  send_file_helper(sockfd, fd);

  char received[1];
  receive_str(sockfd, received, sizeof(received));
  if (received[0] != 'Y') {
    // TODO: 另一端未收到文件情况的处理.
  }

  close(fd);
  return true;
}

bool receive_file(int sockfd, char *filename) {
  char good[1];
  receive_str(sockfd, good, sizeof(good));
  if (good[0] == 'N') {
    printerr("Recv data error.\n");
    return false;
  }

  int fd = 0;
  if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
    printerr("Open error.\n");
    return false;
  }

  receive_file_helper(sockfd, fd);

  send_str(sockfd, "Y");
  close(fd);
  return true;
}
