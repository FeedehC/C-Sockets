#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "utils.h"

int main(int argc, char const *argv[])
{

    int byte_count = 0;
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    char sendBuff[1025];
    //int numrv;

    // checking if correct number of arguments are given on the command line
    if (argc != 3)
    {
        fprintf(stderr, "%s", "wrong command line arguments\n");
        exit(1);
    }
    // converting arguments to their correct types to use them e.g. port num and filename
    int port = (int) strtol(argv[1], NULL, 10);
    char *filename = strdup(argv[2]);
    // printf("%s",filename);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // printf("Socket retrieve success\n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t) port);

    // bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
    // checking if bind fails??

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        fprintf(stderr, "%s", "Bind Failed\n");
        exit(2);
    }
    else
        printf("BindDone: %s\n", argv[1]);

    //}

    if (listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }
    else
    {
        printf("ListenDone: %s\n", argv[1]);
    }

    // while(1)
    //{
    connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

    /* Open the file that we wish to transfer */
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {

        // checking if there is some problem with file or file present or not??

        fprintf(stderr, "%s", "File open error\n");
        exit(3);
    }

    /* Read data from file and send it */
    while (1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[256] = {0};
        int nread = (int) fread(buff, 1, 256, fp);
        byte_count += nread;

        //  printf("Bytes read %d \n", nread);

        /* If read was success, send data. */
        if (nread > 0)
        {
            // printf("Sending \n");
            write(connfd, buff, (size_t) nread);
        }

        if (nread < 256)
        {
            printf("TransferDone: %d bytes\n", byte_count);
            if (feof(fp))
            {

                // printf("End of file\n");
            }

            if (ferror(fp))
                printf("Error reading\n");
            break;
        }
    }

    close(connfd);
    sleep(1);
    //}

    return 0;
}
