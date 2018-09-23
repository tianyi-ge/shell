#include "sh_func.h"
#include <string.h>
#include <stdio.h>

char *my_strdup(const char *src) { // remember to free s
    if (src == NULL) return NULL;
    char *s = malloc(strlen(src));
    while((*(s++) = *(src++)) != '\0');
    return s;
}

cmd_t *parse_cmd(char *s) {
    //redir
    cmd_t *cmd = (cmd_t *)calloc(1, sizeof(cmd_t));
    char *token = NULL;
    int cnt = 0;
    for (token = strtok(s, " \t\r\n"); token != NULL; token = strtok(NULL, " \t\r\n")) {
        if (token[0] != '\0')
            cmd->argv[cnt++] = token;
    }
    cmd->argv[cnt] = NULL;

    int i = 0;
    while (cmd->argv[i]) printf("%s ", cmd->argv[i++]);
    return cmd;
}

pipe_t *parse_pipe(char *s) {
    int size = get_pipesize(s) + 1;
    pipe_t *pip = (pipe_t *)calloc(1, sizeof(pipe_t));
    pip->cmds = (cmd_t **)calloc(size+1, sizeof(cmd_t *));

    char *token = NULL;
    char *words[MAX_LEN];
    int cnt = 0;
    for (token = strtok(s, "|"); token != NULL; token = strtok(NULL, "|"))
        if (token[0] != '\0')
            words[cnt++] = token;
    
    for (int i = 0; i < size; ++i) pip->cmds[i] = parse_cmd(words[i]);

    pip->cmds[size] = NULL;
    pip->size = size;

    return pip;
}

int get_pipesize(char *line) {
    char *tmp = line;
    int size = 0;
    while (*tmp != '\0') if (*(tmp++) == '|') size++;
    return size;
}

int exec_cmd(cmd_t *cmd) {
    if (builtin_cmd(cmd)) return 1;
    printf("[%s] start---------\n", cmd->argv[0]);
    pid_t pid = fork();
    if (pid == -1) {
        printf("Fork error");
        return -1;
    }
    else if (pid == 0) { // child
        // pipe for the current process
        // if none, stdin or stdout
        if (cmd->fd[0] != -1) dup2(cmd->fd[0], STDIN_FILENO), close(cmd->fd[0]);
        if (cmd->fd[1] != -1) dup2(cmd->fd[1], STDOUT_FILENO), close(cmd->fd[1]);
        printf("[%s] read from: (%d), write to (%d)\n", cmd->argv[0], cmd->fd[0], cmd->fd[1]);
        // implement the redirection
        printf("I'm child [%s], my id is (%d)\n",cmd->argv[0],getpid());
        if (execvp(cmd->argv[0], cmd->argv) < 0)
            printf("%s: command not found\n", cmd->argv[0]);
    }
    else {
        printf("I'm father [%s], my id is (%d)\n",cmd->argv[0], getpid());
        int status;
        waitpid(-1, &status, 0);
    }
    
    printf("[%s] end************\n", cmd->argv[0]);
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

void erase_pipe(pipe_t *pip) {
    for (int i = 0; i < pip->size; ++i)
        free(pip->cmds[i]);
    free(pip);
}
