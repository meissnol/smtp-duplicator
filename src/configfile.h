#ifndef _configfile_h
#define _configfile_h

#include "linkedlist.h"

enum t_configfileState {
      cfs_Unknown = 0,
      cfs_listen_addr = 1,
      cfs_listen_port = 2,
      cfs_fwd_addr = 3,
      cfs_fwd_port = 4,
      cfs_mapfile = 5,
};
typedef enum t_configfileState smtp_configfileState;

enum t_mappingType {
      cfgType_NONE = 0,
      cfgType_From = 1,
      cfgType_To   = 2,
      cfgType_Result = 3
};
typedef enum t_mappingType smtp_mappingType;

struct t_mappingEntry {
    char* me_From;
    char* me_To;
    char* me_Result;
    smtp_mappingType match;
};
typedef struct t_mappingEntry smtp_mapEntry;

struct t_configfile {
 char* listen_addr;
 char* listen_port;
 char* fwd_addr;
 char* fwd_port;
 char* mapfile;
 llList *mappings;
};
typedef struct t_configfile smtp_configfile;

/*globals */
smtp_configfile *smtpConfigFile;


void init_configfile(smtp_configfile* cfg);
enum t_configfileState textTo_smtp_configfileState(char *txt);
void dump_configfile(smtp_configfile* cfg);
void free_configfile(smtp_configfile* cfg);
smtp_configfile* parse_configfile(char *filename);
int parse_config_mapfile(smtp_configfile* config);
void chomp(char *str);

#endif //define _configfile_h
