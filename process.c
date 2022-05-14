#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int* shmRemainingtime;
int shmid;

int main(int agrc, char * argv[])
{
    initClk();
    key_t key_id;

    key_id = ftok("key", 65);

    shmid = shmget(key_id, sizeof(int), IPC_CREAT | 0644);

    if (shmid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    

    int clk = getclk();
    shmRemainingtime = (int*)shmat(shmid, (void *)0, 0);
    if (shmRemainingtime == -1)
    {
        perror("Error in attach in process");
        exit(-1);
    }

    remainingtime = *shmRemainingtime;

    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        if(clk != getClk())
        {
            remainingtime--;
            clk = getClk();
            *shmRemainingtime = remainingtime;
        }

    }
    
    // if(remainingtime == 0)
    // {
    //     //then notify the schedular with termination
    //     kill(getppid(), SIGCHLD);
    // }
    
    destroyClk(false);
    
    return 0;
}
