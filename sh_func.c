#include "sh_func.h"
#include <string.h>
#include <stdio.h>

void shell_prompt() {
    printf("mumsh $ ");
    fflush(stdout);
}

void terminate() {
    printf("exit\n");
    exit(0);
}

void sep_redir(char *s) {
    char backup[MAX_LEN];
    int i = 0, j = 0, flag_single = 0, flag_double = 0;
    while(s[i] != '\0') {
        if (flag_single || flag_double) {backup[j++] = s[i++]; continue;} // unfinished quotes
        if (s[i] == '>' && s[i+1] == '>') {
            backup[j++] = ' ';
            backup[j++] = '>';
            backup[j++] = '>';
            backup[j++] = ' ';
            i += 2;
        }
        else if (s[i] == '>' || s[i] == '<') {
            backup[j++] = ' ';
            backup[j++] = s[i++];
            backup[j++] = ' ';
        }
        else {
            if (s[i] == '\'') flag_single ^= 1;
            if (s[i] == '\"') flag_double ^= 1;
            backup[j++] = s[i++];
        }
    }
    for (int k = 0; k < j; ++k) s[k] = backup[k];
    s[j] = '\0';
}

int parse_cmd(char *s, cmd_t **cmd) {
    *cmd = (cmd_t *)calloc(1, sizeof(cmd_t));
    int cnt = 0, waitForFile = 0;
    char *next;
    char *send = s + strlen(s);
    while (s < send) {
        s += strspn(s, " \t\r\n");
        if (s >= send) break;

        if (*s == '>' && *(s+1) == '>') { //safe to check s+1 because of '\0'
            (*cmd)->flag |= OUT_APPEND;
            waitForFile = OUT_FILE;
            s += 2;
            continue;
        }
        else if (*s == '>') { 
            (*cmd)->flag |= OUT_REDIR;
            waitForFile = OUT_FILE;
            s++;
            continue;
        }
        else if (*s == '<') {
            (*cmd)->flag |= IN_REDIR;
            waitForFile = IN_FILE;
            s++;
            continue;
        }
        else if (*s == '\'' || *s == '\"') {
            s++;
            next = strchr(s, *(s-1));
        }
        else {
            next = s + strcspn(s, " \t\r\n");
        }

        if (next == NULL) { 
            //wait for parenthese input
        }
        *next = '\0';
        if (waitForFile == IN_FILE) {
            (*cmd)->infile = s;
            waitForFile = 0;
        }
        else if (waitForFile == OUT_FILE) {
            (*cmd)->outfile = s;
            waitForFile = 0;
        }
        else {
            (*cmd)->argv[cnt++] = s;
        }
        s = next + 1;
    }
    if (waitForFile) {
        //no file, error 
    }
    (*cmd)->argv[cnt] = NULL;
    if (cnt > 0 && (*cmd)->argv[cnt-1][0] == '&') {
        (*cmd)->is_bg = 1;
        (*cmd)->argv[--cnt] = NULL;
    }

    return SU_FLAG;
}

int parse_pipe(char *s, pipe_t **pip) {
    int size = get_pipesize(s) + 1;
    *pip = (pipe_t *)calloc(1, sizeof(pipe_t));
    (*pip)->cmds = (cmd_t **)calloc(size+1, sizeof(cmd_t *));

    char *token = NULL;
    char *words[MAX_LEN];
    int cnt = 0;
    for (token = strtok(s, "|"); token != NULL; token = strtok(NULL, "|"))
        if (token[0] != '\0')
            words[cnt++] = token;
    
    for (int i = 0; i < size; ++i){
        //int res = parse_cmd(words[i], &(*pip)->cmds[i]);
        parse_cmd(words[i], &(*pip)->cmds[i]);
        if ((*pip)->cmds[i]->argv[0] == NULL) {
            if (i == size-1) (*pip)->emptyFLAG = WT_FLAG; 
            else (*pip)->emptyFLAG = ER_FLAG;
        }
    }

    (*pip)->cmds[size] = NULL;
    (*pip)->size = size;

    return SU_FLAG;
}

int get_pipesize(char *line) {
    char *tmp = line;
    int size = 0;
    while (*tmp != '\0') if (*(tmp++) == '|') size++;
    return size;
}

int exec_cmd(cmd_t *cmd, int in, int out) {
    if(builtin_cmd(cmd)) return 0;
    pid_t pid = fork();
    signal(SIGINT, psig_handler);
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

        if (execvp(cmd->argv[0], cmd->argv) < 0) {
            printf("%s: command not found\n", cmd->argv[0]);
            exit(0);
        }
    }
    else {
        close(in);
        close(out);
    }
    return pid;
}

int builtin_cmd(cmd_t *cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        terminate();
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

void sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n");
        shell_prompt();
        signal(SIGINT, sig_handler);
    }
}

void psig_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n");
        signal(SIGINT, psig_handler);
    }
}
