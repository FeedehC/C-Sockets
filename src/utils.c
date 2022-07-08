#include "utils.h"

void error(const char *msg)
{
    fprintf(stderr, "%s errno %d %s\n", msg, errno, strerror(errno));
    // perror(msg); Alternativa a la linea de arriba
    exit(EXIT_FAILURE);
}

/////////////////////////////////