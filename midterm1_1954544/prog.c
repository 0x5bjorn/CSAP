#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int read_line(int fd, char *buf, int sz, off_t *offset)
{
    int nchr = 0;
    int idx = 0;
    char *p = NULL;

    // position fd & read line
    if ((nchr = lseek(fd, *offset, SEEK_SET)) != -1)
    {
        nchr = read(fd, buf, sz);
    }

    if (nchr == -1)
    {
        printf("%s() error: read failure in '%d'.\n", __func__, fd);
        return nchr;
    }

    // end of file - no chars read (not an error, but return -1 )
    if (nchr == 0)
    {
        return -1;
    }

    p = buf;
    while (idx < nchr && *p != '\n')
    {
        p++, idx++;
    }
    *p = 0;

    // newline not found
    if (idx == nchr)
    {
        *offset += nchr;

        // check file missing newline at end
        return nchr < (ssize_t)sz ? nchr : 0;
    }

    *offset += idx + 1;

    return idx;
}

void read_cmds(char **cmds, const char *filename)
{
    printf("READING: %s\n", filename);

    char line[128];
    int input_fd = open(filename, O_RDONLY);
    int len = 0;
    off_t offset = 0;

    int i = 0;
    while ((len = read_line(input_fd, line, 128, &offset)) != -1)
    {
        cmds[i] = malloc(sizeof(char) * strlen(line));
        cmds[i] = strdup(line);
        ++i;
    }

    close(input_fd);
}

void read_and_exec_cmd(const char *cmd)
{
    printf("EXECUTING: %s\n", cmd);

    // *** READ AND PARSE COMMAND PART
    char *command, *arg1, *arg2;
    char *commands[] = {"CP", "LN", "UN", "RN"};
    char *token;

    token = strtok(cmd, " ");
    for (int i = 0; token != NULL; ++i)
    {        
        if (i == 0)
        {
            command = malloc(sizeof(char) * strlen(token));
            command = strdup(token);
        }
        else if (i == 1)
        {
            arg1 = malloc(sizeof(char) * strlen(token));
            arg1 = strdup(token);
        }
        else
        {
            arg2 = malloc(sizeof(char) * strlen(token));
            arg2 = strdup(token);
        }

        token = strtok(NULL, " ");
    }

    // *** EXECUTE COMMAND PART
    int fd[2];
    const int modes[] = {O_RDONLY, O_WRONLY|O_CREAT};
    const int flags = S_IRWXU;
    char buffer[1024];

    if (strcmp(command, commands[0]) == 0)
    {
        if ((fd[0] = open(arg1, modes[0], flags)) < 0)
        {
            printf("FAILED open() %s", arg1);
        }
        if ((fd[1] = open(arg2, modes[1], flags)) < 0)
        {
            printf("FAILED open() %s", arg1);
        }

        int in, out;
        while ((in = read(fd[0], buffer, 1024)) > 0)
        {
            if ((out=write(fd[1], buffer, in)) < in)
            {
                if (out < 0) 
                {
                    printf("write() error\n");
                }
                else 
                {
                    printf("Wrote only %d out of %d bytes\n", out, in);
                }
            }
        }

        close(fd[0]);
        close(fd[1]);
    }
    else if (strcmp(command, commands[1]) == 0)
    {
        if (link(arg1, arg2) < 0) {
	        printf("link() error\n");
        }
    }
    else if (strcmp(command, commands[2]) == 0)
    {
        if (unlink(arg1) < 0) {
	        printf("unlink() error\n");
        }
    }
    else if (strcmp(command, commands[3]) == 0)
    {
        if (rename(arg1, arg2) < 0) {
	        printf("rename() error\n");
        }
    }

    free(arg2);
    free(arg1);
    free(command);
    free(token);
}

// clean memory
void clean_mem(char **cmds)
{
    for (int i = 0; cmds[i] != NULL; ++i)
    {
        free(cmds[i]);
    }
    free(cmds);
}

// out put log
void output_log(const char **log)
{
    char line[256];
    int output_fd = open("prog.log", O_RDWR|O_CREAT);

    write(output_fd, log, 256);

    close(output_fd);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // read all commands from input file and store in variable cmds
    char **cmds = NULL;
    cmds = malloc(sizeof(char *) * 10);
    read_cmds(cmds, argv[1]);

    // execute commands
    for (int i = 0; cmds[i] != NULL; ++i)
    {
        read_and_exec_cmd(cmds[i]);
    }

    // output log
    // there should be log part

    // clean memory
    clean_mem(cmds);

    printf("DONE\n");
    return 0;
}