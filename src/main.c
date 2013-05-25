#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>

#include "main.h"
#include "smtprelay.h"
#include "configfile.h"


/* globals */
volatile unsigned int keep_going = 1;
volatile unsigned int SIG_received = 0;

char *option_programName = "";
int   option_DontFork = 0;
char *option_ConfigFile = "/etc/duplicator.cfg";

/* prototypes */
void sigchld_handler(int s);
int daemonize();
void prepare_syslog(void);
int handle_NewClient(int cliSocki, struct sockaddr_storage *client_addr);

/*implementation */
void usage() {
    fprintf(stderr, "Usage is %s [options]\n", option_programName);
    fprintf(stderr, "Options\n");
    fprintf(stderr, " -d         don\'t fork to background.\n");
    fprintf(stderr, " -f<file>   read config from <file>.\n");
    fprintf(stderr, " -h         show this help and exit.\n");
    exit(8);
}

int parse_cmdLine(int argc, char *argv[]) {
  //char *p;
  option_programName = argv[0];
  while ((argc > 1) && (argv[1][0] == '-')) {
    //argv[1][1] is the actual option character.
    switch (argv[1][1]) {
        case 'd':
            option_DontFork = 1;
            break;
        case 'f':
            if (argv[1][2]) {
               option_ConfigFile = &argv[1][2];
            } else {
               option_ConfigFile = &argv[2][0];
            }
            break;
        case 'h':
            usage();
        default:
            fprintf(stderr, "Bad option %s\n", argv[1]);
            usage();
    }
    //Now move the argument list up one
    //and move the count down one:
    ++argv;
    --argc;
  }
  return(0);
}

FILE* getTMPFileStream(char **filename) {
    char sfn[17] = "";
    FILE *sfp;
    int fd = -1, len;
    bzero(sfn, sizeof(sfn));
    strncpy(sfn, "/tmp/dupl.XXXXXX", sizeof sfn-1);
    if ((fd = mkstemp(sfn)) == -1 ||
       (sfp = fdopen(fd, "w+")) == NULL) {
          syslog(LOG_ERR, "getTMPFileStream: %s: %s", sfn, strerror(errno));
          if (fd != -1) {
            unlink(sfn);
            close(fd);
          }
          syslog(LOG_ERR, "getTMPFileStream: %s: %s", sfn, strerror(errno));
       return (NULL);
    }
    len = sizeof(char)*sizeof(sfn);
    *filename = (char*)malloc(len);
    bzero(*filename, len);
    strncpy(*filename, sfn, len-1);
    syslog(LOG_ERR, "getTMPFileStream: %s\n", sfn);
    syslog(LOG_ERR, "getTMPFileStream: %s\n", *filename);
    return (sfp);
}

void sigchld_handler(int s) {
   SIG_received = 1;
   switch(s) {
    case SIGCHLD:
         syslog(LOG_INFO, "onSIGCHLD\n");
         while(waitpid(-1, NULL, WNOHANG) > 0);
         break;
    case SIGTERM:
         syslog(LOG_INFO, "onSIGTERM\n");
         keep_going = 0;
         break;
    case SIGUSR1:
         dump_configfile(smtpConfigFile);
         break;
   }
}

