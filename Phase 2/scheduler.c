#include "headers.h"
#include "LinkedList.h"
#include "priority_queue.h"
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

/* Memory Management Segment */
int memory_size = 1024;

/* Free Lists */
LinkedList *memory[9];
LinkedList *_1B_segments;
LinkedList *_2B_segments;
LinkedList *_4B_segments;
LinkedList *_8B_segments;
LinkedList *_16B_segments;
LinkedList *_32B_segments;
LinkedList *_64B_segments;
LinkedList *_128B_segments;
LinkedList *_256B_segments;
/*****************************/

FILE *logFile, *perfFile, *memorylogFile;
int algorithm;
int algo;
PriorityQueue readyQ;
PriorityQueue waitingQ;
Process *Process_Table;
Process *running = NULL;
Process idleProcess;
int shmid;
int remainingtime;
int *shmRemainingtime;
int current_process_id;
int total_number_of_received_process;
int total_number_of_processes;
int RR_Priority;
bool if_termination;
int total_CPU_idle_time;
int number_of_terminated_processes;

float CPU_utilization;
float Avg_WTA;
float Avg_Waiting;
float Std_WTA;
float total_Waiting;
float total_WTA;
float *WTA;

bool ifReceived = false;
bool process_generator_finished = false;

int clk;

int sem;

// union Semun semun;
/* arg for semctl system calls. */

int i, Q;

key_t key1;
int shmid1;

key_t key2;
int shmid2;

key_t key3;
int shmid3;

key_t key4;
int shmid4;

int msg_id;
MsgBuf msgbuf;

key_t key_id;
key_t key;
#if (WARNINGS == 1)
#warning "Scheduler: Read the following notes carefully!"
#warning "Systick callback the scheduler.updateInformation()"
#warning "1. Increase cummualtive running time for the running process"
#warning "2. Increase waiting time for the waited process"
#warning "3. Decrease the remaining time"
#warning "-----------------------------------------------------------------------------------------------------------------"
#warning "4. Need to fork process (Uncle) to trace the clocks and interrupt the scheduler (Parent) to do the callback"
#warning "5. We need the context switching to change the state, kill, print."
#warning "-----------------------------------------------------------------------------------------------------------------"
#warning "Note:"
#warning "1. We need to make the receiving operation with notification with no blocking."
#warning "2. Set a handler upon the termination."
#warning "-----------------------------------------------------------------------------------------------------------------"
#endif
/*
1. Signal from process_generator to scheduler to receive a new arrived process
2. When process_generator is finsied, check ppid in scheduler at the end of the handler
3. If ppid == 1 (systemd), then algorithm should now it have to finsih the exist process in readyQ only
4. If not and there is no processes in readyQ, then algorithm should know there are process but not arrived yet,
 so don't terminate.
*/

void RR(int quantum);
void HPF(void);
void SRTN(void);

void down(int sem);
void up(int sem);

void updateInformation();

void handler_notify_scheduler_new_process_has_arrived(int signum);

void ProcessTerminates(int signum);

void write_in_logfile_start();
void write_in_logfile_stopped();
void write_in_logfile_resume();
void write_in_logfile_finished();
void write_in_perffile();

Segment *mergeSegments(Segment *left, Segment *right);
bool memoryInitialize();
bool memoryAllocate(Process *process);
bool memoryDeallocate(Segment *process, int id_of_category);
bool memoryManage(bool isAllocating, Process *process);

int total_CPU_idle_time = 0;
int sem1;

