#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sh_func.h"

#define ERROR_CMD -1
#define EMPTY_CMD 0
#define SUCCESS_CMD 1

int execute(char *line) {
    pipe_t *pip;
    int id[MAX_LEN>>2];
    //int res = parse_pipe(line, &pip);
    parse_pipe(line, &pip);
    int prev = 0;
    if (pip->emptyFLAG) {
        if (pip->size == 1) {
            erase_pipe(pip);
            return EMPTY_CMD;
        }
        /* should not happen
        else {
            if (pip->emptyFLAG == ER_FLAG) {
                erase_pipe(pip);
                return ERROR_CMD;
            }
            else {

            }
        }
        */
    }
    for (int i = 0; i < pip->size; ++i) {
        int fd[2];
        pipe(fd);
        int in = prev, out = fd[1];
        if (i == 0) in = -1;
        if (i == pip->size - 1) out = -1;

        //redirection
        if (pip->cmds[i]->flag & OUT_APPEND) out = open(pip->cmds[i]->outfile, FLAGS_AP, MODE);
        if (pip->cmds[i]->flag & OUT_REDIR) out = open(pip->cmds[i]->outfile, FLAGS_WR, MODE);
        if (pip->cmds[i]->flag & IN_REDIR) in = open(pip->cmds[i]->infile, FLAGS_RD, MODE);
        id[i] = exec_cmd(pip->cmds[i], in, out);
        prev = fd[0];
    }
    int status;
    for (int i = 0; i < pip->size; ++i)
        if (id[i] > 0)
            waitpid(id[i], &status, 0);
    
    erase_pipe(pip);
    return SUCCESS_CMD;
}

int main() {
    char line[MAX_LEN], s[MAX_LEN];
    int res;
    while (1) {
        memset(line, 0, sizeof(line));
        memset(s, 0, sizeof(s));
        shell_prompt();
        signal(SIGINT, sig_handler);
        if (fgets(line, MAX_LEN, stdin) == NULL) {
            if (feof(stdin)) terminate();
            else continue;
        }
        strncpy(s, line, strlen(line)); // backup
        sep_redir(line);
        while ((res = not_finished(line)) == 1) {
            printf("> ");
            fflush(stdout);
            if (fgets(line, MAX_LEN, stdin) == NULL) break;
            
            strcat(s, line); // append new line to backup string s
            sep_redir(s);  // seperate < and >
            strncpy(line, s, strlen(s)); // copy s to line
        }
        if (res == -1) { // wait for file error
            printf("mumsh: syntax error near unexpected token `|'\n");
            continue;
        }
        
        int flag = execute(line);
        switch (flag) {
            case EMPTY_CMD: continue;
            case ERROR_CMD: continue;
            case SUCCESS_CMD: continue;
            default: continue;
        }
    }
    return 0;
}
