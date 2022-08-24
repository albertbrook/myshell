#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/types.h>
#include <dirent.h>

#define PATH_SIZE 255
#define BUFFER_SIZE 200
#define CMD_SIZE 100
#define CMD_COUNT 50

char myPath[PATH_SIZE];
char oldPath[PATH_SIZE];
int nowUmask;

struct desc {
    char *command;
    char *describe;
};

// 字符数组转3位8进制数
int atoo(char *arr) {
    if (arr == NULL)
        return -1;

    int a = atoi(arr);
    int u = a / 100;
    int g = a / 10 % 10;
    int o = a % 10;
    if (a < 0 || a > 777 || u > 7 || g > 7 || o > 7)
        return -1;

    // 除2取余法
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

struct desc myDesc[CMD_COUNT] = {
        {"cd",  "cd 进入目录\ncd [path]\nNULL\t家目录\n~\t家目录\n-\t上一级目录"},
        {"pwd", "pwd 打印当前工作目录"},
        {"lls", "lls 列出所有文件和文件夹"}
};

void help(char *cmd) {
    for (int i = 0; i < CMD_COUNT && myDesc[i].command != NULL; ++i) {
        if (!strcmp(myDesc[i].command, cmd)) {
            printf("%s\n", myDesc[i].describe);
            return;
        }
    }
    printf("not found command: %s\n", cmd);
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

void lls(char *dir) {
    if (dir == NULL) {
        char path[PATH_SIZE];
        getcwd(path, PATH_SIZE);
        dir = path;
    }

    DIR *d = opendir(dir);
    if (d == NULL) {
        printf("dir not exist\n");
        return;
    }

    struct dirent *ptr;
    while (ptr = readdir(d)) {
        if (ptr->d_type == 4)
            printf("\033[31m%s\t", ptr->d_name);
        else if (ptr->d_type == 8)
            printf("\033[32m%s\t", ptr->d_name);
        else if (ptr->d_type == 10)
            printf("\033[34m%s\t", ptr->d_name);
    }

    printf("\n\033[37m");
    closedir(d);
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

    int permission;
    if (permission = atoo(arg)) {
        printf("invalid mode: %s\n", arg);
        return;
    }

    if (chmod(file, permission))
        perror("chmod error");
}

void myChown(char *file, char *owner) {
    if (file == NULL || owner == NULL) {
        printf("command error\n");
        return;
    }

    if (access(file, F_OK)) {
        printf("file not found: %s\n", file);
        return;
    }

    // 根据字符串返回包含uid和gid等信息的结构体
    struct passwd *pwd = getpwnam(owner);
    if (pwd == NULL) {
        printf("not a user: %s\n", owner);
        return;
    }
    uid_t uid = pwd->pw_uid;

    // uid或gid的值为-1时，对应的id不变
    if (chown(file, uid, -1))
        perror("chown error");
}

void myChgrp(char *file, char *group) {
    if (file == NULL || group == NULL) {
        printf("command error\n");
        return;
    }

    if (access(file, F_OK)) {
        printf("file not found: %s\n", file);
        return;
    }

    // 根据字符串返回包含uid和gid等信息的结构体
    struct passwd *pwd = getpwnam(group);
    if (pwd == NULL) {
        printf("not a group: %s\n", group);
        return;
    }
    gid_t gid = pwd->pw_gid;

    // uid或gid的值为-1时，对应的id不变
    if (chown(file, -1, gid))
        perror("chgrp error");
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

    int mask;
    if (mask = atoo(newUmask)) {
        printf("not octal number\n");
        return;
    }

    nowUmask = mask;
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

void ln(char **args) {
    if (args[1] == NULL || args[2] == NULL || (!strcmp(args[1], "-s") && args[3] == NULL)) {
        printf("need more permission\n");
        return;
    }

    if (strcmp(args[1], "-s")) {
        if (link(args[1], args[2]))
            perror("ln error");
    } else {
        if (symlink(args[2], args[3]))
            perror("ln error");
    }
}

void cat(char *file) {
    if (file == NULL) {
        printf("need file name\n");
        return;
    }

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

void passwd(char *owner) {
    if (owner == NULL) {
        // 得到用户名
        struct passwd *pwd = getpwuid(getuid());
        owner = pwd->pw_name;
    }

    // 输出用户密码
    struct spwd *spwd = getspnam(owner);
    if (spwd == NULL) {
        printf("not a user: %s\n", owner);
        return;
    }
    printf("%s:%s\n", owner, spwd->sp_pwdp);

    // 调用外部程序passwd
    char cmd[CMD_SIZE] = "passwd ";
    strcat(cmd, owner);
    system(cmd);
}

void ed(char *file) {
    if (file == NULL) {
        printf("need file name\n");
        return;
    }

    //
}

void touch(char *file) {
    if (file == NULL) {
        printf("need file name\n");
        return;
    }

    if (!access(file, F_OK)) {
        printf("file exist\n");
        return;
    }

    int fd;
    if ((fd = creat(file, 0644)) == -1) {
        perror("create file error");
        return;
    }

    char buffer[] = "\0";
    if (write(fd, buffer, 0) == -1)
        perror("touch error");

    close(fd);
}

void format(char *style) {
    if (style == NULL)
        style = "0";

    if (style[0] != '3' && style[0] != '4')
        printf("\033[%cm", style[0]);
    else
        printf("\033[%c%cm", style[0], style[1]);
}

void clear() {
    printf("\033c");
}

void call(char **args) {
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
        else if (cmd[1] != NULL && !strcmp(cmd[1], "?"))
            help(cmd[0]);
        else if (!strcmp(cmd[0], "cd"))
            cd(cmd[1]);
        else if (!strcmp(cmd[0], "pwd"))
            pwd();
        else if (!strcmp(cmd[0], "lls"))
            lls(cmd[1]);
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
            ln(cmd);
        else if (!strcmp(cmd[0], "cat"))
            cat(cmd[1]);
        else if (!strcmp(cmd[0], "passwd"))
            passwd(cmd[1]);
        else if (!strcmp(cmd[0], "ed"))
            ed(cmd[1]);
        else if (!strcmp(cmd[0], "touch"))
            touch(cmd[1]);
        else if (!strcmp(cmd[0], "format"))
            format(cmd[1]);
        else if (!strcmp(cmd[0], "c"))
            clear();
        else
            call(cmd);
    }
    return 0;
}
