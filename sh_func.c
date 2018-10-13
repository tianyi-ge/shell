#include "sh_func.h"
#include <string.h>
#include <stdio.h>

job_t jobs[MAX_JOBS];
int jobcnt = 1;

void shell_prompt() {
    printf("mumsh $ ");
    fflush(stdout);
}

void terminate() {
    printf("exit\n");
    exit(0);
}

void pipe_error() {
    printf("syntax error near unexpected token `|'\n");
}

void redir_error(char *s) {
    printf("syntax error near unexpected token `%s'\n", s);
}

void miss_error() {
    printf("error: missing program\n");
}

void dup_error(int is_dup) {
    if (is_dup == DUP_IN_ERROR) printf("error: duplicated input redirection\n");
    else if (is_dup == DUP_OUT_ERROR) printf("error: duplicated output redirection\n");
}

int finish_check(char *s) {
    int waitForFile = 0, waitForPipe = -1;
    char *send = s + strlen(s), *next;
    int pipesize = 0;
    while (s < send) {
        s += strspn(s, " \t\r\n");
        if (s >= send) break;

        if (*s == '>' && *(s+1) == '>') { //safe to check s+1 because of '\0'
            if (waitForFile) {
                redir_error(">>");
                return REDIR_ERROR;
            }
            waitForFile = 1;
            s += 2;
            continue;
        }
        else if (*s == '>' || *s == '<') {
            if (waitForFile) {
                if (*s == '>') redir_error(">");
                else redir_error("<");
                return REDIR_ERROR;
            }
            waitForFile = 1;
            s++;
            continue;
        }
        else if (*s == '\'' || *s == '\"') {
            s++;
            next = strchr(s, *(s-1));
        }
        else if (*s == '|'){
            if (waitForFile) return PIPE_ERROR; // error
            if (waitForPipe) return MISS_ERROR; // error
            waitForPipe = 1;
            pipesize++;
            s++;
            continue;
        }
        else {
            next = s + strcspn(s, " \t\r\n");
        }

        if (next == NULL) { //wait for parenthese input
            return WAIT_CMD;
        }
        waitForFile = 0; // if reaches here, must be valid
        waitForPipe = 0;
        s = next + 1;
    }
    if (waitForFile || waitForPipe == 1) return WAIT_CMD;

    return pipesize;
}

void sep_redir(char *s) {
    char backup[MAX_LEN];
    int i = 0, j = 0, k = 0;
    if (s[strlen(s) - 1] == '\n') s[strlen(s) - 1] = '\0'; //get rid of the last '\n'
    while(s[i] != '\0') {
        if (s[i] == '\'' || s[i] == '\"') {
            if (s[i] == '\'') k = i + 1 + strcspn(s + i + 1, "\'"); // from the character after s[i]
            else if (s[i] == '\"') k = i + 1 + strcspn(s + i + 1, "\"");
            while (i <= k) backup[j++] = s[i++]; // the contents amid quotes
            continue;
        }
        else if (s[i] == '>' && s[i+1] == '>') {
            backup[j++] = ' ';
            backup[j++] = '>';
            backup[j++] = '>';
            backup[j++] = ' ';
            i += 2;
            continue;
        }
        else if (s[i] == '>' || s[i] == '<' || s[i] == '|' || s[i] == '&') {
            backup[j++] = ' ';
            backup[j++] = s[i++];
            backup[j++] = ' ';
            continue;
        }
        else {
            backup[j++] = s[i++];
        }
    }
    for (int k = 0; k < j; ++k) s[k] = backup[k];
    s[j] = '\0';
}

