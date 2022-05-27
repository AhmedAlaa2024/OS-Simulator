#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int* shmRemainingtime;
int shmid;


int sem;
union Semun semun;
/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};
void down(int sem);
void up(int sem);

int main(int agrc, char * argv[])
{
    //signal(SIGCONT, SIG_IGN);
    initClk();
    key_t key_id;

    key_id = ftok("key.txt", 65);
    shmid = shmget(key_id, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    


    //semaphore
    // key_id = ftok("key", 55);
    // sem = semget(key_id, 1, 0666 | IPC_CREAT);
    // semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    // if (semctl(sem, 0, SETVAL, semun) == -1)
    // {
    //     perror("Error in semctl");
    //     exit(-1);
    // }


    int clk = getClk();
    shmRemainingtime = (int*)shmat(shmid, (void *)0, 0);
    if (*shmRemainingtime == -1)
    {
        perror("Error in attach in process");
        exit(-1);
    }

    //remainingtime = *shmRemainingtime;
    //printf("\nremaining time: %d\n", remainingtime);

    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    // while (remainingtime > 0)
    // {
    //     if(clk != getClk())
    //     {
    //         remainingtime--;
    //         clk = getClk();
            
    //         *shmRemainingtime = remainingtime;
    //         //up(sem);
    //     }

    // }


    while(*shmRemainingtime > 0);

    
    
    // if(remainingtime == 0)
    // {
    //     //then notify the schedular with termination
    //     kill(getppid(), SIGCHLD);
    // }
    
    

    kill(getppid(), SIGUSR2);
    destroyClk(false);
    
    return 0;
}




void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
