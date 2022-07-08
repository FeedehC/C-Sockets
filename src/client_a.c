#include "utils.h"

void sigint_handler();
void write_in_socket_udp(int sockfd, char * buffer);
int sockfd;
struct sockaddr_in server_addr;
unsigned int length;

//Cliente tipo A (UDP): Envia una Query SQL repetidas veces de forma infinita
int main(int argc, char *argv[])
{
    long int len_rx;
    struct hostent *hostname;
    char *query_sql = "SELECT * FROM Cars WHERE Price > 50000";
    char query_result[FILENAME_MAX];

    if (argc != 3)
    {
        printf("Uso: %s <ip_servidor> <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, sigint_handler);
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

    int j = 0;
    while (1)
    {
        j++;
        printf("[%d] Enviando: %s \n", j, query_sql);
        write_in_socket_udp(sockfd, query_sql);        
        
        len_rx = recvfrom(sockfd, query_result, FILENAME_MAX, 0, (struct sockaddr *)&server_addr, &length);
        //printf("len_rx = %ld \n", len_rx);
        if (len_rx < 0) //Si hay un error en la recepcion
            error("ERROR en recvfrom()");
        else if (len_rx == 0) //Cuando se termina de recibir una consulta
            break;
        printf("[%d] Recibido: %s \n", j, query_result);
              
        sleep(1);
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void write_in_socket_udp(int sockfd, char * buffer)
{   
    length = sizeof(struct sockaddr_in);
    long int len_tx = (int) sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, length);
    if (len_tx < 0)
        error("ERROR en sendto()");
}

void sigint_handler()
{
    printf("\nCerrando cliente mediante SIGINT\n");
    write_in_socket_udp(sockfd, "EXIT");
    close(sockfd);
    exit(EXIT_SUCCESS);
}