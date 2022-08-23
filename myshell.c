#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>

#define PATH_SIZE 255
#define BUFFER_SIZE 200
#define CMD_SIZE 100

char myPath[PATH_SIZE];
char oldPath[PATH_SIZE];
int nowUmask;

// 字符数组转8进制
int atoo(char *arr) {
    int a = atoi(arr);
    int u = a / 100;
    int g = a / 10 % 10;
    int o = a % 10;
    if (a < 0 || a > 777 || u > 7 || g > 7 || o > 7) {
        printf("invalid mode: %s\n", arr);
        return -1;
    }

    a = 0;
    if (u / 4 % 2 == 1)
        a += S_IRUSR;
    if (u / 2 % 2 == 1)
        a += S_IWUSR;
    if (u % 2 == 1)
        a += S_IXUSR;
    if (g / 4 % 2 == 1)
        a += S_IRGRP;
    if (g / 2 % 2 == 1)
        a += S_IWGRP;
    if (g % 2 == 1)
        a += S_IXGRP;
    if (o / 4 % 2 == 1)
        a += S_IROTH;
    if (o / 2 % 2 == 1)
        a += S_IWOTH;
    if (o % 2 == 1)
        a += S_IXOTH;

    return a;
}

void cd(char *path) {
    if (path == NULL || !strcmp(path, "~"))
        path = "/root";
    else if (!strcmp(path, "-")) {
        // 一开始没有上一层目录
        if (strcmp(oldPath, ""))
            path = oldPath;
        else {
            printf("no oldPath\n");
            return;
        }
    }

    if (!chdir(path)) {
        // 只有当前目录发生变化才记录
        if (strcmp(path, myPath))
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
    // 创建子进程
    pid_t pid = fork();
    if (pid == 0) {
        // 数组必须以NULL结尾
        execvp(args[0], args);
        // 成功执行结束当前进程，否则继续
        perror("execvp error");
    } else if (pid < 0)
        perror("fork error");
    // 等待子进程结束
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

    if (access(file, F_OK)) {
        printf("file not found: %s\n", file);
        return;
    }

    int permission = atoo(arg);

    if (chmod(file, permission))
        perror("chmod error");
}

void myChown(char *file, char *owner) {

}

void myChgrp(char *file, char *group) {

}

void myMkdir(char *dir) {
    if (dir == NULL) {
        printf("need directory name\n");
        return;
    }

    if (!access(dir, F_OK)) {
        printf("dir or file exist: %s\n", dir);
        return;
    }

    if (mkdir(dir, 0755))
        perror("create dir error");
}

void myRmdir(char *dir) {
    if (dir == NULL) {
        printf("need directory name\n");
        return;
    }

    if (access(dir, F_OK)) {
        printf("dir not exist: %s\n", dir);
        return;
    }

    if (rmdir(dir))
        perror("delete dir error");
}

void myUmask(char *newUmask) {
    if (newUmask == NULL) {
        printf("%04o\n", nowUmask);
        return;
    }

    nowUmask = atoo(newUmask);

    umask(nowUmask);
}

void mv(char *oldName, char *newName) {
    if (oldName == NULL || newName == NULL) {
        printf("need more permission\n");
        return;
    }

    if (access(oldName, F_OK)) {
        printf("dir or file not exist: %s\n", oldName);
        return;
    }

    if (!access(newName, F_OK)) {
        printf("dir or file exist: %s\n", newName);
        return;
    }

    if (rename(oldName, newName))
        perror("mv error");
}

void cp(char *oldName, char *newName) {
    if (oldName == NULL || newName == NULL) {
        printf("need more permission\n");
        return;
    }

    if (access(oldName, F_OK)) {
        printf("dir or file not exist: %s\n", oldName);
        return;
    }

    if (!access(newName, F_OK)) {
        printf("dir or file exist: %s\n", newName);
        return;
    }

    int fd1, fd2;
    // 成功读取返回文件描述符，否则返回-1
    if ((fd1 = open(oldName, O_RDONLY)) == -1) {
        perror("read file error");
        return;
    }
    // 返回新的文件描述符, 否则返回-1
    if ((fd2 = creat(newName, 0644)) == -1) {
        perror("create file error");
        return;
    }

    // 缓冲区大小，一次性存取的字节数
    char buffer[BUFFER_SIZE];
    // 实际存取的字节数
    int rc, wc = 0;
    // 从fd指向的文件读取一定数量的字节，存入缓冲区buf中，同时文件的当前读写位置向后移
    // 成功返回读出的字节数，失败返回-1，到达文件末尾返回0
    while ((rc = read(fd1, buffer, BUFFER_SIZE)) > 0) {
        // 将缓冲区中一定数量的字节写入fd指向的文件，同时文件的当前读写位置向后移
        // 成功返回写入的字节数，失败返回-1
        wc = write(fd2, buffer, rc);
    }

    close(fd1);
    close(fd2);

    if (rc == -1 || wc == -1)
        perror("cp error");
}

void rm(char *file) {
    if (file == NULL) {
        printf("need file name\n");
        return;
    }

    if (access(file, F_OK)) {
        printf("file not exist: %s\n", file);
        return;
    }

    if (remove(file))
        perror("rm error");
}

void ln() {

}

void cat(char *file) {
    int fd;
    if ((fd = open(file, O_RDONLY)) == -1) {
        perror("read file error");
        return;
    }

    char buffer[BUFFER_SIZE];
    int rc, wc = 0;
    while ((rc = read(fd, buffer, BUFFER_SIZE)) > 0)
        // 0、1、2、代表标准输入、标准输出、标准错误输出
        wc = write(1, buffer, rc);

    close(fd);

    if (rc == -1 || wc == -1)
        perror("cat error");
}

void passwd() {

}

int main() {
    cd(NULL);
    nowUmask = 0022;
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
            if (buffer[i] != ' ' && !flag) {
                cmd[index] = buffer + i;
                flag = 1;
            } else if (buffer[i] == ' ' && flag) {
                // last error
                buffer[i] = '\0';
                flag = 0;
                ++index;
            }
        }

        // 空格回车
        if (cmd[0] == NULL)
            continue;
        else if (!strcmp(cmd[0], "cd"))
            cd(cmd[1]);
        else if (!strcmp(cmd[0], "pwd"))
            pwd();
        else if (!strcmp(cmd[0], "ls"))
            ls(cmd);
        else if (!strcmp(cmd[0], "pid"))
            pid();
        else if (!strcmp(cmd[0], "exit"))
            break;
        else if (!strcmp(cmd[0], "chmod"))
            myChmod(cmd[2], cmd[1]);
        else if (!strcmp(cmd[0], "chown"))
            myChown(cmd[2], cmd[1]);
        else if (!strcmp(cmd[0], "chgrp"))
            myChgrp(cmd[2], cmd[1]);
        else if (!strcmp(cmd[0], "mkdir"))
            myMkdir(cmd[1]);
        else if (!strcmp(cmd[0], "rmdir"))
            myRmdir(cmd[1]);
        else if (!strcmp(cmd[0], "umask"))
            myUmask(cmd[1]);
        else if (!strcmp(cmd[0], "mv"))
            mv(cmd[1], cmd[2]);
        else if (!strcmp(cmd[0], "cp"))
            cp(cmd[1], cmd[2]);
        else if (!strcmp(cmd[0], "rm"))
            rm(cmd[1]);
        else if (!strcmp(cmd[0], "ln"))
            ln();
        else if (!strcmp(cmd[0], "cat"))
            cat(cmd[1]);
        else if (!strcmp(cmd[0], "passwd"))
            passwd();
        else
            printf("no command %s\n", cmd[0]);
    }
    return 0;
}
