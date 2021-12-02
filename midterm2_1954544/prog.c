#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct file_info {
    char *filepath;
    unsigned int blocks;
    unsigned int size;
} file_info_t;

int read_count = 0;
int semid;
const int reader_sem = 0;
const int main_sem = 1;

// ****************** SEM PART ******************
int init_sem()
{
    if ((semid = semget(IPC_PRIVATE, 2, 0600|IPC_CREAT)) < 0)
    {
        perror("semget error\n");
        return -1;
    }

    // reader sem init
    if (semctl(semid, reader_sem, SETVAL, 1) < 0)
    {
	    perror("reader setval:semctl\n");
	    return -1;
	}

    // main sem init
    if (semctl(semid, main_sem, SETVAL, 1) < 0)
    {
	    perror("main setval:semctl\n");
	    return -1;
	}

    return 0;
}

int exit_sem()
{
    if (semctl(semid, 0, IPC_RMID)) {
        perror("semctl rm\n");
        return -1;
    }

    return 0;
}

int sempost(int snum)
{
    struct sembuf sop;
    sop.sem_num = snum;
    sop.sem_op = 1;
    sop.sem_flg = 0;

    if (semop(semid, &sop, 1))
    {
        perror("post:semop error\n");
        return -1;
    }

    return 0;
}

int semwait(int snum)
{
    struct sembuf sop;
    sop.sem_num = snum;
    sop.sem_op = -1;
    sop.sem_flg = 0;

    if (semop(semid, &sop, 1))
    {
        perror("wait:semop error\n");
        return -1;
    }

    return 0;
}

// ****************** PROCESS PART ******************
void swap_min(file_info_t **file_infos, int n, file_info_t *fi)
{
    int min_index = 0;
    int min_blocks = (*file_infos)[min_index].blocks;
    int min_size = (*file_infos)[min_index].size;

    for (int i = 0; i < n; ++i)
    {
        if (file_infos[i]->blocks < min_blocks || file_infos[i]->size < min_size)
        {
            min_blocks = file_infos[i]->blocks;
            min_size = file_infos[i]->size;
            min_index = i;
        }
    }

    file_infos[min_index] = fi;
}

int get_file_size(const char *path, struct stat info, file_info_t **file_infos, int n)
{
    file_info_t *fi = malloc(sizeof(file_info_t));
    fi->filepath = path;
    fi->blocks = info.st_blocks; 
    fi->size = info.st_size;

    for (int i = 0; i < n; ++i)
    {
        if (file_infos[i]->blocks < fi->blocks && file_infos[i]->size < fi->blocks)
        {
            swap_min(file_infos, n, fi);
        }
    }
}

void read_dir(char *dirname)
{
    DIR* dirh;
    struct dirent *dirp;
    struct stat info;
    char pathname[1024];
    int fd;

    if ((dirh = opendir(dirname)) == NULL)
    {
        perror("opendir");
        exit(1);
    }

    for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh)) 
    {

        if (strcmp(".", dirp->d_name) == 0 || 
            strcmp("..", dirp->d_name) == 0 || 
            strcmp("lost+found", dirp->d_name) == 0)
        {
            continue;
        }

        snprintf(pathname, sizeof(pathname) , "%s/%s", dirname, dirp->d_name);

        if ((fd = open(pathname, O_RDONLY)) < 0)
        {
            fprintf(stderr, "%s:", pathname);
            perror("open");
            continue;
        }

        if (fstat(fd, &info) == -1) 
        {
            perror("stat");
            close(fd);
            continue;
        }

        if (S_ISREG(info.st_mode))
        {
            // printf("%s is a file\n", pathname);
            pathname[strlen(pathname)] = '\0';

            // reader sem lock
            semwait(reader_sem);
            read_count++;
            if (read_count == 1)
            {
                semwait(main_sem);
            }
            sempost(reader_sem);

            printf("%s ", pathname);

            // reader sem unlock
            semwait(reader_sem);
            read_count--;
            if (read_count == 0)
            {
                sempost(main_sem);
            }
            sempost(reader_sem);
        }

        if (S_ISDIR(info.st_mode)) 
        {
            // printf("%s is a directory\n", pathname);
            read_dir(pathname);
        }

        close(fd);
    }
}

void create_reader_proc(const char *path, file_info_t **file_infos, int n)
{
    int channel[2];
    if (pipe(channel) == -1)
    {    
        perror("pipe error");
        exit(1);
    }

    int pid = fork();

	if (pid == 0)                       // child
    {
        close(1);
        if (dup2(channel[1], 1) < 0)
        {
            perror("dup2");
            exit(1);
        }

        read_dir(path);

        exit(0);
    }
    else if (pid < 0)
    {
        perror("fork error\n");
        exit(1);
    }
    else                                // parent
    {
        char read_buffer[1024];
        int c, status;
        char *parsed_string;

        if ((pid = waitpid(-1, &status, 0)) < 0)
        {
            perror("waitpid");
        }

        close(channel[1]);
        while ((c = read(channel[0], read_buffer, sizeof(read_buffer)-1)) != 0)
        {
            parsed_string = strtok(read_buffer, " ");
            
            while (parsed_string != NULL)
            {
                int fd;
                struct stat info;
                if ((fd = open(read_buffer, O_RDONLY)) < 0)
                {
                    fprintf(stderr, "%s:", read_buffer);
                    perror("open");
                }
                if (fstat(fd, &info) == -1) 
                {
                    perror("stat");
                    close(fd);
                }

                semwait(main_sem);
                get_file_size(read_buffer, info, file_infos, n);
                sempost(main_sem);

                close(fd);

                parsed_string = strtok (NULL, " ");
            }
        }
        close(channel[0]);

        // for (int i = 0; i < n; ++i)
        // {
        //     printf("%s, %d, %d\n", file_infos[i]->filepath, file_infos[i]->blocks, file_infos[i]->size);
        // }
    }
}

// ****************** MAIN PART ******************
int main(int argc, char *argv[])
{
    if (argc < 3 || atoi(argv[1]) < 0)
    {
        printf("Usage: %s <N+> <path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    init_sem();

    file_info_t *file_infos[atoi(argv[1])];
    for (int i = 0; i < atoi(argv[1]); ++i)
    {
        file_infos[i] = malloc(sizeof(file_info_t));
        file_infos[i]->filepath = "no path";
        file_infos[i]->blocks = 0;
        file_infos[i]->size = 0;
    }

    for (int i = 2; i < argc; ++i)
    {
        create_reader_proc(argv[i], file_infos, atoi(argv[1]));
    }

    for (int i = 0; i < atoi(argv[1]); ++i)
    {
        printf("%s, %d, %d\n", file_infos[i]->filepath, file_infos[i]->blocks, file_infos[i]->size);
    }

    exit_sem();

    return 0;
}
