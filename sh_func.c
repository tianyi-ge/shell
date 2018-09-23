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
    cmd_t *cmd = (cmd_t *)calloc(1, sizeof(cmd_t));
    char *token = NULL;
    int cnt = 0;
    for (token = strtok(s, " \t\r\n"); token != NULL; token = strtok(NULL, " \t\r\n")) {
        if (token[0] != '\0')
            cmd->argv[cnt++] = token;
    }

    int book = 0;

    for (int i = 0; i < cnt; ++i) {
        if (strcmp(cmd->argv[i], ">>") == 0) {
            cmd->flag |= OUT_APPEND;
            if (cmd->argv[i+1] != NULL)
                cmd->outfile = cmd->argv[i+1];
            if (book == 0) book = 1;            
        }
        else if (strcmp(cmd->argv[i], ">") == 0) {
            cmd->flag |= OUT_REDIR;
            if (cmd->argv[i+1] != NULL)
                cmd->outfile = cmd->argv[i+1];
            if (book == 0) book = 1;            
        }
        else if (strcmp(cmd->argv[i], "<") == 0) {
            cmd->flag |= IN_REDIR;
            if (cmd->argv[i+1] != NULL)
                cmd->infile = cmd->argv[i+1];
            if (book == 0) book = 1;
        }
        if (book == 1) cmd->argv[i] = NULL; // not necessary
    }
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

int exec_cmd(cmd_t *cmd, int in, int out) {
    pid_t pid = fork();
    if (pid == -1) {
        printf("Fork error");
        return -1;
    }
    else if (pid == 0) { // child
        // pipe for the current process
        // if none, stdin or stdout
        
        if (in != -1) dup2(in, STDIN_FILENO);
        close(in);
        if (out != 1) dup2(out, STDOUT_FILENO);
        close(out);

        if(builtin_cmd(cmd)) return 0;

        if (execvp(cmd->argv[0], cmd->argv) < 0)
            printf("%s: command not found\n", cmd->argv[0]);
    }
    else {
        close(in);
        close(out);
        int status;
        waitpid(-1, &status, 0);
    }
    return 0;
}

int builtin_cmd(cmd_t *cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit(0);
        return 1;
    }
    /*
    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char buf[MAX_DIR_LEN];
        printf("%s\n", getcwd(buf, sizeof(buf)));
        return 1;
    }
    */

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
    free(pip->cmds);
    free(pip);
}
