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

/*************************************** Log Files ****************************************/
FILE *logFile, *perfFile, *memorylogFile, *DUMP;
/*******************************************************************************************/


/***************************** Scheduling Queues and Tables ********************************/
PriorityQueue readyQ;
PriorityQueue waitingQ;
Process *Process_Table;
/*******************************************************************************************/


/************************************ CPU Management **************************************/
Process *running = NULL;
Process idleProcess;

int remainingtime;
int *shmRemainingtime;
int current_process_id;
int total_number_of_received_process;
int total_number_of_processes;
int number_of_terminated_processes;

float CPU_utilization;
float Avg_WTA;
float Avg_Waiting;
float Std_WTA;
float total_Waiting;
float total_WTA;
float *WTA;
/*******************************************************************************************/

/************************************ Scheduling Flags *************************************/
bool ifReceived = false;
bool process_generator_finished = false;
/*******************************************************************************************/

/************************************ DUMY Variables **************************************/
int algorithm;
int algo;
int clk;
int i, Q;
int total_CPU_idle_time;
int RR_Priority;
int FINISH;
/*******************************************************************************************/

/****************************************** Keys *****************************************/
key_t key_id;
int shmid;

key_t key1;
int shmid1;

key_t key2;
int shmid2;

key_t key3;
int shmid3;

key_t key4;
int shmid4;

key_t key;
int msg_id;
MsgBuf msgbuf;
/*******************************************************************************************/

/************************************ Scheduling Algorithms *********************************/
void RR(int quantum);
void HPF(void);
void SRTN(void);
/*******************************************************************************************/

/***************************************** Utilities ***************************************/
void updateInformation();
/*******************************************************************************************/

/************************************ Signal Handlers **************************************/
void handler_notify_scheduler_new_process_has_arrived(int signum);
void ProcessTerminates(int signum);
void interrupt_handler(int signum);
/*******************************************************************************************/

/************************************ Logging Functions ************************************/
void write_in_logfile_start();
void write_in_logfile_stopped();
void write_in_logfile_resume();
void write_in_logfile_finished();
void write_in_perffile();
/*******************************************************************************************/

/*************************************** Free Lists ****************************************/
// Memory Management Segment
int memory_size = 1024;

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
/*******************************************************************************************/

/************************************ Memory Management ************************************/
bool mergeSegments(int id_of_category, int index);
bool memoryInitialize();
bool memoryAllocate(Process *process);
bool memoryDeallocate(Segment *process);
bool memoryManage(bool isAllocating, Process *process);
void memoryDump(bool isAllocating, Process *process, int size, bool isMergingHappened);
/*******************************************************************************************/


int main(int argc, char *argv[])
{

    FINISH = 0;
    memoryInitialize();

    initClk();
    waitingQ.num_of_nodes = 0;
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, interrupt_handler);

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
        exit(-1);
    }

    /* Create a message buffer between process_generator and scheduler */
    key = ftok("key.txt", 66);
    msg_id = msgget(key, (IPC_CREAT | 0666));

    if (msg_id == -1)
    {
        perror("Error in create!");
        exit(1);
    }

    total_number_of_received_process = 0;
    current_process_id = 0;

   
    if (argc < 3)
    {
        perror("Too few CLA!!");
        return -1;
    }

    algorithm = (argv[2][0] - '0') - 1;
    if (algorithm == 2)
    {
        if (argc < 4)
        {
            perror("Too few CLA!!");
            return -1;
        }
        i = 0;
        Q = atoi(argv[3]);
       
    }
    printf("\nQ at beginning: %d\n", Q);
    algo = algorithm;

    logFile = fopen("Scheduler.log", "w");
    fprintf(logFile, "#At  time  x  process  y  state  arr  w  total  z  remain  y  wait  k\n"); // should we ingnore this line ?
    fflush(0);
    memorylogFile = fopen("memory.log", "w");
    fprintf(memorylogFile, "#At  time  x  allocated  y  bytes  for   process  z  from  i  to  j\n");
    fflush(0);

    DUMP = fopen("DUMP.log", "w");

    RR_Priority = 0;

    while (pq_isEmpty(&readyQ));
    if (algo == 2)
        RR(Q);

    else if (algo == 0)
        HPF();

    else if (algo == 1)
        SRTN();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    fclose(logFile);
    fclose(DUMP);
    write_in_perffile();
    shmctl(shmid, IPC_RMID, NULL);
    destroyClk(true);
    // return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// from the parent we will run each scheduler each clock cycle

