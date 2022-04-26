#include "headers.h"


//don't mess with this variable//
int * shmaddr;                 //
//===============================

Process* Process_Constructor(int id, int arrivaltime, int executiontime,int priority)
{
    Process* p = (Process*) malloc(sizeof(Process));
    p->id = id;
    p->arrivalTime = arrivaltime;
    p->executionTime = executiontime;
    p->priority = priority;
    return p;
}

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(0, 4, 0644 | IPC_CREAT);
    while (shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(0, 4, 0644 | IPC_CREAT);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/
void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}