int daemonize() {
/* Become a daemon: */
   syslog(LOG_INFO, "forking to background.");
   switch (fork ())
     {
     case -1:                    /* can't fork */
       perror ("fork()");
       exit (3);
     case 0:                     /* child, process becomes a daemon: */
       syslog(LOG_INFO, "child: initializing.");
       close (STDIN_FILENO);
       close (STDOUT_FILENO);
       close (STDERR_FILENO);
       if (setsid () == -1)      /* request a new session (job control) */
         {
           exit (4);
         }
       break;
     default:                    /* parent returns to calling process: */
       syslog(LOG_INFO, "parent: exiting.");
       exit(0);
     }
  syslog(LOG_INFO, "fork() finished.");
  return(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int handle_NewClient(int cliSock, struct sockaddr_storage *client_addr) {
    int res = smtprelay_ClientConnect(cliSock, client_addr);
    return(res);
}

void prepare_syslog(void) {
  openlog(SSYSLOGNAME, LOG_CONS | LOG_PID, LOG_MAIL);
}

char* itoa(int val, int base){

    static char buf[32] = {0};

    int i = 30;

    for(; val && i ; --i, val /= base)

        buf[i] = "0123456789abcdef"[val % base];

    return &buf[i+1];
}

int main(int argc, char *argv[])
{
    int listener, new_fd, fdmax;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    struct sigaction sa;
    int yes=1, i;
    int rv;
    fd_set master, readfds;
    struct timeval tv_select;

    //prepare the log interface
    prepare_syslog();
    syslog(LOG_INFO, "starting up...");

    //first of all parse the commandline...
    parse_cmdLine(argc, argv);

    if (!option_ConfigFile) {
          fprintf(stderr, "no config file given, exiting\n");
          syslog(LOG_ERR, "no config file given, exiting\n");
          exit(8);
    }
    syslog(LOG_INFO, "using ConfigFile: %s", option_ConfigFile);

    //read the configfile
    smtpConfigFile = parse_configfile(option_ConfigFile);
    dump_configfile(smtpConfigFile);

    //lets begin...
    if (option_DontFork == 0) {
      daemonize();
    }
    syslog(LOG_INFO, "initialized.");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(smtpConfigFile->listen_addr, smtpConfigFile->listen_port, &hints, &servinfo)) != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        syslog(LOG_ERR, "server: failed to bind\n");
        return 2;
    }

    inet_ntop(p->ai_family,
              get_in_addr((struct sockaddr *)p->ai_addr),
              s,
              sizeof s);
    syslog(LOG_DEBUG, "listening on: %s", s);
    freeaddrinfo(servinfo); // all done with this structure

    if (listen(listener, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
      perror("sigaction SIGTERM");
      exit(1);
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
      perror("sigaction SIGUSR1");
      exit(1);
    }

    syslog(LOG_INFO, "server: waiting for connections...");
    fdmax = listener; //max fd to select() check for
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    FD_SET(listener, &master); //add listener to the master

    while(keep_going) {  // main accept() loop
        readfds = master; //copy, because select changes the fd_set
        SIG_received = 0;
        tv_select.tv_sec = 5;
        tv_select.tv_usec = 0;
        if (select(fdmax+1, &readfds, NULL, NULL, &tv_select) == -1) {
           if (!SIG_received) {
               perror("select");
               exit(4);
           } else {
             //printf("entering continue due to signal...\n");
             continue;
           }
        }
        for (i = 0; i <= fdmax; i++) {
          if (FD_ISSET(i, &readfds)) {
            if (i == listener) { //this can only be a connect request.
              //handle new connections
                sin_size = sizeof their_addr;
                new_fd = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
                if (new_fd == -1) {
                    perror("accept");
                } else {
                    inet_ntop(their_addr.ss_family,
                              get_in_addr((struct sockaddr *)&their_addr),
                              s,
                              sizeof s
                             );
                    syslog(LOG_INFO, "server: got connection from %s\n", s);

                    if (!fork()) { // this is the child process
                        close(listener); // child doesn't need the listener
                        syslog(LOG_INFO, "processing client %s\n", s);
                        /* Kontrolle in smtprelay.c übergeben: */
                        handle_NewClient(new_fd, &their_addr);
                        close(new_fd);
                        syslog(LOG_INFO, "client %s: connection closed\n", s);
                        exit(0);
                    }
                    //printf("parent:close\n");
                    close(new_fd);  // parent doesn't need this
                } //endif new_fd==-1
            } //endif i=listener
          } //endif FD_ISSET(i, &master)
        } //for i<fdmax
    } //while (keep_going);

    syslog(LOG_INFO, "cleaning up...");
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    close(listener);
    free_configfile(smtpConfigFile);
    syslog(LOG_INFO, "shutdown");

    return EXIT_SUCCESS;
}