void RR(int quantum)
{

    int pid, pr;
    int clk = getClk();
    int currentQuantum = quantum;
    int remain_beg;
    bool was_empty = false;
    while(total_number_of_processes || !pq_isEmpty(&readyQ))
    {
        printf("\ntotal_number_of_processes: %d\n", total_number_of_processes);
        if(running && *shmRemainingtime == 0 )
                continue;
        if(running)
        {


            current_process_id = running->id;
            
            

            if(running->remainingTime > 0)
            {
                running->remainingTime--;
                *shmRemainingtime = running->remainingTime;
            }

            if(running->remainingTime == 0 )
                continue;

            currentQuantum--;
            
            
            running->cumulativeRunningTime++;

            //to avoid stopping the process after ending the quantum if it is the only process in the system
            if(currentQuantum == 0 && pq_isEmpty(&readyQ))
            {
                
                currentQuantum = quantum ; //+1 as it will continue without loop on the clock to change
            
            }
            
            
            else if(currentQuantum == 0)
            {
                
                running->state = WAITING;

                //send signal stop to this process and insert it back in the ready queue
                running->waiting_start_time = getClk();

                kill(running->pid, SIGSTOP);

  
                RR_Priority++;
                pq_push(&readyQ, running, RR_Priority);

                //termination occur ??

                write_in_logfile_stopped();
                
                running = NULL;

                continue;  //to make the next process begin exactly after the current is stopped
            }
        }
        else{
            
            if(!pq_isEmpty(&readyQ))
            {
                
                running = pq_pop(&readyQ);
                current_process_id = running->id;
                
                currentQuantum = quantum;
                if(running->state == READY)
                {
                    //meaning that it is the first time to be fun on the cpu
                    //inintialize the remaining time
                    *shmRemainingtime = running->burstTime;
                    running->waitingTime = getClk() - running->arrivalTime;
                    pid = fork();
                    if(pid == -1) perror("Error in fork!!");
                    if(pid == 0)
                    {
                        pr = execl("./process.out", "process.out", (char*) NULL);
                        if(pr == -1)
                        {
                            perror("Error in the process fork!\n");
                            exit(0);
                        }
                    }
                    //put it in the Process
                    running->pid = pid;
                    running->state = RUNNING;
                    running->running_start_time = getClk();
                    
                    remain_beg = running->burstTime;
                    write_in_logfile_start();
                }
                else{
                    //wake it up
                    printf("---------------------old remaining : %d\n", *shmRemainingtime);
                    remain_beg = running->remainingTime;
                    *shmRemainingtime = running->remainingTime;
                    kill(running->pid, SIGCONT); //TO ASK
                    printf("---------------------new remaining : %d\n", *shmRemainingtime);
                    running->remainingTime == *shmRemainingtime;
                    running->state = RUNNING;
                    running->running_start_time = getClk();
                    write_in_logfile_resume();
                }
            }
            
        }

        
        updateInformation();
        printf("\nclk = %d   getclk = %d\n", clk, getClk());
        int preClk = clk;
        while (clk == getClk() && running);
     
        clk = getClk();

        


    }
}

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
        printf("HPF is WORKING now!\n");
        fflush(0);
        if(running && *shmRemainingtime == 0 )
                continue;
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

            if(running->remainingTime == 0 )
                continue;
        }
        else if (!pq_isEmpty(&readyQ))
        {
            firstTime = 0;
            ifUpdated = false;
            running = pq_pop(&readyQ);
            current_process_id = running->id;

            
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

        bool cont = false;
        while (clk == getClk() && running)
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
}

