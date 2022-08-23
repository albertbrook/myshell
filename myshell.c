#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define PATH_SIZE 255
#define BUFFER_SIZE 200
#define CMD_SIZE 100

char myPath[PATH_SIZE];
char oldPath[PATH_SIZE];

void cd(char *path) {
    if (path == NULL || strcmp(path, "~") == 0)
        path = "/root";
    else if (strcmp(path, "-") == 0) {
        if (strcmp(oldPath, "") != 0)
            path = oldPath;
        else {
            printf("no oldPath\n");
            return;
        }
    }

    if (chdir(path) == 0) {
        if (strcmp(path, myPath) != 0)
            memcpy(oldPath, myPath, PATH_SIZE);
        getcwd(myPath, PATH_SIZE);
    } else
        printf("not found %s\n", path);
}

void pwd() {
    getcwd(myPath, PATH_SIZE);
    printf("%s\n", myPath);
}

void ls(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        printf("execvp error\n");
    } else if (pid < 0)
        printf("fork error\n");
    wait(NULL);
}

void pid() {
    printf("%d\n", getpid());
}

void myChmod(char *file, char *arg) {
    if (file == NULL || arg == NULL) {
        printf("command error\n");
        return;
    }
    int a = atoi(arg);
    int u = a / 100;
    int g = a / 10 % 10;
    int o = a % 10;
    if (a < 0 || a > 777 || u > 7 || g > 7 || o > 7) {
        printf("invalid mode\n");
        return;
    }

    int permission = 0;
    if (u / 4 % 2 == 1)
        permission += S_IRUSR;
    if (u / 2 % 2 == 1)
        permission += S_IWUSR;
    if (u % 2 == 1)
        permission += S_IXUSR;
    if (g / 4 % 2 == 1)
        permission += S_IRGRP;
    if (g / 2 % 2 == 1)
        permission += S_IWGRP;
    if (g % 2 == 1)
        permission += S_IXGRP;
    if (o / 4 % 2 == 1)
        permission += S_IROTH;
    if (o / 2 % 2 == 1)
        permission += S_IWOTH;
    if (o % 2 == 1)
        permission += S_IXOTH;

    if (chmod(file, permission) < 0)
        printf("chmod error\n");
}

int main() {
    cd(NULL);
    while (1) {
        printf("albertbrook:%s$ ", myPath);

        char buffer[BUFFER_SIZE] = {'\0'};
        if (scanf("%[^\n]%*c", buffer) == 0) {
            // 直接回车
            scanf("%c", &buffer[0]);
            continue;
        }

        char *cmd[CMD_SIZE] = {NULL};
        int index = 0, flag = 0;
        for (int i = 0; i < BUFFER_SIZE && buffer[i] != '\0'; ++i) {
            if (buffer[i] != ' ' && flag == 0) {
                cmd[index] = buffer + i;
                flag = 1;
            } else if (buffer[i] == ' ' && flag == 1) {
                // last error
                buffer[i] = '\0';
                flag = 0;
                ++index;
            }
        }

        // 空格回车
        if (cmd[0] == NULL)
            continue;
        else if (strcmp(cmd[0], "cd") == 0)
            cd(cmd[1]);
        else if (strcmp(cmd[0], "pwd") == 0)
            pwd();
        else if (strcmp(cmd[0], "ls") == 0)
            ls(cmd);
        else if (strcmp(cmd[0], "pid") == 0)
            pid();
        else if (strcmp(cmd[0], "exit") == 0)
            break;
        else if (strcmp(cmd[0], "chmod") == 0)
            myChmod(cmd[2], cmd[1]);
        else
            printf("no command %s\n", cmd[0]);
    }
    return 0;
}
