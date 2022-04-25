#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================

enum State{
    running,
    waiting
};

//process control block
typedef struct PCB{
    int id;
    int waitingTime;
    int remainingTime;
    int executionTime;
    int priority;
    int cumulativeRunningTime;
    int waiting_start_time;
    int running_start_time;
    int arrivalTime;
    //int stoppingTime;
    //int rerunningTime;
    enum State state

}PCB;

typedef struct Process{
    //int state; //running 0 , waiting 1
    int executionTime;
    int remainingTime;
    int waitingTime;
    int arrivalTime;
    int priority; //(0 -> 10)
    int waiting_start_time;
    int running_start_time;
    int id;
    enum State state;
} process;

struct process* Process_Constructor(int id, int arrivaltime, int executiontime,int priority)
{
    process* p = (process*) malloc(sizeof(process));
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
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
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
