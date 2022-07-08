#include "utils.h"

int main(int argc, char *argv[])
{
    int sockfd, port;
    struct sockaddr_in6 serv_addr;
    struct hostent *server;

    // Se verifica que los argumentos sean 3, sino se indica como invocarlo
    if (argc != 5)
    {
        fprintf(stderr, "Uso: %s <ipv6_servidor> <puerto> <char a enviar> <cantidad de bytes a enviar>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[2]);

    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR en socket()");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, argv[1], &serv_addr.sin6_addr) != 1)
    {
        // printf("inet pton distinto de 1\n");
        server = gethostbyname(argv[1]);
        inet_pton(AF_INET6, server->h_addr, &serv_addr.sin6_addr);
        if (server == NULL)
        {
            error("ERROR Direccion del servidor inválida");
        }
    }
    serv_addr.sin6_port = htons((uint16_t)port);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR en connect()");

    int bytes = atoi(argv[4]);
    char char_to_send = *argv[3];
    tcp_client_send_loop(sockfd, char_to_send, bytes);

    close(sockfd);
    return 0;
}