int main(int argc, char *argv[])
{
    memoryInitialize();

    initClk();
    total_CPU_idle_time = 0;
    signal(SIGCHLD, SIG_IGN);

    total_number_of_processes = 0;
    total_CPU_idle_time = 0;
    i = 0;
    while (argv[1][i])
        total_number_of_processes = total_number_of_processes * 10 + (argv[1][i++] - '0');

    Process_Table = malloc(sizeof(Process) * (total_number_of_processes + 1));

    signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
    signal(SIGUSR2, ProcessTerminates);

    idleProcess.id = 0;
    idleProcess.id = 0;
    CPU_utilization = 0;
    Avg_WTA = 0;
    Avg_Waiting = 0;
    Std_WTA = 0;
    total_Waiting = 0;
    total_WTA = 0;
    number_of_terminated_processes = 0;
    WTA = malloc(sizeof(float) * total_number_of_processes);

    // the remainging time of the current running process
    key_id = ftok("key.txt", 65);
    shmid = shmget(key_id, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    shmRemainingtime = (int *)shmat(shmid, (void *)0, 0);
    if (*shmRemainingtime == -1)
    {
        perror("Error in attach in scheduler --------------");
        // exit(-1);
    }

    // semaphore
    //  union Semun semun;
    //  key_t key1 = ftok("key.txt",11);
    //  sem1 = semget(key1, 1, 0666 | IPC_CREAT );

    // if(sem1 == -1)
    // {
    //     perror("Error in create sem");
    //     exit(-1);
    // }

    /* Create a message buffer between process_generator and scheduler */
    key = ftok("key.txt", 66);
    msg_id = msgget(key, (IPC_CREAT | 0666));

    if (msg_id == -1)
    {
        perror("Error in create!");
        // exit(1);
    }

    total_number_of_received_process = 0;
    current_process_id = 0;

    // printf("scheduler id is  : %d\n",getpid());

    // printf("argc: %d\n", argc);
    // printf("argv[1]: %d\n", atoi(argv[1]));
    // printf("argv[2]: %d\n", atoi(argv[2]));
    // printf("argv[3]: %d\n", atoi(argv[3]));

    if (argc < 3)
    {
        perror("Too few CLA!!");
        return -1;
    }

    // switch (argv[2][0])
    // {

    algorithm = (argv[2][0] - '0') - 1;
    // printf(".............%d\n",algorithm) ;
    if (algorithm == 2)
    {
        // RR_ALGORITHM;
        if (argc < 4)
        {
            perror("Too few CLA!!");
            return -1;
        }
        i = 0;
        Q = 0;
        while (argv[3][i])
            Q = Q * 10 + (argv[3][i++] - '0');
        // printf("\nquantum: %d\n", Q);
        // break;
        // default:
        // perror("undefined algorithm");
        // return -1;
    }
    algo = algorithm;

    logFile = fopen("Scheduler.log", "w");
    fprintf(logFile, "#At  time  x  process  y  state  arr  w  total  z  remain  y  wait  k\n"); // should we ingnore this line ?
    fflush(0);
    memorylogFile = fopen("memory.log", "w");
    fprintf(memorylogFile, "#At  time  x  allocated  y  bytes  for   process  z  from  i  to  j\n");
    fflush(0);

    RR_Priority = 0;

    while (pq_isEmpty(&readyQ))
        ; // to guarantee that once the first process arrives it will begin at once not at the next sec

    if (algo == 2)
        RR(Q);

    else if (algo == 0)
        HPF();

    else if (algo == 1)
        SRTN();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    fclose(logFile);
    write_in_perffile();
    destroyClk(true);
    // return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// from the parent we will run each scheduler each clock cycle
void RR(int quantum)
{
    // printf("I am RR! \n");
    int pid, pr;
    int clk = getClk();
    // int timeToStop;
    int currentQuantum = quantum;
    int remain_beg;
    bool was_empty = false;
    // printf("----------the quantum %d\n", quantum);
    while (total_number_of_processes)
    {
        // printf("###############################################################\n");
        // printf("Total Number of processes until now: %d\n", pq_getLength(&readyQ));
        // printf("###############################################################\n");

        // handler_notify_scheduler_new_process_has_arrived(0);
        // printf("\ni am here -------------------------------------\n");

        // printf("\ni am here \n");
        if (running)
        {

            current_process_id = running->id;

            if (running->remainingTime > 0)
            {
                running->remainingTime--;
                *shmRemainingtime = running->remainingTime;
            }

            if (running->remainingTime == 0)
                continue;

            currentQuantum--;

            //(*running).remainingTime = *shmRemainingtime;
            running->cumulativeRunningTime++;

            // to avoid stopping the process after ending the quantum if it is the only process in the system
            if (currentQuantum == 0 && pq_isEmpty(&readyQ))
            {

                currentQuantum = quantum; //+1 as it will continue without loop on the clock to change
                // continue;
            }

            else if (currentQuantum == 0)
            {

                running->state = WAITING;

                // send signal stop to this process and insert it back in the ready queue
                running->waiting_start_time = getClk();
                // down(sem);
                // while(running->remainingTime == *shmRemainingtime);
                // while(remain_beg - *shmRemainingtime == quantum);
                kill(running->pid, SIGSTOP);

                // running->remainingTime = *shmRemainingtime;

                // termination occur ??

                // if(running->remainingTime == 0)
                //     continue;

                RR_Priority++;
                pq_push(&readyQ, running, RR_Priority);

                // termination occur ??

                write_in_logfile_stopped();

                running = NULL;

                continue; // to make the next process begin exactly after the current is stopped
                // printf("\ni am here after blocking a process--------------------------------------\n");
            }
        }
        else
        {
            // printf("\ni am here -------------------------------------\n");
            if (!pq_isEmpty(&readyQ))
            {
                // printf("\ni am here -------------------------------------\n");
                // if_termination = false;
                running = pq_pop(&readyQ);
                current_process_id = running->id;

                currentQuantum = quantum;
                // running->cumulativeRunningTime++;
                if (running->state == READY)
                {
                    // meaning that it is the first time to be fun on the cpu
                    // inintialize the remaining time
                    *shmRemainingtime = running->burstTime;
                    running->waitingTime = getClk() - running->arrivalTime;
                    pid = fork();
                    if (pid == -1)
                        perror("Error in fork!!");
                    if (pid == 0)
                    {
                        pr = execl("./process.out", "process.out", (char *)NULL);
                        if (pr == -1)
                        {
                            perror("Error in the process fork!\n");
                            exit(0);
                        }
                    }
                    // put it in the Process
                    running->pid = pid;
                    running->state = RUNNING;
                    running->running_start_time = getClk();

                    remain_beg = running->burstTime;
                    // currentQuantum--;
                    write_in_logfile_start();
                }
                else
                {
                    // wake it up
                    //*shmRemainingtime = running->remainingTime;
                    //  printf("---------------------old remaining : %d\n", *shmRemainingtime);
                    remain_beg = running->remainingTime;
                    *shmRemainingtime = running->remainingTime;
                    kill(running->pid, SIGCONT); // TO ASK
                    // printf("---------------------new remaining : %d\n", *shmRemainingtime);
                    running->remainingTime == *shmRemainingtime;
                    running->state = RUNNING;
                    running->running_start_time = getClk();
                    // down(sem);
                    //*shmRemainingtime = running->remainingTime;
                    // currentQuantum--;
                    write_in_logfile_resume();
                }
            }
            else
            {
                total_CPU_idle_time++;
            }
            // else
            //     was_empty = true;
        }

        updateInformation();
        // printf("\nclk = %d   getclk = %d\n", clk, getClk());
        while (clk == getClk())
            ; // && !pq_isEmpty(&readyQ) && running
        // {
        //     if(!pq_isEmpty(&readyQ) && was_empty)
        //         break;
        // }
        clk = getClk();

        // semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        // if (semctl(sem, 0, SETVAL, semun) == -1)
        // {
        //     perror("Error in semctl");
        //     exit(-1);
        // }
    }
}
/* Warning: Under development */
void HPF(void)
{
    int pid;
    clk = getClk();
    int pr;

    running = NULL;
    int firstTime = 0;
    bool ifUpdated = true;
    while (total_number_of_processes)
    {

        // printf("###############################################################\n");
        // printf("ReadyQ length until now: %d\n", pq_getLength(&readyQ));
        // printf("###############################################################\n");

        if (running)
        {
            ifUpdated = false;
            current_process_id = running->id;

            if (running->remainingTime > 0)
            {
                running->remainingTime--;
                *shmRemainingtime = running->remainingTime;
                running->cumulativeRunningTime++;
            }
        }
        else if (!pq_isEmpty(&readyQ))
        {
            firstTime = 0;
            ifUpdated = false;
            running = pq_pop(&readyQ);
            current_process_id = running->id;

            // running->cumulativeRunningTime++;
            if (running->state == READY)
            {
                *shmRemainingtime = running->burstTime;
                running->waitingTime = getClk() - running->arrivalTime;
                pid = fork();
                if (pid == -1)
                    perror("Error in fork");
                if (pid == 0)
                {
                    pr = execl("./process.out", "process.out", (char *)NULL);
                    if (pr == -1)
                    {
                        perror("Error in the process fork");
                        exit(0);
                    }
                }
                running->pid = pid;
                running->state = RUNNING;
                running->running_start_time = getClk();

                write_in_logfile_start();
            }
        }
        else
            total_CPU_idle_time++;

        bool cont = false;
        while (clk == getClk())
        {
            if (running && running->remainingTime == 0)
            {
                cont = true;
                break;
            }
        }
        if (cont)
            continue;
        updateInformation();
        clk = getClk();
    }
    // int CPU_utilization=(1-(total_CPU_idle_time/getClk()))*100;
}
void SRTN(void)
{
    // printf("heeeeeeeeeeeeeeeeeeeeeeeere");
    fflush(0);
    int clk = getClk();
    int peek;
    int pid, pr;
    while (total_number_of_processes)
    {
        if (running && running->remainingTime == 0)
            continue;

        if (ifReceived)
            ifReceived = false;

        if (running)
        {
            if (!pq_isEmpty(&readyQ))
            {
                peek = pq_peek(&readyQ)->remainingTime;

                if (peek < running->remainingTime)
                {
                    // switch:

                    running->state = WAITING;
                    // send signal stop to this process and insert it back in the ready queue
                    running->waiting_start_time = getClk();
                    kill(running->pid, SIGSTOP);
                    pq_push(&readyQ, running, running->remainingTime);

                    write_in_logfile_stopped();
                    running = NULL;
                }
            }
        }
        if (running == NULL)
        {
            if (!pq_isEmpty(&readyQ))
            {
                running = pq_pop(&readyQ);
                current_process_id = running->id;
                *shmRemainingtime = running->remainingTime;

                if (running->state == READY)
                {
                    // Setting initial waiting time
                    running->waitingTime = getClk() - running->arrivalTime;

                    pid = fork();
                    if (pid == -1)
                        perror("Error in fork!!");
                    if (pid == 0)
                    {
                        pr = execl("./process.out", "process.out", (char *)NULL);
                        if (pr == -1)
                        {
                            perror("Error in the process fork!\n");
                            exit(0);
                        }
                    }

                    running->state = RUNNING;
                    running->running_start_time = getClk();

                    running->pid = pid;
                    current_process_id = running->id;

                    write_in_logfile_start();
                }
                if (running->state == WAITING)
                {
                    kill(running->pid, SIGCONT);
                    running->state = RUNNING;
                    running->running_start_time = getClk();

                    current_process_id = running->id;

                    running->waitingTime += getClk() - running->waiting_start_time;

                    write_in_logfile_resume();
                }
            }
        }
        while (clk == getClk())
            if (ifReceived)
                break;
        if (ifReceived)
            continue;
        if (running && running->remainingTime > 0)
        {
            running->cumulativeRunningTime++;
            running->remainingTime--;
            *shmRemainingtime = running->remainingTime;
        }
        if (!running && pq_isEmpty(&readyQ))
            total_CPU_idle_time++;
        clk = getClk();
    }
}
void updateInformation()
{

    /* Update information for the waiting processes */
    for (int i = 1; i <= total_number_of_received_process; i++)
    {
        if (i == current_process_id || Process_Table[i].id == idleProcess.id)
            continue;

        Process_Table[i].waitingTime += 1;
    }
}

// write_in_logfile
void write_in_logfile_start()
{

    // printf("process pid = %d , process id = %d\n", running->pid, running->id);
    // fflush(0);
    // printf("At  time  %d  process  %d  started  arr  %d  total  %d  remain  %d  wait  %d\n",
    //     running->running_start_time,
    //     running->id,
    //     running->arrivalTime,
    //     running->burstTime,    //to make sure ?!
    //     running->remainingTime,
    //     running->waitingTime   //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    // );
    // fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  started  arr  %i  total  %i  remain  %i  wait  %i\n",
            (*running).running_start_time,
            (*running).id,
            (*running).arrivalTime,
            (*running).burstTime, // to make sure ?!
            (*running).remainingTime,
            (*running).waitingTime // we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
}

void write_in_logfile_resume()
{
    // printf("\nAt  time  %i  process  %i  resumed  arr  %i  total  %i  remain  %i  wait  %i\n",
    //     running->running_start_time,
    //     running->id,
    //     running->arrivalTime,
    //     running->burstTime,    //to make sure ?!
    //     running->remainingTime,
    //     running->waitingTime
    // );
    // fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  resumed  arr  %i  total  %i  remain  %i  wait  %i\n",
            running->running_start_time,
            running->id,
            running->arrivalTime,
            running->burstTime, // to make sure ?!
            running->remainingTime,
            running->waitingTime);
    fflush(0);
}

void write_in_logfile_stopped()
{
    // printf("\nAt  time  %d  process  %d  stopped  arr  %d  total  %d  remain  %d  wait  %d\n",
    //     running->waiting_start_time,
    //     running->id,
    //     running->arrivalTime,
    //     running->burstTime,    //to make sure ?!
    //     running->remainingTime,
    //     running->waitingTime   //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    // );
    // fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  stopped  arr  %i  total  %i  remain  %i  wait  %i\n",
            running->waiting_start_time,
            running->id,
            running->arrivalTime,
            running->burstTime, // to make sure ?!
            running->remainingTime,
            running->waitingTime // we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
}

void write_in_logfile_finished()
{
    int clk = getClk();
    // printf("\nAt  time  %d  process  %d  finished  arr  %d  total  %d  remain  %d  wait  %d  TA  %d  WTA  %f\n",
    //     clk,
    //     running->id,
    //     running->arrivalTime,
    //     running->burstTime,    //to make sure ?!
    //     running->remainingTime,
    //     running->waitingTime ,  //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes

    //     clk - running->arrivalTime,  //finish - arrival
    //     (float)(clk - running->arrivalTime) / running->burstTime  //to ask (float)
    // );
    // fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  finished  arr  %i  total  %i  remain  %i  wait  %i  TA  %i  WTA  %.2f\n",
            clk,
            running->id,
            running->arrivalTime,
            running->burstTime, // to make sure ?!
            running->remainingTime,
            running->waitingTime, // we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes

            clk - running->arrivalTime,                              // finish - arrival
            (float)(clk - running->arrivalTime) / running->burstTime // to ask (float)
    );
    fflush(0);
    WTA[number_of_terminated_processes] = (float)(clk - running->arrivalTime) / running->burstTime;
    total_WTA += WTA[number_of_terminated_processes];
    total_Waiting += running->waitingTime;
    number_of_terminated_processes++;
}

