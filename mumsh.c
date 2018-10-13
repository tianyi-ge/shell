#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sh_func.h"

extern job_t jobs[MAX_JOBS];
extern int jobcnt;

int execute(char *line, int size) {
    job_t *job = &jobs[jobcnt];
    pipe_t *pip;
    int *pid = job->pid; // pointer to pid array
    int res = parse_pipe(line, &pip, size);
    int prev = 0;
    if (res != SU_FLAG) {
        int f = pip->cmds[0]->flag;
        erase_pipe(pip);
        if (res == EM_FLAG) {
            if (f) {
                miss_error();
                return ER_FLAG;
            }
            return EM_FLAG;
        }
        else if (res == ER_FLAG) {
            pipe_error(); return ER_FLAG;
        }
        else { //DP_FLAG
            return ER_FLAG;
        }
    }
    for (int i = 1; i < pip->size; ++i) {
        if (pip->cmds[i]->argv[0] == NULL && pip->cmds[i]->flag) {
            miss_error();
            erase_pipe(pip);
            return ER_FLAG;
        }
    }
    jobs[jobcnt].status = RUNNING;
    for (int i = 0; i < pip->size; ++i) {
        int fd[2];
        pipe(fd);
        int in = prev, out = fd[1];
        if (i == 0) in = -1;
        if (i == pip->size - 1) out = -1;

        //redirection
        if (pip->cmds[i]->flag & OUT_APPEND) {
            out = open(pip->cmds[i]->outfile, FLAGS_AP, MODE);
            if (out == -1) {
                printf("%s: Permission denied\n", pip->cmds[i]->outfile);
                erase_pipe(pip);
                return ER_FLAG;
            }
        }
        if (pip->cmds[i]->flag & OUT_REDIR) {
            out = open(pip->cmds[i]->outfile, FLAGS_WR, MODE);
            if (out == -1) {
                printf("%s: Permission denied\n", pip->cmds[i]->outfile);
                erase_pipe(pip);
                return ER_FLAG;
            }
        }
        if (pip->cmds[i]->flag & IN_REDIR) {
            in = open(pip->cmds[i]->infile, FLAGS_RD, MODE);
            if (in == -1) {
                printf("%s: No such file or directory\n", pip->cmds[i]->infile);
                erase_pipe(pip);
                return ER_FLAG;
            }
        }
        pid[i] = exec_cmd(pip->cmds[i], in, out);
        prev = fd[0];
    }
    job->pcnt = pip->size;
    if (pip->is_bg) print_job(jobcnt++); // valid background job. Otherwise it'll be overwitten next time.
    
    if (!pip->is_bg) {
        int status;
        for (int i = 0; i < pip->size; ++i) {
            if (pid[i] > 0)
                waitpid(pid[i], &status, 0);
        }
    }
    erase_pipe(pip);
    return SU_FLAG;
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
        strncpy(jobs[jobcnt].name, line, strlen(line)); // backup
        strncpy(s, line, strlen(line)); // ready for unfinished command
        sep_redir(line);
        while ((res = finish_check(line)) == WAIT_CMD) {
            printf("> ");
            fflush(stdout);
            if (fgets(line, MAX_LEN, stdin) == NULL) break;
            strcat(jobs[jobcnt].name, line); // keep the original string
            strcat(s, line); // append new line to backup string s
            sep_redir(s);  // seperate < and >
            strncpy(line, s, strlen(s)); // copy s to line
        }
        
        if (res == PIPE_ERROR) { // pipe error
            pipe_error();
            continue;
        }

        if (res == MISS_ERROR) {
            miss_error();
            continue;
        }
        
        if (res == REDIR_ERROR) continue;

        int flag = execute(line, res + 1);

        switch (flag) {
            case SU_FLAG: break;
            case EM_FLAG: break;
            case ER_FLAG: break;
            default: break;
        }
    }
    return 0;
}
