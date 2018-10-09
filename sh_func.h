#ifndef _SH_FUNC_H_
#define _SH_FUNC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LEN 1024
#define MAX_DIR_LEN 256

#define IN_REDIR 1
#define OUT_REDIR 2
#define OUT_APPEND 4
#define FLAGS_WR O_WRONLY | O_CREAT | O_TRUNC
#define FLAGS_AP O_WRONLY | O_APPEND | O_CREAT
#define FLAGS_RD O_RDONLY | O_CREAT
#define MODE 0666

#define SU_FLAG 0
#define ER_FLAG 1
#define WT_FLAG 2 

#define IN_FILE 1
#define OUT_FILE 2

typedef struct cmd_t {
    int flag;
    int is_bg;
    char *infile;
    char *outfile;
    char *argv[MAX_LEN];
} cmd_t;

typedef struct pipe_t{
    int size;
    int emptyFLAG;
    cmd_t **cmds;
} pipe_t;

int  not_finished(char *);
void sep_redir(char *);
void shell_prompt();
void terminate();
int  parse_cmd(char *, cmd_t **);
int  parse_pipe(char *, pipe_t **);
int  get_pipesize(char *);
int  exec_cmd(cmd_t *, int, int);
int  builtin_cmd(cmd_t *);
void erase_pipe(pipe_t *);
void sig_handler(int);
void psig_handler(int);

#endif