void SRTN(void)
{

    fflush(0);
    int clk = getClk();
    int peek;
    int pid, pr;
    while (total_number_of_processes)
    {
        if (running && *shmRemainingtime == 0)
            continue;

        if (ifReceived)
        {
            printf("\ni am here at line 502  --> false ifReceived\n");
            ifReceived = false;
        }

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
            {
                printf("\ni am here at line 578  --> break ifReceived\n");
                break;
            }
    
        clk = getClk();
        if (ifReceived)
        {
            printf("\ni am here at line 583  --> ifReceived\n");
            
            continue;
        }
        
        if (running && running->remainingTime > 0)
        {
            running->cumulativeRunningTime++;
            running->remainingTime--;
            *shmRemainingtime = running->remainingTime;
            printf("\nat time: %d, decrementing running time of process: %d, becomes: %d\n", clk, running->id, running->remainingTime);
        }
        
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
    
    fprintf(logFile, "At  time  %i  process  %i  started  arr  %i  total  %i  remain  %i  wait  %i\n",
            (*running).running_start_time,
            (*running).id,
            (*running).arrivalTime,
            (*running).burstTime, // to make sure ?!
            (*running).remainingTime,
            (*running).waitingTime // we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
    if(FINISH != -1)
    {
        total_CPU_idle_time += (*running).running_start_time - FINISH;
        printf("\ntotal_cpu_idle_time: %d\n", total_CPU_idle_time);
    }
}

void write_in_logfile_resume()
{
  
    
    fprintf(logFile, "At  time  %i  process  %i  resumed  arr  %i  total  %i  remain  %i  wait  %i\n",
            running->running_start_time,
            running->id,
            running->arrivalTime,
            running->burstTime, // to make sure ?!
            running->remainingTime,
            running->waitingTime);
    fflush(0);
    FINISH = -1;
}

void write_in_logfile_stopped()
{
    
    
    fprintf(logFile, "At  time  %i  process  %i  stopped  arr  %i  total  %i  remain  %i  wait  %i\n",
            running->waiting_start_time,
            running->id,
            running->arrivalTime,
            running->burstTime, // to make sure ?!
            running->remainingTime,
            running->waitingTime // we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
    FINISH = -1;
}

void write_in_logfile_finished()
{
    int clk = getClk();

    FINISH = clk;
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

    perfFile = fopen("Scheduler.perf", "w");
    printf("\nGETCLK IS NOW ------------------------------ = %d\n", getClk());
    CPU_utilization = (1 - ((float)total_CPU_idle_time / (getClk()))) * 100.0;
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
{ 
    printf("Process Terminate handler is started!\n");
    fflush(0);
    // TODO
    // implement what the scheduler should do when it gets notifies that a process is finished
    bool isMerged = memoryManage(false, &Process_Table[current_process_id]);

    running->remainingTime = *shmRemainingtime;
    write_in_logfile_finished();
    // scheduler should delete its data from the process table
    Process_Table[current_process_id] = idleProcess;
    
    // call the function Terminate_Process
    running = NULL;
    total_number_of_processes--;
    // to ask
    // should we check on the total number of processes and if it equals 0 then terminate the scheduler

   
    Process *process;
    bool canBeAllocated = false;

    PriorityQueue tempQ;
    tempQ.num_of_nodes = 0;
    printf("Debugging at Line 750: WaitingQ Size: %d\n", waitingQ.num_of_nodes);
    fflush(0);
    while (waitingQ.num_of_nodes > 0)
    {
        process = pq_pop(&waitingQ);
        printf("Debugging at Line 750: POPING - WaitingQ Size: %d\n", waitingQ.num_of_nodes);
        fflush(0);
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
                pq_push(&readyQ, &Process_Table[process->id], Process_Table[process->id].remainingTime);
            }
            else if (algo == 2)
            {
                
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
                /* WARNING: This needs change depends on the SRTN algorithm */
                pq_push(&tempQ, process, (*process).remainingTime);
            else if (algo == 2)
                pq_push(&tempQ, process, getClk());
        }
    }

    /* Return all process to the waiting Queue */
    while (tempQ.num_of_nodes > 0)
    {
        process = pq_pop(&tempQ);

        if (algo == 0)
            pq_push(&waitingQ, process, (*process).priority);
        else if (algo == 1)
            /* WARNING: This needs change depends on the SRTN algorithm */
            pq_push(&waitingQ, process, (*process).remainingTime);
        else if (algo == 2)
            pq_push(&waitingQ, process, getClk());
    }
    printf("Process Terminate handler is finished Successfully! ReadyQ Size: %d, WaitingQ Size: %d, TempQ Size: %d\n", readyQ.num_of_nodes, waitingQ.num_of_nodes, tempQ.num_of_nodes);
    fflush(0);
    signal(SIGUSR2, ProcessTerminates);
}

void handler_notify_scheduler_new_process_has_arrived(int signum)
{
    printf("Notification handler is started!\n");
    fflush(0);

    int receiveValue;
    int rc;
    struct msqid_ds buf;
    int num_messages;

    rc = msgctl(msg_id, IPC_STAT, &buf);
    num_messages = buf.msg_qnum;
    for (int i = 0; i < num_messages; i++)
    {
        receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, (!IPC_NOWAIT));

        if (receiveValue != -1)
        {
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
                ifReceived = true;

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
                    pq_push(&readyQ, &Process_Table[temp_process->id], Process_Table[temp_process->id].remainingTime);
                }
                else if (algo == 2)
                {
                    // printf("i am pushing here \n");
                    RR_Priority++;
                    pq_push(&readyQ, &Process_Table[temp_process->id], RR_Priority);
                }
            }
            else
            {
                if (algo == 0)
                    pq_push(&waitingQ, temp_process, (*temp_process).priority);
                else if (algo == 1)
                { /* WARNING: This needs change depends on the SRTN algorithm */
                    pq_push(&waitingQ, temp_process, (*temp_process).remainingTime);
                }
                else if (algo == 2)
                {
                    pq_push(&waitingQ, temp_process, getClk());
                }
            }


            /* Parent is systemd, which means the process_generator is died! */
            if (getppid() == 1)
                process_generator_finished = true;
        }
    }

    printf("Notification handler is finished successfully!\n");
    fflush(0);
    signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
}

