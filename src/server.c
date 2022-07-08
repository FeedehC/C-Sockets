#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sqlite3.h>
#include <zdb/zdb.h>

#include "utils.h"

void tcp_server(int port, Bytes_count *bytes_count);
void udp_server(int port, Bytes_count *bytes_count);
void tcp6_server(int port, Bytes_count *bytes_count);

void server_a(int port, sqlite3 **db_connections);
void server_b(int port, sqlite3 **db_connections);
void server_c(int port);

sqlite3 *create_db_Cars();
int callback(void *NotUsed, int column_count, char **value, char **column_name);
void log_query(char *query);

char query_result[LONG_QUERY_SIZE]; // Buffer global para almacenar el resultado de ejecutar la query con callback
sqlite3 *log_conn; // Se asigna un apuntador diferente para las conexiones de los Logs

int main(int argc, char *argv[])
{
    // Se verifica que los argumentos sean 3, sino se indica como invocarlo
    if (argc != 4)
    {
        fprintf(stderr, "Uso: %s <puerto TCP IPv4> <puerto UDP IPv4> <puerto TCP IPv6>\n", argv[0]);
        exit(1);
    }

    // signal(SIGCHLD, SIG_IGN); // Ignorar señal de hijo terminado
    printf("[SERVIDOR] Inicializando...\n");
    sleep(1); // Espera de 1 segundos para simular inicializacion

    // Configuracion de Puertos
    int port_a, port_b, port_c;
    port_a = atoi(argv[1]);
    port_b = atoi(argv[2]);
    port_c = atoi(argv[3]);

    ///////////////////////////
    sqlite3 *db = create_db_Cars();

    // Usando SQLite3
    key_t key_ftok = ftok(".", 'S');
    if (key_ftok < 0)
        error("ERROR en ftok()");

    // Se crea una memoria compartida para todos los procesos que se conectan a la base de datos
    int shared_memory = shmget(key_ftok, sizeof(sqlite3 *) * MAX_DB_CONNECTIONS, (IPC_CREAT | 0660));
    if (shared_memory < 0)
        error("ERROR en shmget()");

    // Se asigna un sqlite3** para tener un conjunto de apuntadores, para las 5 conexiones
    sqlite3 **db_connections = (sqlite3 **)shmat(shared_memory, (void *)0, 0);
    

    if (db_connections == (void *)-1)
        error("ERROR en shmat()");

    // Se abren las conexiones a la base de datos - Notar los flags Readwrite y Fullmutex para ejecutar concurrentemente excepto durante las transacciones
    for (int i = 0; i < MAX_DB_CONNECTIONS; i++)
    {
        sqlite3_open_v2("Cars.db", &db_connections[i], (SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX), NULL);
    }

    // Se abre la unica conexion para logear
    sqlite3_open_v2("Cars.db", &log_conn, (SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX), NULL);

    int pid = fork();
    if (pid < 0)
        error("ERROR en fork()");
    else if (pid == 0)
    { // Si es el hijo (proceso tipo A - UDP)
        server_a(port_a, db_connections);
    }

    pid = fork();
    if (pid < 0)
        error("ERROR en fork()");
    else if (pid == 0)
    { // Si es el hijo (proceso tipo B - TCP6)
        server_b(port_b, db_connections);
    }

    pid = fork();
    if (pid < 0)
        error("ERROR en fork()");
    else if (pid == 0)
    { // Si es el hijo (proceso tipo C - TCP)
        server_c(port_c);
    }

    printf("[SERVIDOR] Esperando finalización de hijos con wait()\n");
    int pid_status = 0;
    // wait(&pid_status);
    waitpid(-1, &pid_status, 0);
    sqlite3_close(db);
    return EXIT_SUCCESS;
}

