#ifndef _main_h
#define _main_h

#include <arpa/inet.h>

#define BUFSIZE 1024   // default buffer size
#define BACKLOG 10     // how many pending connections queue will hold
#define SSYSLOGNAME "smtp-duplictor"

/* prototypes */
void *get_in_addr(struct sockaddr *sa);
FILE* getTMPFileStream(char **filename);

#endif //define _main_h
