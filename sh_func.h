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

typedef struct cmd_t {
    int flag;
    char *infile;
    char *outfile;
    char *argv[MAX_LEN];
} cmd_t;

typedef struct pipe_t{
    int size;
    cmd_t **cmds;
} pipe_t;

cmd_t *parse_cmd(char *);
pipe_t *parse_pipe(char *);
int get_pipesize(char *);
int exec_cmd(cmd_t *, int, int);
int builtin_cmd(cmd_t *);
void erase_pipe(pipe_t *);

#endif