void parse_quotes(char *s, char *send) {
    char *ptr = s, *next = NULL;
    while (ptr < send) {
        if (*ptr == '\'' || *ptr == '\"') {
            ptr++;
            next = strchr(ptr, *(ptr-1));
            while (ptr < next) *(s++) = *(ptr++);
            ptr++; next++; continue;
        }
        *(s++) = *(ptr++);
    }
    *s = '\0';
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
            if (((*cmd)->flag & OUT_APPEND) || ((*cmd)->flag & OUT_REDIR)) return DUP_OUT_ERROR;
            (*cmd)->flag |= OUT_APPEND;
            waitForFile = OUT_FILE;
            s += 2;
            continue;
        }
        else if (*s == '>') {
            if (((*cmd)->flag & OUT_APPEND) || ((*cmd)->flag & OUT_REDIR)) return DUP_OUT_ERROR;
            (*cmd)->flag |= OUT_REDIR;
            waitForFile = OUT_FILE;
            s++;
            continue;
        }
        else if (*s == '<') {
            if ((*cmd)->flag & IN_REDIR) return DUP_IN_ERROR;
            (*cmd)->flag |= IN_REDIR;
            waitForFile = IN_FILE;
            s++;
            continue;
        }
        else {
            next = s;
            while ((*next) != ' ' && (*next) != '\t' && (*next) != '\r' && (*next) != '\n' && (*next) != '\0') {
                if (*next == '\'' || *next == '\"') {
                    next++;
                    next = strchr(next, *(next-1)) + 1;
                }
                else {
                    next = s + strcspn(s, " \t\r\n");
                    break;
                }
            }
            parse_quotes(s, next);
        }
        
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

    (*cmd)->argv[cnt] = NULL;
    if (cnt > 0 && (*cmd)->argv[cnt-1][0] == '&') {
        (*cmd)->argv[--cnt] = NULL;
        return 1;
    }
    return 0;
}

int parse_pipe(char *s, pipe_t **pip, int size) {
    *pip = (pipe_t *)calloc(1, sizeof(pipe_t));
    (*pip)->cmds = (cmd_t **)calloc(size+1, sizeof(cmd_t *));
    (*pip)->cmds[size] = NULL;
    (*pip)->size = size;

    char *words[MAX_LEN];
    int cnt = 0;
    char *send = s + strlen(s), *beg = s;
    do {
        if (*s == '\'' || *s == '\"') {
            s++;
            s = strchr(s, *(s-1)) + 1;
            continue;
        }
        if (*s == '|') {
            *s = '\0';
            words[cnt++] = beg;
            beg = s + 1;
        }
        s++;
    } while (s < send);
    words[cnt++] = beg;

    int is_bg = 0, is_dup = 0;
    for (int i = 0; i < size; ++i){
        is_bg = parse_cmd(words[i], &(*pip)->cmds[i]);
        if (is_bg < 0) is_dup = is_bg; // duplicated
        if (size != 1) {
            if ((((*pip)->cmds[i]->flag & OUT_APPEND) || ((*pip)->cmds[i]->flag & OUT_REDIR)) && (i != size - 1)) is_dup = DUP_OUT_ERROR;
            if (((*pip)->cmds[i]->flag & IN_REDIR) && (i != 0)) is_dup = DUP_IN_ERROR;
        }

        if ((*pip)->cmds[i]->argv[0] == NULL) {
            if (i == 0 && size == 1)  return EM_FLAG; // empty command
            else return ER_FLAG; // empty command in the middle
        }
    }
    if (is_dup) {
        dup_error(is_dup);
        return DP_FLAG;
    }

    (*pip)->is_bg = is_bg;
    return SU_FLAG;
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

   if (strcmp(cmd->argv[0], "jobs") == 0) {
        for (int i = 1; i < jobcnt; ++i) {
            print_job_status(i);
       }
       return 1;
   }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argv[2]) {
            printf("cd: too many arguments\n");
            return 1;
        }
        if (chdir(cmd->argv[1]) == -1) printf("%s: No such file or directory\n", cmd->argv[1]);
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

void print_job(int jid) {
    printf("[%d] ", jid);
    /*
    for (int i = 0; i < jobs[jid].pcnt; ++i)
        printf("(%d) ", jobs[jid].pid[i]);
    */
    printf("%s", jobs[jid].name);
}

void print_job_status(int jid) {
    printf("[%d] ", jid);
    if (jobs[jid].status == DONE) {
        printf("done ");
    }
    else {
        int done = 1;
        for (int j = 0; j < jobs[jid].pcnt; ++j)
            if (jobs[jid].pid[j] > 0)
                if (waitpid(jobs[jid].pid[j], NULL, WNOHANG) == 0)
                    done = 0; // running
        
        if (done == 0) {
            jobs[jid].status = RUNNING;
            printf("running ");
        }
        else {
            jobs[jid].status = DONE;
            printf("done ");
        }
    }
    printf("%s", jobs[jid].name);
}
