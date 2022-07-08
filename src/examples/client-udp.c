#include "utils.h"

int main(int argc, char *argv[])
{
    int sockfd;
    long int len_tx;
    unsigned int length;
    struct sockaddr_in server_addr;
    struct hostent *hostname;
    char buffer[BUF_SIZE];

    if (argc != 5)
    {
        printf("Uso: %s <ip_servidor> <puerto> <char a enviar> <cantidad de bytes por mensaje>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR en socket()");

    server_addr.sin_family = AF_INET;
    hostname = gethostbyname(argv[1]);
    if (hostname == 0)
        error("Dirección del servidor desconocida");

    bcopy((char *)hostname->h_addr, (char *)&server_addr.sin_addr, (size_t)hostname->h_length);
    server_addr.sin_port = htons((uint16_t)atoi(argv[2]));
    length = sizeof(struct sockaddr_in);

    int bytes = atoi(argv[4]);
    int j = 0;
    while (1)
    {
        j++;
        bzero(buffer, BUF_SIZE);
        for (int i = 0; i < bytes; i++)
        {
            buffer[i] = *argv[3];
        }
        buffer[bytes] = '\0';

        printf("[%d] Enviando: %s \n", j, buffer);

        len_tx = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, length);
        if (len_tx < 0)
            error("ERROR en sendto()");
    }
    close(sockfd);
    return 0;
}