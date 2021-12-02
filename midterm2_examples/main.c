#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>

int sig_flag = 0;
int read_count = 0;
int semid;

void sig_handler(int sig)
{
    sig_flag = 1;
}

int random_sleep(int seconds) 
{
    srand48((long) getpid());

    int len = (int) ((drand48() * seconds) + 1);
    sleep(len);
    return len;
}

// ****************** PROCESS PART ******************
void read_file(int i, const char *filename)
{
    printf("Reader %d is going to read.\n", i);
    int sleep_time = 0;
    sleep_time = random_sleep(5);

    printf("----Reader %d starts reading.\n", i);
    semwait(0);
    read_count++;
    if (read_count == 1)
    {
        semwait(1);
    }
    sempost(0);

    // operation start
    printf("After around %d seconds Reader %d reads:\n", sleep_time, i);
    char line[50];
    FILE *file = fopen(filename, "r");
    int k = 0;
    while (fgets(line, sizeof(line), file))
    {
        printf(" - %s", line);
        ++k;
    }
    fclose(file);
    // operation end

    printf("----Reader %d finishes reading.\n", i);
    semwait(0);
    read_count--;
    if (read_count == 0)
    {
        sempost(1);
    }
    sempost(0);

    exit(0);
}

void write_file(int i, const char *filename)
{
    printf("Writer %d is going to write.\n", i);
    int sleep_time = 0;
    sleep_time = random_sleep(5);

    printf("----Writer %d starts writing.\n", i);
    semwait(1);

    // operation start
    printf("After around %d seconds Writer %d writes...\n", sleep_time, i);
    FILE *file = fopen(filename, "a");
    fprintf(file, "Writer %d waited around %d seconds before writing\n", i, sleep_time);
    fclose(file);
    // operation end
    
    printf("----Writer %d finishes writing.\n", i);
    sempost(1);

    exit(0);
}

int create_reader(int i, const char *filename)
{ 
    int pid = fork();

	if (pid == 0)
    {
	    if (signal(SIGUSR1, sig_handler) < 0)
        {
		    perror("signal error\n");
	        return -1;
	    }
        
	    read_file(i, filename);
    }
    else if (pid < 0)
    {
        perror("fork error\n");
	    return -1;
    }
    else
    {
	    return pid;
    }
}

int create_writer(int i, const char *filename)
{ 
    int pid = fork();

	if (pid == 0)
    {        
        if (signal(SIGUSR1, sig_handler) < 0)
        {
		    perror("signal error\n");
	        return -1;
	    }

	    write_file(i, filename);
    }
    else if (pid < 0)
    {
        perror("fork error\n");
	    return -1;
    }
    else
    {
	    return pid;
    }
}

// ****************** SEM PART ******************
int init_sem()
{
    if ((semid = semget(IPC_PRIVATE, 2, 0600|IPC_CREAT)) < 0)
    {
        perror("semget error\n");
        return -1;
    }

    // reader sem init
    if (semctl(semid, 0, SETVAL, 1) < 0)
    {
	    perror("reader setval:semctl\n");
	    return -1;
	}

    // writer sem init
    if (semctl(semid, 1, SETVAL, 1) < 0)
    {
	    perror("writer setval:semctl\n");
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

// ****************** MAIN PART ******************
int main(int argc, char *argv[])
{
    int total_pid_num = atoi(argv[1]) + atoi(argv[2]);
    pid_t pids[total_pid_num];

    init_sem();

    // for (int i = 0; i < total_pid_num; ++i)
    // {
    //     if (i <= atoi(argv[1]))
    //     {
    //         pids[i] = create_reader(i, argv[3]);
    //     }
    //     else
    //     {
    //         pids[i] = create_writer(i, argv[3]);
    //     }
    // }

    pids[0] = create_reader(0, argv[3]);
    pids[1] = create_writer(1, argv[3]);
    pids[2] = create_reader(2, argv[3]);
    pids[3] = create_reader(3, argv[3]);
    pids[4] = create_reader(4, argv[3]);
    pids[5] = create_reader(5, argv[3]);

    for (int i = 0; i < total_pid_num; ++i)
    {
        if (waitpid(pids[i], NULL, 0) < 0)
        {
            perror("waitpid error\n");
        }
    }

    exit_sem();

    return 0;
}