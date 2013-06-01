#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "main.h"
#include "smtprelay.h"
#include "configfile.h"
#include "duplicator.h"

int smtprelay_ClientConnect(int cliSock, struct sockaddr_storage *client_addr) {
    /* Diese procedure ist der Kontroll-Übergabepunkt aus der main.c  */

    //Fehlerbehandlung...
    if (cliSock < 0) {
      return(1);
    }
    //store client information for later use in global var:
    currSession.client_addr = client_addr;

    //begin relaying...
    return(smtprelay_doprocessing(cliSock));
    //if (send(cliSock, "Hello, world!!!\n", 16, 0) == -1) {
    //    perror("send");
    //};
};

void smtprelay_handleRelayMsg(struct SMTPSession *session) {
    //Diese procedure ist der Kontroll-Übergabe-Punkt an die
    //hierdrauf aufsetzende Software...

    //duplicator_handleRelayMsg(session);
    //AFunc(session);
    //    crypto_handleRelayMsg(session);
    /*
    if (session->event == hmsg_SendFromClientToServer) {
        session->data = (char*)"nö, das geht aber so\n";
        session->dataSize = strlen(session->data);
    }
    */

    //syslog(LOG_DEBUG, "smtprelay_handleRelayMsg: %s", session->data);
}


char* smtprelay_retCodeToStr(enum SMTPRetCode retCode) {
    switch (retCode) {
      case sr_Unknown: return("sr_Unknown");break;
      case sr_1      : return("sr_1");break;
      case sr_2      : return("sr_2");break;
      case sr_3      : return("sr_3");break;
      case sr_4      : return("sr_4");break;
      case sr_5      : return("sr_5");break;
    }
    return("sr_Unknown");
}

enum SMTPRetCode smtprelay_getRetCodeFromLine(char *line) {
    if (line[0] == '1') return sr_1;
    if (line[0] == '2') return sr_2;
    if (line[0] == '3') return sr_3;
    if (line[0] == '4') return sr_4;
    if (line[0] == '5') return sr_5;
    return sr_Unknown;
}

char* smtprelay_ltrim(char* line) {
  while (*line == ' ' || *line == '\t') { line++; }
  return line;
}

int smtprelay_readline(int fd, char *str, int maxlen) {
    int n;           /* no. of chars */
    size_t readcount;   /* no. characters read */
    char c;
    //if (!fd) {
    //    syslog(LOG_ERR, "dont try to read from closed socket: fd=%d", fd);
    //    return(-1);
    //}
    for (n = 1; n < maxlen; n++) {
        /* read 1 character at a time */
        readcount = read(fd, &c, 1); /* store result in readcount */
        if (readcount == 1) { /* 1 char read? */
            *str = c;      /* copy character to buffer */
            str++;         /* increment buffer index */
            if (c == '\n') /* is it a newline character? */
            break;      /* then exit for loop */
        } else {
            if (readcount == 0) {/* no character read? */
                if (n == 1) {    /* no character read? */
                    return (0); /* then return 0 */
                } else {
                    break;      /* else simply exit loop */
                }
            } else {
                return (-1); /* error in read() */
            }
        }
    }
    *str=0;       /* null-terminate the buffer */
    return (n);   /* return number of characters read */
} /* smtprelay_readline() */

enum SMTPState smtprelay_getSmtpFromLine(char* line) {
  if (strncasecmp(line, "HELO", 4)       == 0) { return (ss_Helo);     }
  if (strncasecmp(line, "EHLO", 4)       == 0) { return (ss_Helo);     }
  if (strncasecmp(line, "MAIL FROM", 9)  == 0) { return (ss_MailFrom); }
  if (strncasecmp(line, "RCPT TO", 7)    == 0) { return (ss_RcptTo);   }
  if (strncasecmp(line, "DATA", 4)       == 0) { return (ss_DataBegin);}
  if (strncasecmp(line, "RSET", 4)       == 0) { return (ss_Rset);     }
  if (strncasecmp(line, "QUIT", 4)       == 0) { return (ss_Quit);     }
  return ss_Unknown;
}

