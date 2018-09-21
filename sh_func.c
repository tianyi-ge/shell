#include "sh_func.h"

cmd_t *parse_cmd(char *line) {
    //redir
    cmd_t *cmd = (cmd_t *)malloc(sizeof(cmd_t) + MAX_LEN * sizeof(char*));
    memset(cmd, 0, sizeof(cmd));

    char *token = NULL, *s = strdup(line);
    int cnt = 0;

    while (token = strsep(&s, " \t\r\n")) {
        if (token[0] != '\0')
            cmd->argv[cnt++] = token;
    }
    cmd->argv[cnt] = NULL;

    free(s);
    return cmd;
}

pipe_t *parse_pipe(char *line) {
    int size = get_pipesize(line) + 1;
    pipe_t *pipe = (pipe_t *)malloc(sizeof(pipe_t) + size * sizeof(cmd_t *));
    memset(pipe, 0, sizeof(pipe));

    char *token = NULL, *s = strdup(line);
    int cnt = 0;

    while (token = strsep(&s, "|"))
        if (token[0] != '\0')
            pipe->cmds[cnt++] = parse_cmd(token);
    pipe->cmds[cnt] = NULL;
    pipe->size = size;

    free(s);
    return pipe;
}

int get_pipesize(char *line) {
    char *tmp = line;
    int size = 0;
    while (*tmp != '\0') if (*(tmp++) == '|') size++;
    return size;
}

int exec_cmd(cmd_t *cmd) {
    if (builtin_cmd(cmd)) return 1;
    pid_t pid = fork();

    switch(pid) {
        case -1: printf("Fork error"); return -1;
        case  0: { //child
            // pipe for the current process
            // if none, stdin or stdout
            dup2(cmd->fd[0], STDIN_FILENO);
            dup2(cmd->fd[1], STDOUT_FILENO);
            // implement the redirection
            printf("I'm child, my id is (%d)\n",getpid());
            printf("[%s]\n",cmd->argv[0]);
            if (execvp(cmd->argv[0], cmd->argv) < 0)
                printf("%s: command not found\n", cmd->argv[0]);
            
            close(cmd->fd[0]);
            close(cmd->fd[1]);
            break;
        }
        default:  {
            //close(cmd->fd[0]);
            //close(cmd->fd[1]);
            printf("I'm father, my id is (%d)\n",getpid());
            wait(NULL);
            break; // parent
        }
    }

    //waitpid(pid,NULL,0);
    return 0;
}

int builtin_cmd(cmd_t *cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit(0);
        return 1;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char buf[MAX_DIR_LEN];
        printf("%s\n", getcwd(buf, sizeof(buf)));
        return 1;
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argv[2]) {
            printf("mumsh: cd: too many arguments\n");
            return 1;
        }
        if (chdir(cmd->argv[1]) == -1) printf("mumsh: cd: %s: No such file or directory\n", cmd->argv[1]);
        return 1;
    }
    return 0;
}