#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sh_func.h"

int main() {
    char line[MAX_LEN], s[MAX_LEN];
    int len;
    pipe_t *pip;
    while(1) {
        printf("mumsh $ ");
        fflush(stdout);

        if ((len = read(0, line, (size_t)MAX_LEN)) <= 1) continue;     //'\n' counts for 1 character, also ignored
        line[len-1] = '\0';
        strncpy(s, line, strlen(line)); // backup

        pip = parse_pipe(line);

        int (*fd)[2];
        if(pip->size > 1) {
            fd = (int (*)[2])malloc(sizeof(int) * 2 * (pip->size - 1));
            pip->cmds[0]->fd[0] = -1;
            pip->cmds[pip->size-1]->fd[1] = -1; //The first read and the last write

            for (int i = 0; i < pip->size-1; ++i) {
                pipe(fd[i]);
                pip->cmds[i+1]->fd[0] = fd[i][0];
                pip->cmds[i]->fd[1] = fd[i][1];
            }

            for (int i = 0; i < pip->size - 1; ++i)
                close(fd[i][0]), close(fd[i][1]);

            for (int i = 0; i < pip->size - 1; ++i)
                printf("%d ==> %d\n", fd[i][1],fd[i][0]);
        }

        for (int i = 0; i < pip->size; ++i)
            exec_cmd(pip->cmds[i]);

        if (pip->size > 1) free(fd);
        erase_pipe(pip);
    }
    return 0;
}