char* smtprelay_StateToStr(enum SMTPState smtpstate) {
  switch (smtpstate) {
    case ss_Unknown  :   return("ss_Unknown"); break;
    case ss_Inititalize: return("ss_Inititalize"); break;
    case ss_Connect  :   return("ss_Connect"); break;
    case ss_Helo     :   return("ss_Helo"); break;
    case ss_MailFrom :   return("ss_MailFrom"); break;
    case ss_RcptTo   :   return("ss_RcptTo"); break;
    case ss_DataBegin:   return("ss_DataBegin"); break;
    case ss_Data     :   return("ss_Data"); break;
    case ss_DataEnd  :   return("ss_DataEnd"); break;
    case ss_Rset     :   return("ss_Rset"); break;
    case ss_Quit     :   return("ss_Quit"); break;
    case ss_CleanUp  :   return("ss_CleanUp"); break;
    case ss_DisConn  :   return("ss_DisConn"); break;
    default          :   return("[invalid SMTPState]");
  }
}

void smtprelay_setState(enum SMTPState newState) {
  currentState = newState;
  //printf("New State: %d (%s)\n", currentState, smtprelay_StateToStr(currentState));
  syslog(LOG_DEBUG, "New State: %d (%s)\n", currentState, smtprelay_StateToStr(currentState));
}

int smtprelay_sendMsg(struct SMTPSession *session) {
    /* Send msg with length msgLen to tSock and return the number of
     * Bytes sent off.
     * Use this function to send data to the mailserver when in ss_Data-
     * state.                                                         */
    int nWrite = -1;
    session->event = hmsg_SendFromClientToServer;
    session->_sendDataOut = 1;
    smtprelay_handleRelayMsg(session);
    if (session->_sendDataOut == 0) {
        return(0);
    }
    if (session->dataSize > 0) {
        nWrite = write(*session->serverSock, session->data, session->dataSize);
        if (nWrite < 0) {
            syslog(LOG_ERR, "ERROR writing to socket in smtprelay_relayMsg()");
            perror("ERROR writing to socket");
            exit(1);
        }
    }
    session->dataSize = nWrite;
    return(nWrite);
}

enum SMTPRetCode smtprelay_relayMsg(struct SMTPSession *session) {
  /* Send msg to tSock and the reply to sSock
   * Do not use this procedure to relay something when
   * in DATA-state. There will be no answer...
   * sSock: Source Socket the Message comes from
   * tSock: Target socket the Message goes to
   * msg:   The Message
   * Returns: SMTPRetCode of the Reply-Message from tSock
   */

    //sSock = *session->clientSock;
    //tSock = *session->serverSock;
    //msg = session->data;
    //msgLen = session->dataSize;


    enum SMTPRetCode result;
    int nWrite, nRead;
    char buffer[BUFSIZE + 1];

    session->currReply = sr_Unknown;
    session->event = hmsg_RelayFromClientToServer;
    smtprelay_handleRelayMsg(session);

