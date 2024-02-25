#ifndef FTP_INC_COMMONS_H_
#define FTP_INC_COMMONS_H_

#include <stdbool.h>

#define clear(x) bzero(x, sizeof(x))
#define cmdequ(x, y) !strncmp(x, y, sizeof(y) - 1)

// sizeof(struct packet) == 512
#define PACKET_SIZE (512)
#define BUFFER_SIZE (PACKET_SIZE - 4 * sizeof(short))  // = 504
#define STR_SIZE (8 * BUFFER_SIZE)                     // = 4032, 字符串最多占用 8 个 packet

#define BLACK "\e[90m"
#define RED "\e[91m"
#define GREEN "\e[92m"
#define YELLOW "\e[93m"
#define BLUE "\e[94m"
#define PURPLE "\e[95m"
#define CYAN "\e[96m"
#define WHITE "\e[97m"
#define RESET "\e[0m"

#define MSG CYAN
#define ERR RED
#define WAR YELLOW
#define OK GREEN

#define printmsg(xx) printf(MSG xx RESET)
#define printerr(xx) printf(ERR xx RESET)
#define printok(xx) printf(OK xx RESET)

static short id = 0;

struct packet {
  short id;                  // 通信编号
  short packet_id;           // 一次通信中 packet 编号
  short datalen;             // 数据实际长度
  short flags;               // 16 个 flag 位 (现在只有一个结束位和一个类型位)
  char buffer[BUFFER_SIZE];  // 待输送的数据
};

struct packet *init_packet(short packet_id, short datalen, short flags, char *buffer);

bool send_str(int sockfd, char *buffer);
bool receive_str(int sockfd, char *buffer, int maxlen);

bool send_file_helper(int sockfd, int fd);
bool receive_file_helper(int sockfd, int fd);

bool send_file(int sockfd, char *filename);
bool receive_file(int sockfd, char *filename);

#endif  // FTP_INC_COMMONS_H_
