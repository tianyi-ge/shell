#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sh_func.h"

int main() {
    char line[MAX_LEN];
    int len;
    pipe_t *pipeline;
    int n = 0;
    while(1) {
        printf("mumsh $ ");
        fflush(stdout);

        if ((len = read(0, line, (size_t)MAX_LEN)) <= 1) continue;     //'\n' counts for 1 character, also ignored
        line[len-1] = '\0';
        pipeline = parse_pipe(line);


        int (*fd)[2] = (int (*)[2])malloc(sizeof(int) * 2 * (pipeline->size - 1));
        pipeline->cmds[0]->fd[0] = 0;
        pipeline->cmds[pipeline->size -1]->fd[1] = 1; //The first read and the last write
        for (int i = 0; i < pipeline->size-1; ++i) {
            pipe(fd[i]);
            // A pipe with a pair of fd, respectively in and out
            //(in)====(out), (in) for the former (child) command, (out) for the latter (parent command)
            // if error, return -1
            pipeline->cmds[i+1]->fd[0] = fd[i][0];
            pipeline->cmds[i]->fd[1] = fd[i][1];
        }

        for (int i = 0; i < pipeline->size - 1; ++i)
            close(fd[i][0]), close(fd[i][1]);

        for (int i = 0; i < pipeline->size; ++i)
            exec_cmd(pipeline->cmds[i]);
        
        for (int i = 0; i < pipeline->size - 1; ++i)
            close(fd[i][0]), close(fd[i][1]);
        
        free(fd);
    }
    return 0;
}
