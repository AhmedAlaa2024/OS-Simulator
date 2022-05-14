#ifndef _HEADER_H
#define _HEADER_H

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

/* Set it to 1 to use the nondefault handlers for some signals speciefied in the code! */
#define HANDLERS        0

/* Set it to 0 if you want to disable the warnings written withen the code! */
#define WARNINGS        0

/* Set it to 0 if you want to disable the notifications upon communication written withen the code! */
#define NOTIFICATION    1

/* Set it to 0 if you want to disable the debugging mode! */
#define DEBUGGING       0

#define ADDRESS(element) (&(element))

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

typedef enum {
    RUNNING = 0,
    WAITING = 1,
    READY = 2  //mean that the process hasn't run yet on the cpu before
} State;

typedef enum {
    HPF_ALGORITHM = 0,
    SRTN_ALGORITHM = 1,
    RR_ALGORITHM = 2
} ALGORITHM;

typedef struct {
    //int state; //running 0 , waiting 1
    int id;
    int pid;
    int burstTime;
    int waitingTime;
    int remainingTime;
    int priority; //(0 -> 10)
    int cumulativeRunningTime;
    int waiting_start_time; 
    int running_start_time;
    int arrivalTime;
    State state;
} Process;

typedef struct {
 long mtype; /* type of message */
 /* The process as a message */
 int id;
 int waitingTime;
 int remainingTime;
 int burstTime;
 int priority; //(0 -> 10)
 int cumulativeRunningTime;
 int waiting_start_time; 
 int running_start_time;
 int arrivalTime;
 State state;
} MsgBuf;

Process* Process_Constructor(int id, int arrivaltime, int executiontime,int priority);
int getClk();


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk();


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/
void destroyClk(bool terminateAll);
int get_shmid(void);
#endif