void write_in_perffile()
{

    // printf("\ntotal_CPU_idle_time = %d\n", total_CPU_idle_time);
    total_CPU_idle_time--;

    perfFile = fopen("Scheduler.perf", "w");
    CPU_utilization = (1 - ((float)total_CPU_idle_time / getClk())) * 100.0;
    fprintf(perfFile, "CPU utilization = %.2f%%\n", CPU_utilization);
    Avg_WTA = total_WTA / total_number_of_received_process;
    fprintf(perfFile, "Avg WTA = %.2f\n", Avg_WTA);
    Avg_Waiting = (float)total_Waiting / total_number_of_received_process;
    fprintf(perfFile, "Avg Waiting = %.2f\n", Avg_Waiting);
    float sum = 0;
    for (int i = 0; i < total_number_of_received_process; i++)
    {
        sum += ((WTA[i] - Avg_WTA) * (WTA[i] - Avg_WTA));
    }
    Std_WTA = sqrt((1 / (float)total_number_of_received_process) * sum);
    fprintf(perfFile, "Std WTA = %.2f\n", Std_WTA);
    fflush(0);
    fclose(perfFile);
    return;
}

// handler_notify_scheduler_I_terminated
void ProcessTerminates(int signum)
{ // down(sem1);
    // printf("\n a process terminates\n");
    // TODO
    // implement what the scheduler should do when it gets notifies that a process is finished
    bool isMerged = memoryManage(false, &Process_Table[current_process_id]);

    running->remainingTime = *shmRemainingtime;
    write_in_logfile_finished();
    // scheduler should delete its data from the process table
    Process_Table[current_process_id] = idleProcess;
    // free(Process_Table + running->id);
    // call the function Terminate_Process
    running = NULL;
    total_number_of_processes--;
    // to ask
    // should we check on the total number of processes and if it equals 0 then terminate the scheduler

    // printf("\n after process terminates-------------------------\n");
    // int dummy=getClk();
    // while(dummy == getClk());
    if_termination = true;

    // if (isMerged)
    //     printf("Process %d is deallocated successfully! and Merging is happened!\n", Process_Table[current_process_id].pid);
    // else
    //     printf("Process %d is deallocated successfully! and Merging is not happened!\n", Process_Table[current_process_id].pid);

    Process *process;
    bool canBeAllocated = false;

    PriorityQueue tempQ;
    tempQ.num_of_nodes = 0;

    while (!pq_isEmpty(&waitingQ))
    {
        process = pq_pop(&waitingQ);
        canBeAllocated = memoryManage(true, process);

        if (canBeAllocated)
        {
            Process_Table[process->id].id = process->id;
            Process_Table[process->id].waitingTime = process->waitingTime;
            Process_Table[process->id].remainingTime = process->remainingTime;
            Process_Table[process->id].burstTime = process->burstTime;
            Process_Table[process->id].priority = process->priority;
            Process_Table[process->id].cumulativeRunningTime = process->cumulativeRunningTime;
            Process_Table[process->id].waiting_start_time = process->waiting_start_time;
            Process_Table[process->id].running_start_time = process->running_start_time;
            Process_Table[process->id].arrivalTime = process->arrivalTime;
            Process_Table[process->id].sizeNeeded = process->sizeNeeded;
            Process_Table[process->id].state = process->state;
            Process_Table[process->id].segment = process->segment;

            if (algo == 0)
                pq_push(&readyQ, &Process_Table[process->id], Process_Table[process->id].priority);
            else if (algo == 1)
            { /* WARNING: This needs change depends on the SRTN algorithm */
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
#endif
                pq_push(&readyQ, &Process_Table[process->id], Process_Table[process->id].remainingTime);
            }
            else if (algo == 2)
            {
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
#endif
                // printf("i am pushing here \n");
                RR_Priority++;
                pq_push(&readyQ, &Process_Table[process->id], RR_Priority);
            }

            break;
        }
        else
        {
            if (algo == 0)
                pq_push(&tempQ, process, (*process).priority);
            else if (algo == 1)
            { /* WARNING: This needs change depends on the SRTN algorithm */
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
#endif
                pq_push(&tempQ, process, (*process).remainingTime);
            }
            else if (algo == 2)
            {
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
#endif
                pq_push(&tempQ, process, getClk());
            }
        }
    }

    /* Return all process to the waiting Queue */
    while (tempQ.num_of_nodes > 0)
    {
        process = pq_pop(&tempQ);

        if (algo == 0)
            pq_push(&waitingQ, process, (*process).priority);
        else if (algo == 1)
        { /* WARNING: This needs change depends on the SRTN algorithm */
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
#endif
            pq_push(&waitingQ, process, (*process).remainingTime);
        }
        else if (algo == 2)
        {
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
#endif
            pq_push(&waitingQ, process, getClk());
        }
    }
    signal(SIGUSR2, ProcessTerminates);
}

