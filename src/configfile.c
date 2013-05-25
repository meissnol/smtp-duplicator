#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include "configfile.h"

#define BUFSIZE 1024
#define PORT "3490"    // the port users will be connecting to
#define ADDR "::"      // the addr users will be connecting to
#define FPORT "25"     // the port server will be connected

smtp_configfileState textTo_smtp_configfileState(char *txt) {
  smtp_configfileState result = cfs_Unknown;
  if (strcasecmp(txt, "LISTEN_PORT")  ==0)   { result = cfs_listen_port; }
  if (strcasecmp(txt, "LISTEN_ADDR")  ==0)   { result = cfs_listen_addr; }
  if (strcasecmp(txt, "FORWARD_PORT") ==0)   { result = cfs_fwd_port; }
  if (strcasecmp(txt, "FORWARD_ADDR") ==0)   { result = cfs_fwd_addr; }
  if (strcasecmp(txt, "MAPFILE")      ==0)   { result = cfs_mapfile; }
  if (strcasecmp(txt, "  ")   ==0)   { result = cfs_Unknown; }
  return(result);
}

void dump_configfile(smtp_configfile* cfg) {
  //printout result:
  syslog(LOG_INFO, "listen_addr: %s\n", cfg->listen_addr);
  syslog(LOG_INFO, "listen_port: %s\n", cfg->listen_port);
  syslog(LOG_INFO, "fwd_addr:    %s\n", cfg->fwd_addr);
  syslog(LOG_INFO, "fwd_port:    %s\n", cfg->fwd_port);
  syslog(LOG_INFO, "mapfile:     %s\n", cfg->mapfile);
}

void free_configfile(smtp_configfile* cfg) {
  free(cfg->listen_addr);
  free(cfg->listen_port);
  free(cfg->fwd_addr);
  free(cfg->fwd_port);
  free(cfg->mapfile);
  free(cfg);
  cfg=NULL;
}

void init_configfile(smtp_configfile* cfg) {
  bzero(cfg, sizeof(smtp_configfile));

  cfg->listen_port = (char*)malloc(sizeof(char)* strlen(PORT)+1);
  strcpy(cfg->listen_port, PORT);

  cfg->fwd_port = (char*)malloc(sizeof(char)* strlen(FPORT)+1);
  strcpy(cfg->fwd_port, FPORT);

  cfg->listen_addr = (char*)malloc(sizeof(char)* strlen(ADDR)+1);
  strcpy(cfg->listen_addr, ADDR);
}

smtp_configfile* parse_configfile(char *filename) {
  FILE *cFile;
  char *buffer;
  char *p, **r, *o;
  smtp_configfile *result;
  smtp_configfileState currState;

  result = (smtp_configfile *)malloc(sizeof (smtp_configfile));
  init_configfile(result);
  buffer = (char *)malloc(sizeof(char) * BUFSIZE +1);

  cFile = fopen(filename, "r");
  if (!cFile) {
    syslog(LOG_ERR, "error open file %s", filename);
    exit(1);
  }
  rewind(cFile);

  while (!feof(cFile)) {
    currState = cfs_Unknown;
    bzero(buffer, BUFSIZE +1);
    fgets(buffer, BUFSIZE, cFile);
    for (p=buffer; *p!='\0'; p++) {
      switch (*p) {
        case '\0': break;
        case '\n': *p='\0'; break;
        case '#':  *p='\0'; break;
      }
      if (!*p) break;
    }
    p = strtok(buffer, " =");
    if (p) {
      r=NULL;
      currState = textTo_smtp_configfileState(p);
      o = p;
      //fprintf(stderr, "currState = %i (%s)\n",currState, p);
      p = strtok(NULL, " =");
       if (p) {
        switch (currState) {
          case cfs_listen_addr:
            r=&(result->listen_addr);
            break;
          case cfs_listen_port:
            r=&(result->listen_port);
            break;
          case cfs_fwd_addr:
            r=&(result->fwd_addr);
            break;
          case cfs_fwd_port:
            r=&(result->fwd_port);
            break;
          case cfs_mapfile:
            r=&(result->mapfile);
            break;
          case cfs_Unknown:
            r=NULL;
            syslog(LOG_INFO, "ignore unknown setting: \"%s\" in file \"%s\"\n", o, filename);
            break;
        }
        if (r) { //we need to update
          if (*r) { //but first clean up if still allocated
            //printf("clean up: %s\n", *r);
            free(*r);
          }
          *r = (char*)malloc((sizeof(char)*strlen(p))+1); //(re)allocate mem
          strcpy(*r, p); //copy setting...
        }
       }
    }
  } //while !feof()
  fclose(cFile);
  if (!result->listen_port) {
    result->listen_port = (char*)malloc(sizeof(char)*strlen(PORT)+1);
    strcpy(result->listen_port, PORT);
  }
  return(result);
}
