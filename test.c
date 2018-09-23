#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void func(char *vic, char **output) {
    char *tmp = NULL;
    printf("(%s)\n", vic);
    char *t = strtok(vic, " ");
    printf("(%s)\n", vic);
    t = strtok(NULL, " ");
    printf("[%s]\n", t);
    *output = t;
    printf("%d %s\n", output, output);
}

int main(){
    char vic[30] = "echo 123 | grep 1", *tmp;
    func(vic, &tmp);
    printf("%s\n", vic);
    return 0;
}