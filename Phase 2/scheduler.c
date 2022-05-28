#include "headers.h"
#include "priority_queue.h"
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>


FILE* logFile, *perfFile;
int algorithm;
int algo;
PriorityQueue readyQ;
Process* Process_Table;
Process* running = NULL;
Process idleProcess;
int shmid;
int remainingtime;
int* shmRemainingtime;
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
float* WTA;

bool ifReceived = false;
bool process_generator_finished = false;

int clk;


int sem;

//union Semun semun;
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
key_t key ;
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

int total_CPU_idle_time=0;
int sem1;

int main(int argc, char * argv[])
{

    
    initClk();
    total_CPU_idle_time=0;
    signal(SIGCHLD, SIG_IGN);

    total_number_of_processes = 0;
    total_CPU_idle_time = 0;
    i = 0;
    while(argv[1][i])
            total_number_of_processes = total_number_of_processes * 10 + (argv[1][i++] - '0');


    Process_Table = malloc(sizeof(Process)* (total_number_of_processes + 1));


   

    signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
    signal(SIGUSR2, ProcessTerminates);
 

    idleProcess.id = 0;
    CPU_utilization = 0;
    Avg_WTA = 0;
    Avg_Waiting = 0;
    Std_WTA = 0;
    total_Waiting = 0;
    total_WTA = 0;
    number_of_terminated_processes = 0;
    WTA = malloc(sizeof(float) * total_number_of_processes);

    //the remainging time of the current running process
    key_id = ftok("key.txt", 65);
    shmid = shmget(key_id, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    shmRemainingtime = (int*)shmat(shmid, (void *)0, 0);
    if (*shmRemainingtime == -1)
    {
        perror("Error in attach in scheduler --------------");
        //exit(-1);
    }




    //semaphore
    // union Semun semun;
    // key_t key1 = ftok("key.txt",11); 
    // sem1 = semget(key1, 1, 0666 | IPC_CREAT );

    // if(sem1 == -1)
    // {
    //     perror("Error in create sem");
    //     exit(-1);
    // }


    /* Create a message buffer between process_generator and scheduler */
    key = ftok("key.txt" ,66);
    msg_id = msgget( key, (IPC_CREAT | 0666) );

    if (msg_id == -1) {
        perror("Error in create!");
        //exit(1);
    }




    total_number_of_received_process = 0;
    current_process_id = 0;
    
    // printf("scheduler id is  : %d\n",getpid());

    // printf("argc: %d\n", argc);
    // printf("argv[1]: %d\n", atoi(argv[1]));
    // printf("argv[2]: %d\n", atoi(argv[2]));
    // printf("argv[3]: %d\n", atoi(argv[3]));



    
    
    if(argc < 3) { perror("Too few CLA!!"); return -1;}

    // switch (argv[2][0])
    // {
        
    algorithm = (argv[2][0] -'0')-1;
      printf(".............%d\n",algorithm) ; 
    if(algorithm == 2){
        //RR_ALGORITHM;
        if(argc < 4) { perror("Too few CLA!!"); return -1;}
        i = 0;
        Q = 0;
        while(argv[3][i])
            Q = Q * 10 + (argv[3][i++] - '0');
        printf("\nquantum: %d\n", Q);
        // break;
        // default:
        // perror("undefined algorithm");
        // return -1;
    }
    algo = algorithm;




    logFile = fopen("Scheduler.log", "w");
    fprintf(logFile, "#At  time  x  process  y  state  arr  w  total  z  remain  y  wait  k\n");//should we ingnore this line ?
    fflush(0);
    

    RR_Priority = 0;

    while(pq_isEmpty(&readyQ)); //to guarantee that once the first process arrives it will begin at once not at the next sec

    total_CPU_idle_time = getClk();

    if(algo== 2)
        RR(Q);
        
    else if(algo== 0)
        HPF();
       

    else if(algo== 1)
        SRTN();
       


    
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.


    fclose(logFile);

    write_in_perffile();

    destroyClk(true);
    // return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RR(int quantum)
{
    //printf("I am RR! \n");
    int pid, pr;
    int clk = getClk();
    //int timeToStop;
    int currentQuantum = quantum;
    int remain_beg;
    bool was_empty = false;
    //printf("----------the quantum %d\n", quantum);
    while(total_number_of_processes)
    {
        printf("###############################################################\n");
        printf("Total Number of processes until now: %d\n", pq_getLength(&readyQ));
        printf("###############################################################\n");

        
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
                //continue;
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
                //running->cumulativeRunningTime++;
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
                    //currentQuantum--;
                    write_in_logfile_start();
                }
                else{
                    //wake it up
                    //*shmRemainingtime = running->remainingTime;
                    printf("---------------------old remaining : %d\n", *shmRemainingtime);
                    remain_beg = running->remainingTime;
                    *shmRemainingtime = running->remainingTime;
                    kill(running->pid, SIGCONT); //TO ASK
                    printf("---------------------new remaining : %d\n", *shmRemainingtime);
                    running->remainingTime == *shmRemainingtime;
                    running->state = RUNNING;
                    running->running_start_time = getClk();
                    //*shmRemainingtime = running->remainingTime;
                    //currentQuantum--;
                    write_in_logfile_resume();
                }
            }
            else
            {
                total_CPU_idle_time++;
            }
            
        }

        
        updateInformation();
        printf("\nclk = %d   getclk = %d\n", clk, getClk());
        while(clk == getClk());  // && !pq_isEmpty(&readyQ) && running
       
        clk = getClk();

        


    }
}
/* Warning: Under development */
void HPF(void)
{
    printf("in hpf.............%d\n",algorithm) ; 
    int pid;
     clk = getClk();
    int pr;

    running = NULL;
    int firstTime = 0;
    bool ifUpdated = true;
    while(total_number_of_processes)
    {
        printf("\ntotal_CPU_idle_time = %d\n", total_CPU_idle_time);

        // printf("###############################################################\n");
        // printf("ReadyQ length until now: %d\n", pq_getLength(&readyQ));
        // printf("###############################################################\n");

        if(running)
        {
            ifUpdated = false;
            current_process_id=running->id;
            
            if(running->remainingTime>0)
            {
                running->remainingTime--;
                *shmRemainingtime=running->remainingTime;
                running->cumulativeRunningTime++;
            }
 
            printf("if ruuning block");
            

        }
        else if(pq_peek(&readyQ))
            {
                firstTime = 0;
                ifUpdated = false;
                running=pq_pop(&readyQ);
                current_process_id=running->id;

                //running->cumulativeRunningTime++;
                if(running->state==READY)
                {
                    *shmRemainingtime=running->burstTime;
                    running->waitingTime = getClk() - running->arrivalTime;
                    pid = fork();
                    if(pid == -1) perror("Error in fork");
                    if(pid ==0)
                    {
                        pr=execl("./process.out","process.out",(char*)NULL);
                        if(pr == -1)
                        {
                            perror("Error in the process fork");
                            exit(0);
                        }
                }
                running->pid=pid;
                running->state=RUNNING;
                running->running_start_time=getClk();
                
                write_in_logfile_start();
                
            }
            printf("if pq peek block");
        }
        else
            total_CPU_idle_time++;
        
        

        bool cont = false;
        while(clk == getClk())
        {
            if(running && running->remainingTime == 0)
            {
                cont = true;
                break;
            }
                
        }
        if(cont)
            continue;
        updateInformation();
        clk=getClk();

        
    }
    //int CPU_utilization=(1-(total_CPU_idle_time/getClk()))*100;
}
void SRTN(void)
{
    printf("heeeeeeeeeeeeeeeeeeeeeeeere");
    fflush(0);
    int clk = getClk();
    int peek;
    int pid, pr;
    while (total_number_of_processes)
    {
        if(running && running->remainingTime == 0)
                    continue;
        
        if(ifReceived) ifReceived = false;

        if (running)
        {
            if(!pq_isEmpty(&readyQ))
            {
                peek = pq_peek(&readyQ)->remainingTime;

                if(peek < running->remainingTime)
                {
                    //switch:
                
                running->state = WAITING;
                //send signal stop to this process and insert it back in the ready queue
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
            if(!pq_isEmpty(&readyQ))
            {
                running = pq_pop(&readyQ);
                current_process_id = running->id;
                *shmRemainingtime = running->remainingTime;

                if (running->state == READY)
                {
                    // Setting initial waiting time
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

                    running->state = RUNNING;
                    running->running_start_time = getClk();

                    running->pid = pid;
                    current_process_id = running->id;

                    write_in_logfile_start();

                }
            if(running->state == WAITING)
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
        while (clk == getClk()) if(ifReceived) break;
        if(ifReceived) continue;
        if(running && running->remainingTime > 0)
        {
            running->cumulativeRunningTime++;
            running->remainingTime--;
            *shmRemainingtime = running->remainingTime;
        }
        if(!running && pq_isEmpty(&readyQ))
            total_CPU_idle_time++;
        clk = getClk();
    }
}
void updateInformation() {

    /* Update information for the waiting processes */
    for(int i = 1; i <= total_number_of_received_process; i++)
    {
        if (i == current_process_id || Process_Table[i].id == idleProcess.id) 
            continue;

        Process_Table[i].waitingTime += 1;
    }
}


//write_in_logfile
void write_in_logfile_start()
{
    
    printf("process pid = %d , process id = %d\n", running->pid, running->id);
    fflush(0);
    printf("At  time  %d  process  %d  started  arr  %d  total  %d  remain  %d  wait  %d\n",
        running->running_start_time,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime   //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
    fprintf(logFile,"At  time  %i  process  %i  started  arr  %i  total  %i  remain  %i  wait  %i\n",
    (*running).running_start_time,
    (*running).id,
    (*running).arrivalTime,
    (*running).burstTime,    //to make sure ?!
    (*running).remainingTime,
    (*running).waitingTime   //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
}

void write_in_logfile_resume()
{
    printf("\nAt  time  %i  process  %i  resumed  arr  %i  total  %i  remain  %i  wait  %i\n",
        running->running_start_time,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime
    );
    fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  resumed  arr  %i  total  %i  remain  %i  wait  %i\n",
        running->running_start_time,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime
    );
    fflush(0);
}

void write_in_logfile_stopped()
{
    printf("\nAt  time  %d  process  %d  stopped  arr  %d  total  %d  remain  %d  wait  %d\n",
        running->waiting_start_time,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime   //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  stopped  arr  %i  total  %i  remain  %i  wait  %i\n",
        running->waiting_start_time,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime   //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes
    );
    fflush(0);
}
void write_in_logfile_finished()
{
    int clk = getClk();
    printf("\nAt  time  %d  process  %d  finished  arr  %d  total  %d  remain  %d  wait  %d  TA  %d  WTA  %f\n",
        clk,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime ,  //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes

        clk - running->arrivalTime,  //finish - arrival
        (float)(clk - running->arrivalTime) / running->burstTime  //to ask (float)
    );
    fflush(0);
    fprintf(logFile, "At  time  %i  process  %i  finished  arr  %i  total  %i  remain  %i  wait  %i  TA  %i  WTA  %.2f\n",
        clk,
        running->id,
        running->arrivalTime,
        running->burstTime,    //to make sure ?!
        running->remainingTime,
        running->waitingTime ,  //we are sure that this variable --> no 2 processes will write on it at the same time as the update info func update it for only the wainting (not running) processes

        clk - running->arrivalTime,  //finish - arrival
        (float)(clk - running->arrivalTime) / running->burstTime  //to ask (float)
    );
    fflush(0);
    WTA[number_of_terminated_processes] = (float)(clk - running->arrivalTime) / running->burstTime;
    total_WTA += WTA[number_of_terminated_processes];
    total_Waiting += running->waitingTime;
    number_of_terminated_processes++;
}
// void write_in_perf_file()
// {
//     float CPU_utilization = (getClk() - (total_CPU_idle_time)) / (float)getClk();
// }

void write_in_perffile()
{

    printf("\ntotal_CPU_idle_time = %d\n", total_CPU_idle_time);
    total_CPU_idle_time--;

    perfFile = fopen("Scheduler.perf", "w");
    CPU_utilization = (1-((float)total_CPU_idle_time/getClk()))*100.0;
    fprintf(perfFile, "CPU utilization = %.2f%%\n",CPU_utilization);
    Avg_WTA = total_WTA/total_number_of_received_process;
    fprintf(perfFile, "Avg WTA = %.2f\n",Avg_WTA);
    Avg_Waiting = (float)total_Waiting/total_number_of_received_process;
    fprintf(perfFile, "Avg Waiting = %.2f\n",Avg_Waiting);
    float sum = 0;
    for(int i = 0; i < total_number_of_received_process;i++)
    {
        sum += ((WTA[i]-Avg_WTA)*(WTA[i]-Avg_WTA));
    }
    Std_WTA = sqrt((1/(float)total_number_of_received_process)*sum);
    fprintf(perfFile, "Std WTA = %.2f\n",Std_WTA);
    fflush(0);
    fclose(perfFile);
    return;
}

//handler_notify_scheduler_I_terminated
void ProcessTerminates(int signum)
{//down(sem1);
    printf("\n a process terminates\n");
    //TODO
    //implement what the scheduler should do when it gets notifies that a process is finished
    running->remainingTime = *shmRemainingtime;
    write_in_logfile_finished();
    //scheduler should delete its data from the process table
    Process_Table[current_process_id] = idleProcess;
    //free(Process_Table + running->id);
    //call the function Terminate_Process
    running = NULL;
    total_number_of_processes--;
    //to ask
    //should we check on the total number of processes and if it equals 0 then terminate the scheduler

    printf("\n after process terminates-------------------------\n");
    if_termination = true;
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
    for(int i=0; i<num_messages;i++)
        {
            receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, (!IPC_NOWAIT));
        printf("\nreceiveValue : %d \n", receiveValue);
        

        if(receiveValue != -1)
        {
            printf("\nScehduler: I received!\n");
            fflush(0);
            #if (NOTIFICATION == 1)
            printf("Notification (Scheduler): { \nProcess ID: %d,\nProcessArrival Time: %d\n}\n", msgbuf.id, msgbuf.arrivalTime);
            #endif
            total_number_of_received_process += 1;

            Process_Table[msgbuf.id].id = msgbuf.id;
            Process_Table[msgbuf.id].waitingTime = msgbuf.waitingTime;
            Process_Table[msgbuf.id].remainingTime = msgbuf.remainingTime;
            Process_Table[msgbuf.id].burstTime = msgbuf.burstTime;
            Process_Table[msgbuf.id].priority = msgbuf.priority;
            Process_Table[msgbuf.id].cumulativeRunningTime = msgbuf.cumulativeRunningTime;
            Process_Table[msgbuf.id].waiting_start_time = msgbuf.waiting_start_time;
            Process_Table[msgbuf.id].running_start_time = msgbuf.running_start_time;
            Process_Table[msgbuf.id].arrivalTime = msgbuf.arrivalTime;
            Process_Table[msgbuf.id].state = msgbuf.state;

      

            if (algo == 0)
                pq_push(&readyQ, &Process_Table[msgbuf.id], Process_Table[msgbuf.id].priority);
            else if (algo == 1) { /* WARNING: This needs change depends on the SRTN algorithm */
                #if (WARNINGS == 1)
                #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
                #endif
                pq_push(&readyQ, &Process_Table[msgbuf.id], Process_Table[msgbuf.id].remainingTime);
            }
            else if (algo == 2) {
                #if (WARNINGS == 1)
                #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
                #endif
                printf("i am pushing here \n");
                RR_Priority++;
                pq_push(&readyQ, &Process_Table[msgbuf.id], RR_Priority);
            }


            /* Parent is systemd, which means the process_generator is died! */
            if (getppid() == 1) {
                printf("My father is died!\n");
                fflush(0);
                process_generator_finished = true;
            }
        }
    }

    ifReceived = true;
    
   signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
}