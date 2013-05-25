#ifndef _configfile_h
#define _configfile_h

enum t_configfileState {
      cfs_Unknown = 0,
      cfs_listen_addr = 1,
      cfs_listen_port = 2,
      cfs_fwd_addr = 3,
      cfs_fwd_port = 4,
      cfs_mapfile = 5
};
typedef enum t_configfileState smtp_configfileState;

struct t_configfile {
 char* listen_addr;
 char* listen_port;
 char* fwd_addr;
 char* fwd_port;
 char* mapfile;
};
typedef struct t_configfile smtp_configfile;

/*globals */
smtp_configfile *smtpConfigFile;


void init_configfile(smtp_configfile* cfg);
smtp_configfile* parse_configfile(char *filename);
enum t_configfileState textTo_smtp_configfileState(char *txt);
void dump_configfile(smtp_configfile* cfg);
void free_configfile(smtp_configfile* cfg);

#endif //define _configfile_h