void handler_notify_scheduler_new_process_has_arrived(int signum)
{
    int receiveValue;
    int rc;
    struct msqid_ds buf;
    int num_messages;

    rc = msgctl(msg_id, IPC_STAT, &buf);
    num_messages = buf.msg_qnum;
    for (int i = 0; i < num_messages; i++)
    {
        receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, (!IPC_NOWAIT));
        // printf("\nreceiveValue : %d \n", receiveValue);

        if (receiveValue != -1)
        {
// printf("\nScehduler: I received!\n");
// fflush(0);
#if (NOTIFICATION == 1)
// printf("Notification (Scheduler): { \nProcess ID: %d,\nProcessArrival Time: %d\n}\n", msgbuf.id, msgbuf.arrivalTime);
#endif
            total_number_of_received_process += 1;

            Process *temp_process = (Process *)malloc(sizeof(Process));
            (*temp_process).id = msgbuf.id;
            (*temp_process).waitingTime = msgbuf.waitingTime;
            (*temp_process).remainingTime = msgbuf.remainingTime;
            (*temp_process).burstTime = msgbuf.burstTime;
            (*temp_process).priority = msgbuf.priority;
            (*temp_process).cumulativeRunningTime = msgbuf.cumulativeRunningTime;
            (*temp_process).waiting_start_time = msgbuf.waiting_start_time;
            (*temp_process).running_start_time = msgbuf.running_start_time;
            (*temp_process).arrivalTime = msgbuf.arrivalTime;
            (*temp_process).state = msgbuf.state;
            (*temp_process).sizeNeeded = msgbuf.sizeNeeded;

            printf("At time %d process %d has arrived!\n", getClk(), temp_process->id);
            fflush(0);

            bool canBeAllocated = memoryManage(true, temp_process);

            if (canBeAllocated)
            {
                printf("Debugging at Line 922: I can allocate!\n");
                Process_Table[temp_process->id].id = (*temp_process).id;
                Process_Table[temp_process->id].waitingTime = (*temp_process).waitingTime;
                Process_Table[temp_process->id].remainingTime = (*temp_process).remainingTime;
                Process_Table[temp_process->id].burstTime = (*temp_process).burstTime;
                Process_Table[temp_process->id].priority = (*temp_process).priority;
                Process_Table[temp_process->id].cumulativeRunningTime = (*temp_process).cumulativeRunningTime;
                Process_Table[temp_process->id].waiting_start_time = (*temp_process).waiting_start_time;
                Process_Table[temp_process->id].running_start_time = (*temp_process).running_start_time;
                Process_Table[temp_process->id].arrivalTime = (*temp_process).arrivalTime;
                Process_Table[temp_process->id].sizeNeeded = (*temp_process).sizeNeeded;
                Process_Table[temp_process->id].state = (*temp_process).state;
                Process_Table[temp_process->id].segment = (*temp_process).segment;

                if (algo == 0)
                {
                    pq_push(&readyQ, &Process_Table[temp_process->id], Process_Table[temp_process->id].priority);
                }
                else if (algo == 1)
                { /* WARNING: This needs change depends on the SRTN algorithm */
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
#endif
                    pq_push(&readyQ, &Process_Table[temp_process->id], Process_Table[temp_process->id].remainingTime);
                }
                else if (algo == 2)
                {
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
#endif
                    // printf("i am pushing here \n");
                    RR_Priority++;
                    pq_push(&readyQ, &Process_Table[temp_process->id], RR_Priority);
                }
            }
            else
            {
                printf("Debugging at Line 959: I can't allocate!\n");
                if (algo == 0)
                    pq_push(&waitingQ, temp_process, (*temp_process).priority);
                else if (algo == 1)
                { /* WARNING: This needs change depends on the SRTN algorithm */
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
#endif
                    pq_push(&waitingQ, temp_process, (*temp_process).remainingTime);
                }
                else if (algo == 2)
                {
#if (WARNINGS == 1)
#warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
#endif
                    pq_push(&waitingQ, temp_process, getClk());
                }
            }

            /* Parent is systemd, which means the process_generator is died! */
            if (getppid() == 1)
            {
                // printf("My father is died!\n");
                fflush(0);
                process_generator_finished = true;
            }
        }
    }

    ifReceived = true;

    signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
}