// Este servidor UDP da servicio a los clientes tipo A
void server_a(int port, sqlite3 **db_connections)
{
    int server_socket, server_len;
    long int len_rx, len_tx;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUF_SIZE];
    char send_buffer[LONG_QUERY_SIZE];

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
        error("ERROR en socket()");
    int flag = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
        error("ERROR en setsockopt()");
    server_len = sizeof(server_addr);
    bzero(&server_addr, (size_t)server_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, (socklen_t)server_len) < 0)
        error("ERROR en bind()");
    client_len = sizeof(struct sockaddr_in);
    while (1)
    {
        bzero(buffer, BUF_SIZE); // El buffer recibe la Query SQL para ejecutar en la DB
        len_rx = recvfrom(server_socket, buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (len_rx < 0)
            error("ERROR en recvfrom()");
        else if (len_rx == 0)
        {
            printf("[SERVIDOR] Se dejó de recibir datos de un cliente tipo A");
            break;
        }

        int pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0) // El hijo
        {
            // Se toma una conexion aleatoria del pool
            sqlite3 *client_db_conn = db_connections[rand() % MAX_DB_CONNECTIONS];

            bzero(send_buffer, LONG_QUERY_SIZE);
            if (sqlite3_exec(client_db_conn, buffer, callback, NULL, NULL) == SQLITE_OK)
            { // Si se ejecuto bien copio el resultado para enviar
                strcpy(send_buffer, query_result);
                log_query(buffer);
            }
            else
            { // Si no mando mensaje de error
                strcpy(send_buffer, "Error al ejecutar la query\n");
            }
            //sqlite3_close_v2(client_db_conn);

            len_tx = sendto(server_socket, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&client_addr, (socklen_t)client_len);
            if (len_tx < 0)
                error("ERROR en sendto()");

            exit(EXIT_SUCCESS);
        }
        else // El padre
        {
            // printf("Soy el UDP padre.\n");
        }

        // Se coloca a 0 el buffer de envio para la proxima iteracion
        bzero(send_buffer, BUF_SIZE);
        bzero(query_result, LONG_QUERY_SIZE);
    }
    printf("[SERVIDOR] UDP Finalizando...\n");
    close(server_socket);
    exit(EXIT_SUCCESS);
}

// Este servidor TCP6 da servicio a los clientes tipo B
void server_b(int port, sqlite3 **db_connections)
{
    int sockfd;
    char str_addr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 server_addr, client_addr;
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR en socket()");
    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
        error("ERROR en setsockopt()");
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons((uint16_t)port);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error("ERROR en bind()");
    listen(sockfd, 5);

    // Esto es lo mismo que IPv4
    char buffer[BUF_SIZE];
    char send_buffer[LONG_QUERY_SIZE];
    socklen_t client_len = sizeof(client_addr);
    while (1)
    {
        int new_client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (new_client_socket < 0)
            error("ERROR on accept");

        // Se lee la direccion IPv6 y se la guarda en str_addr
        inet_ntop(AF_INET6, &(client_addr.sin6_addr), str_addr, sizeof(str_addr));
        printf("[SERVIDOR] Conexión establecida con el Cliente %d - ", new_client_socket);
        printf("Dirección IP: %s - ", str_addr);
        printf("Puerto: %d\n", (int)ntohs(client_addr.sin6_port));

        int pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0) // El hijo
        {
            close(sockfd);
            while (1)
            {
                bzero(buffer, BUF_SIZE);
                sprintf(send_buffer, "La query ejecutada fue un INSERT UPDATE o DELETE y no tiene resultado.\n");
                int len_rx = (int)read(new_client_socket, buffer, sizeof(buffer));
                if (len_rx < 0)
                    error("ERROR en read()");
                if (strcmp(buffer, "EXIT") == 0)
                {
                    printf("[SERVIDOR] Conexión finalizada con el Cliente %d - ", new_client_socket);
                    printf("Dirección IP: %s - ", str_addr);
                    printf("Puerto: %d\n", (int)ntohs(client_addr.sin6_port));
                    close(new_client_socket);
                    exit(EXIT_SUCCESS);
                }

                printf("[SERVIDOR] Mensaje recibido tipo B: %s\n", buffer);

                // Se toma una conexion aleatoria del pool
                sqlite3 *client_db_conn = db_connections[rand() % MAX_DB_CONNECTIONS];

                char **error_msg = malloc(sizeof(char) * 256);
                if (sqlite3_exec(client_db_conn, buffer, callback, NULL, error_msg) == SQLITE_OK)
                { // Si se ejecuto bien copio el resultado para enviar
                    // printf("SQLITEOK\n");
                    strcpy(send_buffer, query_result);
                    log_query(buffer);
                }
                else
                { // Si no mando mensaje de error
                    perror("ERROR en sqlite3_exec()");
                    bzero(send_buffer, LONG_QUERY_SIZE);
                    strcpy(send_buffer, "Error al ejecutar la query\n");
                }
                //sqlite3_close_v2(client_db_conn);

                // printf("errormsg = %s\n", *error_msg);
                // printf("sendbuffer = %s\n", send_buffer);

                int len_tx = (int)write(new_client_socket, send_buffer, sizeof(send_buffer));
                if (len_tx < 0)
                    error("ERROR en write()");

                // Se coloca a 0 el buffer de envio para la proxima iteracion
                bzero(send_buffer, LONG_QUERY_SIZE);
                bzero(query_result, LONG_QUERY_SIZE);
            }
            exit(EXIT_SUCCESS);
        }
        else // El padre
        {
            // printf("Soy el TCP6 padre.\n");
            close(new_client_socket);
        }
    }
}

