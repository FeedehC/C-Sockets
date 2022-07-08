#include "utils.h"

int main(int argc, char *argv[])
{
    int sockfd, port;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Se verifica que los argumentos sean 3, sino se indica como invocarlo
    if (argc != 5)
    {
        fprintf(stderr, "Uso: %s <ip_servidor> <puerto> <char a enviar> <cantidad de bytes a enviar>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[2]);

    // Se crea el socket y almacenamos su file descriptor en sockfd
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR en socket()");

    // Se revisa en caso que el host tenga un nombre, en el archivo /etc/hosts o por DNS
    server = gethostbyname(argv[1]);
    if (server == NULL)
        error("ERROR, la direccion del servidor no es válida.\n");

    // Se inicializa la estructura con ceros por las dudas que haya basura
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // Se escribe la direccion retornada por gethostbyname a la estructura
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, (size_t)server->h_length);
    // La funcion htons ayuda a verificar el endianness de los equipos que se quiere comunicar, y asigna el puerto indicado
    serv_addr.sin_port = htons((uint16_t)port);

    // Se intenta conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR en connect()");

    int bytes = atoi(argv[4]);
    char char_to_send = *argv[3];
    tcp_client_send_loop(sockfd, char_to_send, bytes);

    close(sockfd);
    return EXIT_SUCCESS;
}