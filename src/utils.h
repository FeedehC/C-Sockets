#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#include <sqlite3.h>

#define BUF_SIZE 1024
#define MAX_DB_CONNECTIONS 5 // Se mantienen 5 conexiones con la base de datos
#define LONG_QUERY_SIZE 16384

typedef struct
{
    long int bytes_tcp;
    long int bytes_udp;
    long int bytes_tcp6;
    long int bytes_total;
} Bytes_count;

void error(const char *msg);
void reset_bytes(Bytes_count *bytes_count);
void inc_bytes(Bytes_count *bytes_count);
void tcp_client_send_loop(int sockfd, char char_to_send, int bytes);

