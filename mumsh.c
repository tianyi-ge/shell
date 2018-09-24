#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sh_func.h"

int main() {
    char line[MAX_LEN], s[MAX_LEN];
    pipe_t *pip;
    while(1) {
        printf("mumsh $ ");
        fflush(stdout);

        if (fgets(line, MAX_LEN, stdin) == NULL) continue;
        sep_redir(line);
        strncpy(s, line, strlen(line)); // backup
        pip = parse_pipe(line);

        int prev = 0;

        if (pip->emptyFLAG) {
            if (pip->size == 1) {
                erase_pipe(pip);
                continue;
            }
            /*
            else {
                if (pip->emptyFLAG == ER_FLAG) {//error info}
                else {//wait}
            }
            */
        }

        for (int i = 0; i < pip->size; ++i) {
            int fd[2];
            pipe(fd);
            int in = prev, out = fd[1];
            if (i == 0) in = -1;
            if (i == pip->size - 1) out = -1;
            if (pip->cmds[i]->flag & OUT_APPEND) out = open(pip->cmds[i]->outfile, FLAGS_AP, MODE);
            if (pip->cmds[i]->flag & OUT_REDIR) out = open(pip->cmds[i]->outfile, FLAGS_WR, MODE);
            if (pip->cmds[i]->flag & IN_REDIR) in = open(pip->cmds[i]->infile, FLAGS_RD, MODE);
            exec_cmd(pip->cmds[i], in, out);
            prev = fd[0];
        }
        erase_pipe(pip);
    }
    return 0;
}