bool memoryManage(bool isAllocating, Process *process)
{
    /* New Process Allocation Request */
    bool check, isMergingHappened;
    int size;
    if (isAllocating)
    {
        

        check = memoryAllocate(process);
        if (check)
            size = process->segment->size;
        else
            size = 0;
    }
    else
    {
        int id_of_category = ceil(log(process->segment->size) / log(2));
        size = process->segment->size;
        check = memoryDeallocate(process->segment);
        isMergingHappened = check;
        printf("At time %d freed %d bytes from proccess %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
        fflush(0);
        fprintf(memorylogFile, "At time %d freed %d bytes from proccess %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
        fflush(0);

        memory_size += pow(2, id_of_category);
    }
    memoryDump(isAllocating, process, size, isMergingHappened);
    return check;
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
    /* Case there are no nodes in the list */
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
    memory[id_of_category - 1]->num_of_nodes += 2;
    segmentation(id_of_category - 1, target_of_category, process);
}

bool memoryAllocate(Process *process)
{
    int size = process->sizeNeeded;
    int target_id_of_category = ceil(log(size) / log(2));
    int id_of_category = target_id_of_category;
    bool check = false;

    /* Check of my category, if there are free segments */
    while (id_of_category <= 8)
    {
        check = ll_isEmpty(memory[id_of_category]);

        if (!check)
        {
            segmentation(id_of_category, target_id_of_category, process);

            printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
            fflush(0);
            fprintf(memorylogFile, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), process->segment->size, process->id, process->segment->start_address, process->segment->end_address);
            fflush(0);
            int id_of_category = ceil(log(process->segment->size) / log(2));
            memory_size -= pow(2, id_of_category);
            return true;
        }
        else
            id_of_category++;
    }

    printf("At time %d allocating Process %d is failed!\n", getClk(), process->id);
    fflush(0);
    return false;
}



bool mergeSegments(int id_of_category, int index)
{
    printf("<MERGING> - id_of_category: %d\n", id_of_category);
    fflush(0);
    // Base Case
    if (id_of_category == 8)
    {
        printf("<FINISH MERGING> - id_of_category: %d\n", id_of_category);
        fflush(0);
        if (index == 0)
            return false;

        return true;
    }

    // No chance to merge anything!
    if (memory[id_of_category]->num_of_nodes < 2)
    {
        if (index == 0)
            return false;

        return true;
    }
    else if (memory[id_of_category]->num_of_nodes == 2)
    {
        ll_Node *left = memory[id_of_category]->Head;
        ll_Node *right = left->next;

        int check = (left->data->start_address / left->data->size) % 2;

        if ((left->data->end_address + 1 == right->data->start_address) && (check == 0))
        {
            Segment *newSegment = (Segment *)malloc(sizeof(Segment));
            newSegment->size = left->data->size * 2;
            newSegment->start_address = left->data->start_address;
            newSegment->end_address = right->data->end_address;

            memory[id_of_category]->Head = nullptr;
            free(left);
            free(right);

            memory[id_of_category]->num_of_nodes = 0;

            ll_Node *previous, *walker;

            walker = memory[id_of_category + 1]->Head; // rufaida

            // In case, there is no node in the current list, insert as the head
            if (!walker)
            {
                memory[id_of_category + 1]->Head = ll_newNode(newSegment, nullptr);
                memory[id_of_category + 1]->num_of_nodes++; // rufaida

                return true;
            }

            previous = nullptr;
            ll_Node *newNode;

            /* After merging, we need to search for the best fit position in memory[id+1] to insert the new merged segment! */
            while (walker)
            {
                // Initially the walker stands on the head of the current list
                if (walker->data->start_address > newSegment->end_address)
                {
                    newNode = ll_newNode(newSegment, walker);

                    // In case the right position is the head
                    if (walker == memory[id_of_category + 1]->Head)
                    {
                        memory[id_of_category + 1]->Head = newNode;
                        break;
                    }

                    previous->next = newNode;

                    break;
                }
                else
                {
                    previous = walker;
                    walker = walker->next;
                }
            }

            if (!walker)
            {
                newNode = ll_newNode(newSegment, nullptr);
                previous->next = newNode;
            }

            memory[id_of_category + 1]->num_of_nodes += 1;

            return mergeSegments(id_of_category + 1, index + 1);
        }
    }
    else
    {
        ll_Node *doublePrevious = nullptr;
        ll_Node *previous = memory[id_of_category]->Head;
        ll_Node *walker = memory[id_of_category]->Head->next;
        ll_Node *newNode;

        while (walker)
        {
            int check = (previous->data->start_address / previous->data->size) % 2;
            // Initially the previous stands on the head of the current list
            if ((walker->data->start_address == previous->data->end_address + 1) && (check == 0))
            {
                Segment *newSegment = (Segment *)malloc(sizeof(Segment));
                newSegment->size = previous->data->size * 2;
                newSegment->start_address = previous->data->start_address;
                newSegment->end_address = walker->data->end_address;

                if (doublePrevious == nullptr)
                    memory[id_of_category]->Head = walker->next;
                else
                    doublePrevious->next = walker->next;

                free(previous);
                free(walker);

                memory[id_of_category]->num_of_nodes -= 2;

                newNode = ll_newNode(newSegment, nullptr);

                ll_Node *ptr1 = nullptr;
                ll_Node *ptr2 = memory[id_of_category + 1]->Head;

                // In case, there is no node in the current list, insert as the head
                if (!ptr2)
                {
                    memory[id_of_category + 1]->Head = newNode;
                    memory[id_of_category + 1]->num_of_nodes = 1;

                    return true;
                }

                while (ptr2)
                {
                    // Initially the walker stands on the head of the current list
                    if (ptr2->data->start_address > newNode->data->end_address)
                    {
                        // In case the right position is the head
                        if (ptr2 == memory[id_of_category + 1]->Head)
                        {
                            newNode->next = memory[id_of_category + 1]->Head;
                            memory[id_of_category + 1]->Head = newNode;

                            break;
                        }

                        ptr1->next = newNode;
                        newNode->next = ptr2;

                        break;
                    }
                    else
                    {
                        ptr1 = ptr2;
                        ptr2 = ptr2->next;
                    }
                }

                if (!ptr2)
                    ptr1->next = newNode;

                memory[id_of_category + 1]->num_of_nodes += 1;

                break;
            }
            else
            {
                doublePrevious = previous;
                previous = walker;
                walker = walker->next;
            }
        }

        if (!walker)
        {
            if (index == 0)
                return false;

            return true;
        }

        return mergeSegments(id_of_category + 1, index + 1);
    }
}

bool memoryDeallocate(Segment *segment)
{
    // 25 B -> starting address = 64, Ending Address = 95
    //  I will search for a 32B-segment starting with 96 or ending with 63 (left or right)
    // 96 / 32 = 3 % 2 = 1 -> [Odd]
    // (63+1) / 32 = 2 % 2 = 0 -> Even
    // 128
    // Size:    *32 *16  [16]  *16 16 *32
    // Starting: 0   32  [48]   64 80  96
    // Ending:   31  47  [63]   79 95  127
    // Parity:   E   E   [O]    E  O   O
    // If Even -> Merge with right
    // If Odd -> Merge with left

    int id_of_category = ceil(log(segment->size) / log(2));

    ll_Node *walker = memory[id_of_category]->Head;

    // In case, there is no node in the current list, insert as the head
    if (!walker)
    {
        memory[id_of_category]->Head = ll_newNode(segment, nullptr);
        memory[id_of_category]->num_of_nodes = 1;
        return false; // Indication no merging is happened!
    }

    ll_Node *previous = nullptr;
    ll_Node *newNode;

    while (walker)
    {
        // Initially the walker stands on the head of the current list
        if (walker->data->start_address > segment->end_address)
        {
            newNode = ll_newNode(segment, walker);

            // In case the right position is the head
            if (walker == memory[id_of_category]->Head)
            {
                memory[id_of_category]->Head = newNode;
                break;
            }

            previous->next = newNode;

            break;
        }
        else
        {
            previous = walker;
            walker = walker->next;
        }
    }

    if (!walker)
    {
        newNode = ll_newNode(segment, nullptr);
        previous->next = newNode;
    }

    // [previousPrevious] -> [previous] -> [newNode] -> [walker]
    memory[id_of_category]->num_of_nodes += 1;

    // Can't merge 256B-Segments
    if (id_of_category == 8)
        return false;

    int index = 0;

    return mergeSegments(id_of_category, index);
}

void memoryDump(bool isAllocating, Process *process, int size, bool isMergingHappened)
{
    if (isAllocating)
    {
        fprintf(DUMP, "=> snapshot time : %d <ALLOCATTING %d> [Process %d], with memory size = %d.\n", getClk(), size, process->id, memory_size);
        fflush(0);
    }
    else
    {
        if (isMergingHappened)
        {
            fprintf(DUMP, "=> snapshot time : %d <DEALLOCATTING %d> [Process %d], with memory size = %d <MERGED>\n", getClk(), size, process->id, memory_size);
            fflush(0);
        }
        else
        {
            fprintf(DUMP, "=> snapshot time : %d <DEALLOCATTING %d> [Process %d], with memory size = %d <NOT MERGED>\n", getClk(), size, process->id, memory_size);
            fflush(0);
        }
    }
    fprintf(DUMP, "Total Number of Processes = %d\n", total_number_of_processes);
    fflush(0);
    fprintf(DUMP, "Ready Queue Size = %d\n", readyQ.num_of_nodes);
    fflush(0);
    fprintf(DUMP, "Waiting Queue Size = %d\n", waitingQ.num_of_nodes);
    fflush(0);

    ll_Node *walker = nullptr;
    PriorityQueue tempQ;
    tempQ.num_of_nodes = 0;
    for (int i = 0; i < 9; i++)
    {
        walker = memory[i]->Head;
        fprintf(DUMP, "(%d), size -> (%d) : ", i, memory[i]->num_of_nodes);
        fflush(0);
        if ((walker == nullptr) || (memory[i]->num_of_nodes == 0))
        {
            fprintf(DUMP, "[NULL]\n");
            continue;
        }
        fprintf(DUMP, "< %d > ", walker->data->size);
        fflush(0);
        while (walker)
        {
            Process* tempProcess = (Process*)malloc(sizeof(Process));
            tempProcess->segment = walker->data;
            pq_push(&tempQ, tempProcess, walker->data->start_address);
            fprintf(DUMP, "[ %d | %d ] -> ", walker->data->start_address, walker->data->end_address);
            fflush(0);
            walker = walker->next;
        }
        fprintf(DUMP, "[NULL]\n");
    }

    fprintf(DUMP, "<MEMORY %d>: ", tempQ.num_of_nodes);
    fflush(0);
    Process* tempProcess;
    while (!pq_isEmpty(&tempQ))
    {
        tempProcess = pq_pop(&tempQ);
        fprintf(DUMP, "(%d)[ %d | %d ] -> ", tempProcess->segment->size, tempProcess->segment->start_address, tempProcess->segment->end_address);
        fflush(0);
        free(tempProcess);
    }
    fprintf(DUMP, "[NULL]\n");
    fprintf(DUMP, "##############################################################################################\n");
}


void interrupt_handler(int signum)
{
    shmctl(shmid, IPC_RMID, NULL);
    destroyClk(false);
}

// deallocate is called during termination
// get the size of the segment  --> to know the category
// traverse to find the tween
// if found --> merge  --> and find the right location in the new category
// if not found --> insert in the right location
//

// merge
// recersively ---> from the current category to the largest category 256
// once we become unable to merge any more -----> then