bool memoryManage(bool isAllocating, Process *process)
{
    printf("#################################################################################\n");
    printf("Debugging at Line 993: ReadyQ's #nodes: %d\n", readyQ.num_of_nodes);
    printf("Debugging at Line 994: WaitingQ's #nodes: %d\n", waitingQ.num_of_nodes);
    printf("Debugging at Line 995: Number of nodes for 1B_list: %d\n", memory[0]->num_of_nodes);
    printf("Debugging at Line 996: Number of nodes for 2B_list: %d\n", memory[1]->num_of_nodes);
    printf("Debugging at Line 997: Number of nodes for 4B_list: %d\n", memory[2]->num_of_nodes);
    printf("Debugging at Line 998: Number of nodes for 8B_list: %d\n", memory[3]->num_of_nodes);
    printf("Debugging at Line 999: Number of nodes for 16B_list: %d\n", memory[4]->num_of_nodes);
    printf("Debugging at Line 1000: Number of nodes for 32B_list: %d\n", memory[5]->num_of_nodes);
    printf("Debugging at Line 1001: Number of nodes for 64B_list: %d\n", memory[6]->num_of_nodes);
    printf("Debugging at Line 1002: Number of nodes for 128B_list: %d\n", memory[7]->num_of_nodes);
    printf("Debugging at Line 1003: Number of nodes for 256B_list: %d\n", memory[8]->num_of_nodes);
    printf("#################################################################################\n");
    /* New Process Allocation Request */
    if (isAllocating)
    {
        /* Check if I can allocate the required process */
        if (process->sizeNeeded > memory_size)
            return false;

        return memoryAllocate(process);
    }
    else
    {
        int id_of_category = ceil(log(process->segment->size) / log(2));
        bool check = memoryDeallocate(process->segment, id_of_category);
        memory_size += pow(2, id_of_category);
        printf("At time %d freed %d bytes from proccess %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
        fflush(0);
        fprintf(memorylogFile, "At time %d freed %d bytes from proccess %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
        fflush(0);
        return check;
    }
}

bool memoryInitialize()
{
    _1B_segments = ll_constructor();
    _2B_segments = ll_constructor();
    _4B_segments = ll_constructor();
    _8B_segments = ll_constructor();
    _16B_segments = ll_constructor();
    _32B_segments = ll_constructor();
    _64B_segments = ll_constructor();
    _128B_segments = ll_constructor();
    _256B_segments = ll_constructor();

    memory[0] = _1B_segments;
    memory[1] = _2B_segments;
    memory[2] = _4B_segments;
    memory[3] = _8B_segments;
    memory[4] = _16B_segments;
    memory[5] = _32B_segments;
    memory[6] = _64B_segments;
    memory[7] = _128B_segments;
    memory[8] = _256B_segments;

    Segment *segment_4 = (Segment *)malloc(sizeof(Segment));
    segment_4->size = 256;
    segment_4->start_address = 768; // 768
    segment_4->end_address = 1023;  // 1023
    ll_Node *_256B_segment_4 = ll_newNode(segment_4, nullptr);

    Segment *segment_3 = (Segment *)malloc(sizeof(Segment));
    segment_3->size = 256;
    segment_3->start_address = 512;                                    // 512
    segment_3->end_address = 767;                                      // 767
    ll_Node *_256B_segment_3 = ll_newNode(segment_3, _256B_segment_4); // changed

    Segment *segment_2 = (Segment *)malloc(sizeof(Segment));
    segment_2->size = 256;
    segment_2->start_address = 256; // 256
    segment_2->end_address = 511;   // 511
    ll_Node *_256B_segment_2 = ll_newNode(segment_2, _256B_segment_3);

    Segment *segment_1 = (Segment *)malloc(sizeof(Segment));
    segment_1->size = 256;
    segment_1->start_address = 0; // 0
    segment_1->end_address = 255; // 255
    ll_Node *_256B_segment_1 = ll_newNode(segment_1, _256B_segment_2);

    _256B_segments->num_of_nodes += 4;
    _256B_segments->Head = _256B_segment_1;
}

void segmentation(int id_of_category, int target_of_category, Process *process)
{
    /* Base Case */
    if (id_of_category == target_of_category)
    {
        ll_Node *node = memory[id_of_category]->Head;
        memory[id_of_category]->Head = memory[id_of_category]->Head->next;
        node->next = nullptr;

        memory[id_of_category]->num_of_nodes -= 1;

        process->segment = node->data;
        free(node);

        return;
    }

    /*
    * Recursion Trace Tree
    [8] 256 -> 3
    [7] 128 -> 1
    [6] 64  -> 1
    [5] 32  -> 1
    [4] 16  -> 1
    [3] 8   -> 1
    [2] 4   -> 1
    [1] 2   -> 1
    [0] 1   -> 1
    */

    ll_Node *old_segment = memory[id_of_category]->Head;
    ll_Node *next_segment = old_segment->next;
    Segment *segment_1 = (Segment *)malloc(sizeof(Segment));
    segment_1->size = old_segment->data->size / 2;
    segment_1->end_address = old_segment->data->end_address;
    segment_1->start_address = segment_1->end_address - segment_1->size + 1;
    ll_Node *new_segment_1;

    /* Case there are some nodes in the list */
    if (memory[id_of_category - 1]->Head != nullptr)
    {
        new_segment_1 = ll_newNode(segment_1, memory[id_of_category - 1]->Head); // changed ->data menna
    }
    /* Case there are some nodes in the list */
    else
    {
        new_segment_1 = ll_newNode(segment_1, nullptr);
        memory[id_of_category - 1]->Head = new_segment_1;
    }

    Segment *segment_2 = (Segment *)malloc(sizeof(Segment));
    segment_2->size = segment_1->size;
    segment_2->start_address = old_segment->data->start_address;
    segment_2->end_address = segment_1->start_address - 1;
    ll_Node *new_segment_2 = ll_newNode(segment_2, memory[id_of_category - 1]->Head); // changed ->data menna
    memory[id_of_category - 1]->Head = new_segment_2;

    free(old_segment->data);
    free(old_segment);

    memory[id_of_category]->Head = next_segment;
    memory[id_of_category]->num_of_nodes -= 1;
    memory[id_of_category-1]->num_of_nodes += 2;
    segmentation(id_of_category - 1, target_of_category, process);
}

bool memoryAllocate(Process *process)
{
    // printf("Debugging at Line 1167: I am tring to allocate!\n");
    int size = process->sizeNeeded;
    int target_id_of_category = ceil(log(size) / log(2));
    int id_of_category = target_id_of_category;
    bool check = false;
    /* Check of my category, if there are free segments */
    while (id_of_category <= 8)
    {
        // printf("Debugging at Line 1174: id -> %d\n", id_of_category);
        // printf("Debugging at Line 1175: target -> %d\n", target_id_of_category);
        check = ll_isEmpty(memory[id_of_category]);

        if (!check)
        {
            segmentation(id_of_category, target_id_of_category, process);

            memory_size -= pow(2, id_of_category);
            printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
            fflush(0);
            fprintf(memorylogFile, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
            fflush(0);
            return true;
        }
        else
            id_of_category++;
    }

    printf("At time %d allocating Process %d is failed!\n", getClk(), process->id);
    fflush(0);
    return false;
}

Segment *mergeSegments(Segment *left, Segment *right)
{
    int id_of_old = ceil(log(left->size) / log(2));
    Segment *newSegment = (Segment *)malloc(sizeof(Segment));

    newSegment->start_address = left->start_address;
    newSegment->end_address = right->end_address;
    newSegment->size = left->size * 2;

    ll_Node *node = ll_newNode(newSegment, nullptr);

    ll_Node *previous = nullptr;
    ll_Node *walker = memory[id_of_old + 1]->Head;
    // printf("1233 line %d\n",memory[id_of_old+1]->num_of_nodes);
    if (memory[id_of_old + 1]->num_of_nodes == 0)
    {
        memory[id_of_old + 1]->Head = node;
    }

    else if (walker->data->start_address > node->data->end_address) // changed to be < by menna
    {
        node->next = walker;
        memory[id_of_old + 1]->Head = node;
    }
    else
    {
        while (walker)
        {
            previous = walker;
            walker = walker->next;

            if (!walker)
            {
                previous->next = node;
                node->next = walker; /* Which is nullptr */
                break;
            }
            else if (walker->data->start_address > node->data->end_address)
            {
                previous->next = node;
                node->next = walker;
                break;
            }
        }
    }

    printf("At time %d merging %d-segment starting at %d and %d into %d-segment at %d\n", getClk(), left->size, left->start_address, right->start_address, newSegment->size, newSegment->start_address);
    memory[id_of_old + 1]->num_of_nodes++;
    return newSegment;
}

bool memoryDeallocate(Segment *segment, int id_of_category)
{
    /*
    printf("Debugging at Line 1218: Deallocation Function!\n");
    printf("Debugging at Line 1219: id_of_category: %d\n", id_of_category);
    printf("Debugging at Line 1220: Number of nodes: %d\n", memory[id_of_category]->num_of_nodes);
    */

    // 25 KB -> starting address = 64, Ending Address = 95
    //  I will search for a 32KB-segment starting with 96 or ending with 63 (left or right)
    // 96 / 32 = 3 % 2 = 1 -> [Odd]
    // (63+1) / 32 = 2 % 2 = 0 -> Even
    // 128
    // Size:    *32 *16  [16]  *16 16 *32
    // Starting: 0   32  [48]   64 80  96
    // Ending:   31  47  [63]   79 95  127
    // Parity:   E   E   [O]    E  O   O
    // If Even -> Merge with right
    // If Odd -> Merge with left

    // empty
    // even head

    if (id_of_category == 9)
        return true;

    if (id_of_category == 8)
    {
        Segment *segment_Left = (Segment *)malloc(sizeof(Segment));
        Segment *segment_Right = (Segment *)malloc(sizeof(Segment));
        segment_Left->start_address = segment->start_address;
        segment_Left->end_address = segment->start_address + 127;
        segment_Left->size = 128;
        segment_Right->start_address = segment_Left->end_address + 1;
        segment_Right->end_address = segment->end_address;
        segment_Right->size = 128;
        mergeSegments(segment_Left, segment_Right);
        
        return true;
    }

    if (memory[id_of_category]->num_of_nodes == 0)
    {
        printf("Debugging at Line 1254: Start Critical Case!\n");
        fflush(0);
        printf("Debugging at Line 1255: id_of_category: %d\n", id_of_category);
        fflush(0);
        memory[id_of_category]->Head = ll_newNode(segment, nullptr);
        memory[id_of_category]->num_of_nodes++;
        printf("Debugging at Line 1257: End Critical Case!\n");
        fflush(0);
        return true;
    }

    bool check = (segment->start_address / segment->size) % 2;
    bool isFound = false;

    /* It's an even segment */
    if (check == 0)
    {
        ll_Node *walker = memory[id_of_category]->Head; // 1
        ll_Node *temp = memory[id_of_category]->Head;   // 2

        if ((segment->end_address + 1 == walker->data->start_address))
        {

            (memory[id_of_category]->num_of_nodes)--;
            memory[id_of_category]->Head = walker->next;

            isFound = true;
        }

        while (walker && !isFound)
        {

            temp = walker;         // 1
            walker = walker->next; // 2

            if (segment->end_address + 1 == walker->data->start_address)
            {
                isFound = true;

                temp->next = walker->next;
                walker->next = nullptr;

                (memory[id_of_category]->num_of_nodes)--;
                break;
            }
        }

        if (isFound)
        {
            Segment *newSegment = mergeSegments(segment, walker->data);
            free(walker);

            return memoryDeallocate(newSegment, id_of_category + 1);
        }
        else
            return false;
    }
    else /* It's an odd segment */
    {
        ll_Node *walker = memory[id_of_category]->Head; // 1
        ll_Node *temp = walker;                         // 1

        if ((segment->start_address - 1 == walker->data->end_address))
        {

            (memory[id_of_category]->num_of_nodes)--;
            memory[id_of_category]->Head = temp->next;

            isFound = true;
        }

        while (walker && !isFound)
        {
            temp = walker;         // 1
            walker = walker->next; // 2
            if (segment->start_address - 1 == walker->data->end_address)
            {
                isFound = true;

                temp->next = walker->next;
                walker->next = nullptr;

                (memory[id_of_category]->num_of_nodes)--;
            }

            temp = walker;         // 2
            walker = walker->next; // 3
        }

        if (isFound)
        {
            Segment *newSegment = mergeSegments(walker->data, segment);
            free(walker);

            return memoryDeallocate(newSegment, id_of_category + 1);
        }
    }
}