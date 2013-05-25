#ifndef _duplicator_h
#define _duplicator_h

#include "smtprelay.h"

/* prototypes */
int duplicator_OpenServerSocket();
int duplicator_CloseServerSocket();

int duplicator_handleRelayMsg(struct SMTPSession *session);


#endif //_duplicator_h
