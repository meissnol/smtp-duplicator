#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <syslog.h>
#include <string.h>

#include "configfile.h"
#include "linkedlist.h"
#include "quote.h"

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
  if (strcasecmp(txt, "  ")           ==0)   { result = cfs_Unknown; }
  return(result);
}

void dump_configfile(smtp_configfile* cfg) {
  //printout result:
  syslog(LOG_INFO, "listen_addr: %s\n", cfg->listen_addr);
  syslog(LOG_INFO, "listen_port: %s\n", cfg->listen_port);
  syslog(LOG_INFO, "fwd_addr:    %s\n", cfg->fwd_addr);
  syslog(LOG_INFO, "fwd_port:    %s\n", cfg->fwd_port);
  syslog(LOG_INFO, "mapfile:     %s\n", cfg->mapfile);
  if (cfg->mappings) {
      llItem * itm = cfg->mappings->first;
      smtp_mapEntry* tmp = (smtp_mapEntry*)malloc(sizeof(smtp_mapEntry));
      int i = 0;
      while (itm) {
          llPop(cfg->mappings, itm, tmp);
          syslog(LOG_INFO, "mapping(%d): from='%s', to='%s', result='%s'\n",
                           i++, tmp->me_From, tmp->me_To, tmp->me_Result);
          itm = itm->next;
      }
      free(tmp);
  }
}

void free_configfile(smtp_configfile* cfg) {
  free(cfg->listen_addr);
  free(cfg->listen_port);
  free(cfg->fwd_addr);
  free(cfg->fwd_port);
  free(cfg->mapfile);
  llFree(&cfg->mappings);
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
  if (result->mapfile) {
      parse_config_mapfile(result);
  }
  return(result);
}

int parse_config_mapfile(smtp_configfile* config) {
  int result = -1;
  if ((!config) || (!config->mapfile)) {
      syslog(LOG_ERR, "no mapfile configured.");
      exit(1);
  }
  FILE *cFile;
  int nRet;
  size_t *t = malloc(0);
  char **gptr = malloc(sizeof(char*));
  char **words = NULL;
  uint i = 0;
  *gptr = NULL;
  char del[] = "\t\n \0";

  llInit(&config->mappings, sizeof(smtp_mapEntry));

  cFile = fopen(config->mapfile, "r");
  if (!cFile) {
    syslog(LOG_ERR, "error open mapfile %s", config->mapfile);
    exit(1);
  }
  rewind(cFile);

  while( (nRet=getline(gptr, t, cFile)) > 0) {
      //fputs(*gptr,stdout);
      chomp(*gptr);
      int wrdcnt = splitQuotedStr(*gptr, del, (1 << FL_DLMFLD), -1, &words);
      if (wrdcnt == 3) {
        /* prepare the new llEntry */
        smtp_mapEntry* tmp = (smtp_mapEntry*)malloc(sizeof(smtp_mapEntry));
        tmp->me_From = (char*)malloc(sizeof(char)*strlen(words[0]));
        strcpy(tmp->me_From, words[0]);
        tmp->me_To  = (char*)malloc(sizeof(char)*strlen(words[1]));
        strcpy(tmp->me_To, words[1]);
        tmp->me_Result = (char*)malloc(sizeof(char)*strlen(words[2]));
        strcpy(tmp->me_Result, words[2]);
        tmp->match = cfgType_NONE;
        llPush(config->mappings, tmp);
        free(tmp);
      } else {
           syslog(LOG_WARNING, "WARNING: found malformed line in mapfile: %s\n", *gptr);
      }
      for (i = 0; i < wrdcnt; i++)
        if (words[i]) free(words[i]);
  }
  free(t);
  free(words);
  fclose(cFile);
  return result;
}

void chomp(char *str) {
   size_t p=strlen(str);
   /* '\n' mit '\0' überschreiben */
   str[p-1]='\0';
}

