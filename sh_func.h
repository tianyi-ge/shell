#ifndef _SH_FUNC_H_
#define _SH_FUNC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LEN 1024
#define MAX_DIR_LEN 256

typedef struct cmd_t {
    int fd[2];
    char *argv[MAX_LEN];
} cmd_t;

typedef struct pipe_t{
    int size;
    cmd_t **cmds;
} pipe_t;

cmd_t *parse_cmd(char *);
pipe_t *parse_pipe(char *);
int get_pipesize(char *);
int exec_cmd(cmd_t *);
int builtin_cmd(cmd_t *);
void erase_pipe(pipe_t *);

#endif
