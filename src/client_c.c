#include "utils.h"

void sigint_handler();
void write_in_socket(int sockfd, char * buffer);
int sockfd;

//Cliente tipo C (TCP): Solicita descargar el archivo de la Base de Datos Cars.db en el estado actual
int main(int argc, char *argv[])
{
    int port = 0;
    int byte_count = 0, bytes_received = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUF_SIZE];

    // Se verifica que los argumentos sean 3, sino se indica como invocarlo
    if (argc != 4)
    {
        fprintf(stderr, "Uso: %s <ip_servidor> <puerto> <nombre del archivo a escribir>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[2]);
    signal(SIGINT, sigint_handler);

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
    else
        printf("Conexión exitosa: %s:%s\n", argv[1], argv[2]);    

    char *filename = strdup(argv[3]);

    // Creamos el archivo donde se almacenaran los bits recibidos
    FILE *fp;
    fp = fopen(filename, "ab");
    if (NULL == fp)
        error("ERROR en fopen()");

    //Recibimos los datos en paquetes de 256 bits (BUF_SIZE)
    while ((bytes_received = (int) read(sockfd, buffer, BUF_SIZE)) > 0)
    {
        // printf("Bytes received %d\n",bytes_received);
        // buffer[n] = 0;
        fwrite(buffer, 1, (size_t) bytes_received, fp);
        // printf("%s \n", buffer);
        byte_count += bytes_received;
    }

    //  if(bytes_received < 0)
    //{
    //  printf("\n Read Error \n");
    //}

    printf("Escritos: %d bytes\n", byte_count);
    close(sockfd);
    exit(EXIT_SUCCESS);
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