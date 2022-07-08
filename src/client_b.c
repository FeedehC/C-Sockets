#include "utils.h"

void sigint_handler();
void write_in_socket(int sockfd, char * buffer);
int sockfd;

// Cliente tipo B (TCP6): Muestra una consola para ejecutar Querys SQL y recibir el resultado en pantalla, en caso de error solo imprime el mensaje de error
int main(int argc, char *argv[])
{
    int port;
    struct sockaddr_in6 serv_addr;
    struct hostent *server;

    // Se verifica que los argumentos sean 3, sino se indica como invocarlo
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <ipv6_servidor> <puerto>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[2]);
    signal(SIGINT, sigint_handler);

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

    char buffer[BUF_SIZE];
    char result[LONG_QUERY_SIZE];
    int j = 0;
    while (1)
    {
        j++;
        bzero(buffer, BUF_SIZE); // Se vacia el buffer
        printf("Ingrese sentencias SQL: ");
        //scanf("%s", buffer);
        fgets(buffer, BUF_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0'; // Se agrega el null terminated char
        //strcat(buffer, "\0");

        if (strcmp(buffer, "EXIT") == 0)
        {
            printf("Cerrando cliente\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        printf("[%d] Enviando: %s \n", j, buffer);
        write_in_socket(sockfd, buffer);

        bzero(buffer, BUF_SIZE); // Se vacia el buffer
        int len_rx = (int)read(sockfd, result, LONG_QUERY_SIZE);
        if (len_rx < 0)
            error("ERROR en read()");

        printf("[%d] Recibido: %s \n", j, result);
    }

    close(sockfd);
    return 0;
}

void write_in_socket(int sockfd, char * buffer)
{   
    int len_tx = (int)write(sockfd, buffer, strlen(buffer));
    if (len_tx < 0)
        error("ERROR en write()");
}

void sigint_handler()
{
    printf("\nCerrando cliente mediante SIGINT\n");
    write_in_socket(sockfd, "EXIT");
    close(sockfd);
    exit(EXIT_SUCCESS);
}