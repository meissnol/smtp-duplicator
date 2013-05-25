#ifndef _smtprelay_h
#define _smtprelay_h

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

enum SMTPState { ss_Unknown,
                 ss_Inititalize,
                 ss_Connect,
                 ss_Helo,
                 ss_MailFrom,
                 ss_RcptTo,
                 ss_DataBegin,
                 ss_Data,
                 ss_DataEnd,
                 ss_Rset,
                 ss_Quit,
                 ss_CleanUp,
                 ss_DisConn
                };

enum SMTPRetCode { sr_Unknown,
                   sr_1,
                   sr_2,
                   sr_3,
                   sr_4,
                   sr_5
                 };

enum stmpHandleMsg_Event {
                            hmsg_Unknown,
                            hmsg_SendFromClientToServer,
                            hmsg_RelayFromClientToServer,
                            hmsg_RelayFromServerToClient
                         };

struct SMTPSession {
    int                     *clientSock;
    int                     *serverSock;
    enum SMTPState          *currState;
    enum SMTPRetCode        currReply;
    char                    *data;
    int                     dataSize;
    struct sockaddr_storage *client_addr;
    enum stmpHandleMsg_Event event;
    int                     _sendDataOut;
};

/* globals */
enum SMTPState     currentState; //stores current smtp state
struct SMTPSession currSession;  //stores important session data


/* functions */

int   smtprelay_ClientConnect(int cliSock, struct sockaddr_storage *client_addr);
void  smtprelay_handleRelayMsg(struct SMTPSession *session);

int   smtprelay_readline(int fd, char *str, int maxlen);
int   smtprelay_doprocessing (int sock);
int   smtprelay_sendMsg(struct SMTPSession *session);

char* smtprelay_retCodeToStr(enum SMTPRetCode retCode);
char* smtprelay_ltrim(char* line);
char* smtprelay_StateToStr(enum SMTPState smtpstate);
char* smtprelay_cleanUpSMTPAdress(char *adr);

void  smtprelay_setState(enum SMTPState newState);
void  smtprelay_cleanupLineBreak(char *str);

enum SMTPState   smtprelay_getSmtpFromLine(char* line);
enum SMTPRetCode smtprelay_getRetCodeFromLine(char *line);
enum SMTPRetCode smtprelay_relayMsg(struct SMTPSession *session);

#endif /* _smtprelay_h */