    //Write msg to Sock...
    //fprintf(stdout, "RELAY: sending: %s\n", currSession.data);
    if (session->dataSize > 0) {
        nWrite = write(*session->serverSock, session->data, session->dataSize);
        if (nWrite < 0) {
            syslog(LOG_ERR, "ERROR writing to socket in smtprelay_relayMsg()");
            perror("ERROR writing to socket");
            exit(1);
        }
    }
    //then wait for reply...
    //session data is pointer to the answer of the server that'll be
    //sent off to the client...
    session->data = &buffer[0];
    do {
        bzero(buffer, BUFSIZE + 1);
        session->dataSize = 0;
        nRead = smtprelay_readline(*session->serverSock, buffer, BUFSIZE);
        session->dataSize = nRead;
        if (nRead < 0) {
            syslog(LOG_ERR, "ERROR reading from socket in smtprelay_relayMsg()");
            perror("ERROR reading from socket");
            exit(1);
        }
        if (nRead == BUFSIZE) {
            if ((*buffer += BUFSIZE) != '\n') {
                syslog(LOG_ERR, "ERROR: line too long in smtprelay_relayMsg()");
                perror("ERROR: line too long");
                exit(1);
            }
        }
        //before sending answer to the client handle userdefined things...
        //maybe the answer will be changed... ;)
        session->currReply = smtprelay_getRetCodeFromLine(buffer);
        session->event = hmsg_RelayFromServerToClient;
        smtprelay_handleRelayMsg(session);
        //fprintf(stdout, "RELAY: received: %s\n", buffer);
        //write answer to sSock:
        if (session->sendDataToClientRequested) {
            nWrite = write(*session->clientSock, session->data, session->dataSize);
            if (nWrite < 0) {
                syslog(LOG_ERR, "ERROR writing answer to socket in smtprelay_relayMsg()");
                perror("ERROR writing to socket");
                exit(1);
            }
        } else {
            //fprintf(stdout, "RELAY: suppressing answer to client\n");
        }
    } while (buffer[3] == '-');
    //get the result...
    result = session->currReply;

    return(result);
}

char *smtprelay_cleanUpSMTPAdress(char *adr) {
  char *result, *tmp;
  int iBeg, iEnd, i;
  iBeg = 0; iEnd = 0; i = 0;
  tmp = adr;
  while (*tmp != '\0') {
    i++;
    if (*tmp == '<') {
      iBeg = i;
    }
    if (*tmp == '>') {
      iEnd = i;
    }
    if ((iBeg > 0) && (iEnd > 0)) {
      break;
    }
    tmp++;
  }

  if ((iEnd == 0) || (iEnd <= iBeg)) { //dann den ganzen string wieder zurueckgeben
    iBeg = 0;
    iEnd = i;
  }
  i= (iEnd - iBeg);
  result = (char*)malloc(sizeof(char) * i + 1);
  bzero(result, i);
  strncpy(result, (adr + iBeg), i - 1);
  return(result);
}

void smtprelay_cleanupLineBreak(char *str) {
  /* suche \r und \n und ersetze durch " " */
  char *p = str;
  while (*p != '\0') {
    if (*p == '\r') { *p = ' '; }
    if (*p == '\n') { *p = ' '; }
    p++;
  }
}