// Este servidor TCP da servicio a los clientes tipo C, que piden descargar el archivo .db de la base de datos
void server_c(int port)
{
    int sockfd = 0, byte_count = 0;
    char *filename = strdup("Cars.db");
    struct sockaddr_in server_addr, client_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR en socket()");
    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
        error("ERROR en setsockopt()");
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);

    char buffer[BUF_SIZE];
    socklen_t client_len = sizeof(client_addr);
    while (1)
    {
        int new_client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (new_client_socket < 0)
            error("ERROR on accept");

        printf("[SERVIDOR] Conexión establecida con el Cliente %d - ", new_client_socket);
        printf("Dirección IP: %s - ", inet_ntoa(client_addr.sin_addr));
        printf("Puerto: %d\n", (int)ntohs(client_addr.sin_port));

        int pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0) // El hijo
        {
            close(sockfd);
            // Se abre el archivo que se va a enviar
            FILE *fp = fopen(filename, "rb");
            if (fp == NULL)
                error("ERROR en fopen()");
            // Se lee el archivo y se envia al cliente
            while (1)
            {
                // Se leen paquetes de 256 bytes (BUF_SIZE)
                int len_rx = (int)fread(buffer, 1, BUF_SIZE, fp);
                byte_count += len_rx;

                // Si no hubo errores se envian los 256 bytes leidos del archivo
                if (len_rx > 0)
                {
                    write(new_client_socket, buffer, (size_t)len_rx);
                }

                if (len_rx < BUF_SIZE)
                {
                    if (feof(fp))
                    {
                        // printf("End of file\n");
                    }
                    if (ferror(fp))
                        printf("Error al leer el archivo que se desea enviar.\n");
                    printf("Archivo enviado: %s [%d bytes].\n", filename, byte_count);

                    char file_log[BUF_SIZE];
                    sprintf(file_log, "Archivo Cars.db descargado por el cliente %d - IP: %s - Puerto: %d", new_client_socket, inet_ntoa(client_addr.sin_addr), (int)ntohs(client_addr.sin_port));
                    log_query(file_log);

                    break;
                }
            }
            exit(EXIT_SUCCESS);
        }
        else // El padre
        {
            close(new_client_socket);
        }
    }
}

///////////////////////////////////////DATABASE/////////////////////////////////////

sqlite3 *create_db_Cars()
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("Cars.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "ERROR al abrir base de datos: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
    char *sql = "DROP TABLE IF EXISTS Cars;"
                "CREATE TABLE Cars(Id INT, Name TEXT, Price INT);"
                "INSERT INTO Cars VALUES(1, 'Audi', 52642);"
                "INSERT INTO Cars VALUES(2, 'Mercedes', 57127);"
                "INSERT INTO Cars VALUES(3, 'Skoda', 9000);"
                "INSERT INTO Cars VALUES(4, 'Volvo', 29000);"
                "INSERT INTO Cars VALUES(5, 'Bentley', 350000);"
                "INSERT INTO Cars VALUES(6, 'Citroen', 21000);"
                "INSERT INTO Cars VALUES(7, 'Hummer', 41400);"
                "INSERT INTO Cars VALUES(8, 'Volkswagen', 21600);"
                "INSERT INTO Cars VALUES(9, 'Tesla', 65000);"
                "DROP TABLE IF EXISTS Logs;" // Esto esta mal?
                "CREATE TABLE Logs(Id INTEGER PRIMARY KEY, Query TEXT);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
    return db;
}

int callback(void *NotUsed, int column_count, char **value, char **column_name)
{
    (void)NotUsed;
    char buffer[BUF_SIZE];
    for (int i = 0; i < column_count; i++)
    {
        bzero(buffer, BUF_SIZE);
        sprintf(buffer, "%s = %s ; ", column_name[i], value[i] ? value[i] : "NULL");
        strcat(query_result, buffer);
    }
    strcat(query_result, "\n");
    return 0;
}

void log_query(char *query)
{
    /*printf("AQUI LOGEO\n");
    (void) db_connections;
    (void) query;*/
    
    // Se toma una conexion aleatoria del pool
    //sqlite3 *client_db_conn = db_connections[rand() % MAX_DB_CONNECTIONS];

    char log_query[LONG_QUERY_SIZE + 256];
    sprintf(log_query, "INSERT INTO Logs VALUES(NULL, '%s');", query);

    if (sqlite3_exec(log_conn, log_query, callback, NULL, NULL) != SQLITE_OK)
    {
        perror("ERROR al logear query_result");
    }
}