int smtprelay_doprocessing (int sock) {
    int nRead /*, nWrite */;
    char buffer[BUFSIZE + 1];
    enum SMTPState newState = ss_Inititalize;
    struct addrinfo hints, *servinfo, *p;
    int ms_sockfd; //socket to target mailserver
    int rv, yes=1;

    smtprelay_setState(newState);

    currSession.currState = &currentState;
    currSession.clientSock= &sock;
    currSession.data = (char*)malloc(sizeof(char)*BUFSIZE + 1);
    currSession.sendDataToClientRequested = 1;
    bzero(currSession.data, BUFSIZE + 1);

    newState = ss_Connect;
    smtprelay_setState(newState);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = NULL; // use my IP

    if ((rv = getaddrinfo(smtpConfigFile->fwd_addr, smtpConfigFile->fwd_port, &hints, &servinfo)) != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((ms_sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(ms_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("server: setsockopt");
            return(-1);
        }
        if (connect(ms_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(ms_sockfd);
            perror("server: connect");
            continue;
        }
        break;
    }

    if (p == NULL)  {
        syslog(LOG_ERR, "server: failed to connect to mailserver @host:%s; port:%s\n", smtpConfigFile->fwd_addr, smtpConfigFile->fwd_port);
        return -2;
    }
    /* OK, this is the point we're connected to our target mailserver
     * and now we save the serverSocket for later use                 */
    currSession.serverSock= &ms_sockfd;

    /* now let's begin the magic SMTP... :o) */
    //get initial greeting from server
    currSession.dataSize = -1;
    bzero(currSession.data, BUFSIZE + 1);
    smtprelay_relayMsg(&currSession);

    do {
        bzero(buffer, BUFSIZE + 1);
        nRead = smtprelay_readline(sock, buffer, BUFSIZE);
        if (nRead < 0) {
            syslog(LOG_ERR, "ERROR reading from socket in smtprelay_doprocessing()");
            perror("ERROR reading from socket");
            exit(1);
        }
        if (nRead == BUFSIZE) {
            if ((*buffer += BUFSIZE) != '\n') {
                syslog(LOG_ERR, "ERROR: line too long in smtprelay_doprocessing()");
                perror("ERROR: line too long");
                exit(1);
            }
        }
        /* Fehlerbehandlung ist abgeschlossen, wir schauen mal in die Daten...
         * Besonderheit ist die Uebertragung des DATA-Teils, dieser darf
         * nur mit einem "." abgeschlossen werden.
         * Sobald DATA gesendet wurde, gehen wir in den Status ss_DataBegin
         * damit wir spaeter im SWITCH-Abschnitt noch auf den Beginn des DATA
         * reagieren koennen und wechseln dort dann auch in den Status ss_Data.
         */
        if (currentState == ss_Data) { //wenn wir im Status ss_Data sind...
            if (strcmp(buffer, ".\r\n") == 0) { //...warten wir auf das Ende der DUe
                smtprelay_setState(ss_DataEnd); //und treten dann in den Status ss_DataEnd ein.
            }
        } else {
            newState = smtprelay_getSmtpFromLine(buffer);
            if (newState != ss_Unknown) {
                smtprelay_setState(newState);
            }
        }
        //initialize currSession values with message data:
        currSession.data = &buffer[0];
        currSession.dataSize = nRead;
        currSession.event = hmsg_Unknown;

        switch (currentState) {
            case ss_Helo:
                currSession.initMappingData = 1;
                smtprelay_relayMsg(&currSession);
                break;
            case ss_MailFrom:
                //printf("MAILFROM: wenn das akzeptiert wird, dann die configs durchgehen.\n");
                switch (smtprelay_relayMsg(&currSession)) {
                    case sr_2:
                        //fprintf(stdout,"...und wurde akzeptiert:%s\n", buffer);
                        if (currSession.initMappingData) {
                            findmatchingConfig(cfgType_NONE, NULL); //cfgType_NONE: Reset all items
                            currSession.initMappingData = 0;
                        }
                        findmatchingConfig(cfgType_From, &buffer[0]);
                        break;
                    default:
                        //printf("...und wurde leider nicht akzeptiert\n");
                    break;
                }
                break;
            case ss_RcptTo:
                //printf("RCPTTO: wenn das akzeptiert wird, dann die configs durchgehen.\n");
                switch (smtprelay_relayMsg(&currSession)) {
                    case sr_2:
                         /* dieser Punkt ist wunderbar geeignet um die LinkedList
                         * der auf From- & To- passenden mappings zu initialisieren */
                       //printf("...und wurde akzeptiert\n");
                        findmatchingConfig(cfgType_To, &buffer[0]);
                        break;
                    default:
                        //printf("...und wurde leider nicht akzeptiert\n");
                    break;
                }
                break;
            case ss_Rset:
                /* nun alle configs resetten, aber nur wenn der server das gleiche tut*/
                switch (smtprelay_relayMsg(&currSession)) {
                    case sr_2:
                       //tmpcfg = config;
                       //while (tmpcfg) {
                       //  tmpcfg->processThis = 0;
                       //  tmpcfg = tmpcfg->next;
                       //}
                       currSession.initMappingData = 1;
                    default:
                        break;
                }
                break;
            case ss_Data:
                /* Hier koennen wir nun den DATA-Teil abgreifen... */
                /* in unserem Fall reichen wir die Mail 1:1 durch... */
                smtprelay_sendMsg(&currSession);
                break;
            case ss_DataEnd:
                /* Nun senden wir den DATA-abschliessenden punkt selbst...
                 * und warten ganz normal auf die Antwort die wir dann durchreichen */
                syslog(LOG_INFO, " on ss_DataEnd --> processing data...\n");
                currSession.data = (char*)".\r\n";
                currSession.dataSize = 3;
                smtprelay_relayMsg(&currSession);
                syslog(LOG_INFO, " on ss_DataEnd --> processing data done.\n");
                break;
            case ss_DataBegin: /* DATA wurde eben gesendet. */
                /* jetzt muessen wir in den Status DATA gehen,
                 * weil wir fortan auf den "." warten... (s.o.)
                 * Doch so halt! Das machen wir nur, wenn der Mailserver
                 * von uns weitere Daten empfangen moechte. Das sagt er uns
                 * durch einen 3xx-reply
                 */
                injectRecipients();
                //fprintf(stdout, "++>sending: %s", currSession.data);
                switch (smtprelay_relayMsg(&currSession)) {
                    case sr_3:
                        /* check if we have to inject further recipients... */
                        smtprelay_setState(ss_Data);
                        break;
                    default:
                        break;
                }
                break;
            case ss_Unknown:
                //break; --> No break here, do default.
            default:
                /* wenn sonst nix passt, dann relayen wir die message */
                smtprelay_relayMsg(&currSession);
                break;
        }
    } while (currentState < ss_Quit);
    /* invoke smtprelay_relayMsg for possible cleanup */
    smtprelay_setState(ss_CleanUp);
    currSession.dataSize = -1;
    bzero(currSession.data, BUFSIZE+1);
    smtprelay_relayMsg(&currSession);

    close(ms_sockfd);
    return(currentState);
}

int injectRecipients() {
    llItem *itm = smtpConfigFile->mappings->first;
    smtp_mapEntry *e;
    char *orig = (char *)malloc(sizeof(char) * strlen(currSession.data)+1);
    memcpy(orig, currSession.data, strlen(currSession.data)+1);
    //fprintf(stdout, "orig=%s\n", orig);
    while (itm) {
        e = (smtp_mapEntry *)itm->data_ptr;
        //fprintf(stdout, "dump: %s: %d\n", e->me_Result, e->match);
        if ( ((e->match & cfgType_From) == cfgType_From) &&
             ((e->match & cfgType_To)   == cfgType_To) ) {
                sprintf(currSession.data, "RCPT TO: %s\r\n", e->me_Result);
                currSession.dataSize = strlen(currSession.data);
                //fprintf(stdout, " -->sending: %s", currSession.data);
                currSession.sendDataToClientRequested = 0;
                smtprelay_relayMsg(&currSession);
                currSession.sendDataToClientRequested = 1;
        }
        itm = itm->next;
    }
    memcpy(currSession.data, orig, strlen(orig)+1);
    currSession.dataSize = strlen(currSession.data);
    free(orig);
    return(0);
}


int findmatchingConfig(smtp_mappingType mapType, char *searchData) {
    llItem *itm = smtpConfigFile->mappings->first;
    smtp_mapEntry *e = NULL;
    char **r = NULL;

    while (itm) {
        e = (smtp_mapEntry*)itm->data_ptr;
        if (mapType == cfgType_NONE) {
            //Wenn cfgType_NONE, dann das itm einfach zurücksetzen
            e->match = cfgType_NONE;
        } else {
            switch (mapType) {
                case cfgType_From: r = &(e->me_From); break;
                case cfgType_To:   r = &(e->me_To);   break;
                default: r = NULL;
            }
            if (*r) {
                if (strstr(searchData, (*r)) || (**r == '*')) {
                    //Wenn gefunden, dann das itm markieren:
                    e->match |= mapType;
                    //fprintf(stdout, "gefunden: %s (in %s mit: %d)\n", (*r), e->me_Result, e->match);
                }
            }
        }
        itm = itm->next;
    }
    return 0;
}
