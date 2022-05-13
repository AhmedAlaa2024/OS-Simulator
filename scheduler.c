#include "headers.h"
#include "priority_queue.h"
#include <string.h>

FILE* logFile, *perfFile;
ALGORITHM algorithm;
PriorityQueue readyQ;
Process* Process_Table;
int* current_process_id;
int* total_number_of_received_process;

bool process_generator_finished = false;

key_t key1;
int shmid1;

key_t key2;
int shmid2;

key_t key3;
int shmid3;

int msg_id;
MsgBuf msgbuf;

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

int parent(void);
int child(void);

void RR(int quantum);
void HPF(void);
void SRTN(void);

void updateInformation();

void Context_Switching_To_Run(int Entry_Number);
void Context_Switching_To_Wait(int Entry_Number);
void Context_Switching_To_Start(int Entry_Number);

void Terminate_Process(int Entry_Number);
void checkProcessArrival(void);

void handler_notify_scheduler_new_process_has_arrived(int signum);

int main(int argc, char * argv[])
{
    #if (DEBUGGING == 1)
    printf("Debugging mode is ON!\n");
    #endif


    key_t key1 = ftok("key.txt" ,77);
    int shmid1 = shmget(key1, 512 * 1024, IPC_CREAT | 0666); // We allocated 512 KB
    Process_Table = (Process*) shmat(shmid1, NULL, 0);

    key_t key2 = ftok("key.txt" ,78);
    int shmid2 = shmget(key2, sizeof(int), IPC_CREAT | 0666); // We allocated 8 Bytes
    total_number_of_received_process = (int*) shmat(shmid2, NULL, 0);

    key_t key3 = ftok("key.txt" ,79);
    int shmid3 = shmget(key3, sizeof(int), IPC_CREAT | 0666); // We allocated 8 Bytes
    current_process_id = (int*) shmat(shmid3, NULL, 0);

    int pid;

    pid = fork();

    if (pid == -1) /* I can't give birth for you! */
    {
        perror("Error in forking!");
    }
    else if (pid == 0) /* Hi, I am the child! */
    {
        child();
    }
    else /* Hi, I am the parent! */
    { 
        signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
        parent();   
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int parent(void)
{
    initClk();

    /* Create a message buffer between process_generator and scheduler */
    key_t key = ftok("key.txt" ,66);
    msg_id = msgget( key, (IPC_CREAT | 0660) );

    if (msg_id == -1) {
        perror("Error in create!");
        exit(1);
    }
    #if (NOTIFICATION == 1)
    printf("Notification (Scheduler): Message Queue ID = %d\n", msg_id);
    #endif



    #if (DEBUGGING == 1) // To debug the communication between the scheduler module and the process_generator module
    for(;;)
    {
        // printf("Schedule: I am debugging!");
        // fflush(0);
        int receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, !(IPC_NOWAIT));
        printf("DEBUGGING: { \nProcess ID: %d,\nProcessArrival Time: %d\n}\n", msgbuf.id, msgbuf.arrivalTime);
        fflush(0);
    }
    
    return 0;
    #endif

    *total_number_of_received_process = 0;
    *current_process_id = 0;

    #if (WARNINGS == 1)
    #warning "For now, I used a super loop, but We should change it to be a callback function, called when the scheduler is notified that there is an arrived process!"
    #endif
    printf("Algorithm is running!\n");
    while (!pq_isEmpty(&readyQ) || !process_generator_finished)
    {
        if (algorithm == HPF_ALGORITHM)
            pq_push(&readyQ, Process_Table + msgbuf.id, Process_Table[msgbuf.id].priority);
        else if (algorithm == SRTN_ALGORITHM) { /* WARNING: This needs change depends on the SRTN algorithm */
            #if (WARNINGS == 1)
            #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
            #endif
            pq_push(&readyQ, Process_Table + msgbuf.id, Process_Table[msgbuf.id].remainingTime);
        }
        else if (algorithm == RR_ALGORITHM) {
            #if (WARNINGS == 1)
            #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
            #endif
            pq_push(&readyQ, Process_Table + msgbuf.id, 0);
        }

        pq_pop(&readyQ);
    }
    printf("Algorithm is finished!\n");

    shmctl(shmid1, IPC_RMID, (struct shmid_ds *)0);
    shmctl(shmid2, IPC_RMID, (struct shmid_ds *)0);
    shmctl(shmid3, IPC_RMID, (struct shmid_ds *)0);
    
    #if (WARNINGS == 1)
    #warning "Scheduler: I think we should make the logging in periodic maner. I suggest to put it in updateInformation function which is calledback every clock."
    #endif
    /*
    logFile = fopen("Scheduler.log", "w");
    fprintf(logFile, "#At  time  x  process  y  state  arr  w  total  z  remain  y  wait  k\n");//should we ingnore this line ?
    */
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    

    fclose(logFile);
    destroyClk(true);
}

int child(void)
{
    key_t key1 = ftok("key.txt" ,77);
    int shmid1 = shmget(key1, 512 * 1024, IPC_CREAT | 0666); // We allocated 512 KB
    Process_Table = (Process*) shmat(shmid1, NULL, 0);

    key_t key2 = ftok("key.txt" ,78);   
    int shmid2 = shmget(key2, sizeof(int), IPC_CREAT | 0666); // We allocated 8 Bytes
    total_number_of_received_process = (int*) shmat(shmid2, NULL, 0);

    key_t key3 = ftok("key.txt" ,79);
    int shmid3 = shmget(key3, sizeof(int), IPC_CREAT | 0666); // We allocated 8 Bytes
    current_process_id = (int*) shmat(shmid3, NULL, 0);

    /* Super Loop to keep track the clock */
    int clk = 0;
    initClk();
    for(;;)
    {
        /* To detect the new cycle */
        if(getClk() != clk) {
            clk = getClk();
            
            updateInformation();

            #if (NOTIFICATION == 1)
            printf("Notification (Scheduler): Processes' Information have been updated successfully!\n");
            fflush(0);
            #endif
        }
    }
}

void RR(int quantum)
{
    int pid, pr;
    int clk ;//= getClk();
    int timeToStop;
    //loop on the ready  q
    while(1){
        while(!pq_isEmpty(&readyQ))
        {
            checkProcessArrival();

            Process* p = pq_pop(&readyQ);
            
            if(p->executionTime == p->remainingTime)
            {
                //meaning that it is the first time to be fun on the cpu
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
                else
                {
                    //put it in the Process
                    Process_Table[p->id].pid = pid;
                }
                
                
            }

            Context_Switching_To_Run(p->id);

            clk = Process_Table[p->id].running_start_time;

            timeToStop = clk + quantum;

            if(Process_Table[p->id].remainingTime <= quantum)
            {
                timeToStop = clk + Process_Table[p->id].remainingTime;
                //then run it to completion then remove from the system ---> but the schedular doesn't terminate a process --> what should i do ?
                while(getClk() != timeToStop);

                //terminate the process

            }
            else{
                while(timeToStop);
                Context_Switching_To_Wait(p->id);
                //push it again in the readyQ
                pq_push(&readyQ, p, p->priority);
            }
            //give it its parameters
        }

    }

}

/* Warning: Under development */
void HPF(void)
{
    int pid;
    int timeToStop;
    int pr;

    for(;;) // Super Loop
    {
        while(!pq_isEmpty(&readyQ)) {
            checkProcessArrival();

            Process* p = pq_pop(&readyQ);

            //meaning that it is the first time to be fun on the cpu
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
            else
            {
                //put it in the Process
                Process_Table[p->id].pid = pid;
            }
            timeToStop = getClk() + Process_Table[p->id].executionTime;

            Context_Switching_To_Run(p->id);
            /* Not Finished Yet */
        }
    }
}

void SRTN(void)
{
    int clk = -1;
    int peek;
    int pid, pr;
    Process* current = NULL;
    do
    {
        if(getClk() != clk)
        {
            clk = getClk();

            //check if arrived.
            if(pq_isEmpty(&readyQ))continue;
            if(current != NULL)
            {
                peek = pq_peek(&readyQ)->remainingTime;
                if(peek >= current->remainingTime) continue;

            //switch:
                Context_Switching_To_Wait(current->id);
            }
            current = pq_pop(&readyQ);

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
            else
            {
                //put it in the Process
                Process_Table[current->id].pid = pid;
            }

            Context_Switching_To_Start(current->id);

        }
        //when terminates --> set current to NULL.
    } while (1);

}

void updateInformation() {
    /* Update information for the currently running process */
    if (*total_number_of_received_process == 0)
    {
        printf("No received processes yet!\n");
        return;
    }

    // printf("DEBUGGING: { \nClock Now: %d,\nProcess ID: %d,\nArrival Time: %d\n}\n", getClk(), Process_Table[*total_number_of_received_process-1].pid, Process_Table[*total_number_of_received_process-1].arrivalTime);

    Process_Table[*current_process_id].cumulativeRunningTime += 1;
    Process_Table[*current_process_id].remainingTime -= 1;

    /* Update information for the waiting processes */
    for(int i = 0; i < *total_number_of_received_process; i++)
    {
        if (i == *current_process_id)
            continue;

        Process_Table[i].waitingTime += 1;
    }
}

void Context_Switching_To_Run(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state = RUNNING;
    Process_Table[Entry_Number].waitingTime += (getClk()-(Process_Table[Entry_Number].waiting_start_time));
    Process_Table[Entry_Number].running_start_time = getClk(); 
    fprintf(logFile, "At  time  %d  process  %i  resumed  arr  %d  total  %d  remain  %d  wait  %d\n", 
        Process_Table[Entry_Number].running_start_time,
        Entry_Number,
        Process_Table[Entry_Number].arrivalTime,
        Process_Table[Entry_Number].executionTime,    //to make sure ?!
        Process_Table[Entry_Number].remainingTime,
        Process_Table[Entry_Number].waitingTime 
    );
    kill(Process_id,SIGCONT); //continue the stopped process
}

void Context_Switching_To_Wait(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state = WAITING;
    Process_Table[Entry_Number].remainingTime-=(getClk()-(Process_Table[Entry_Number].running_start_time));
    Process_Table[Entry_Number].waiting_start_time=getClk();
    fprintf(logFile, "At  time  %d  process  %i  stopped  arr  %d  total  %d  remain  %d  wait  %d\n", 
        Process_Table[Entry_Number].running_start_time,
        Entry_Number,
        Process_Table[Entry_Number].arrivalTime,
        Process_Table[Entry_Number].executionTime,    //to make sure ?!
        Process_Table[Entry_Number].remainingTime,
        Process_Table[Entry_Number].waitingTime 
    );
    kill(Process_id,SIGSTOP); //stopping it to the waiting state
}

void Context_Switching_To_Start(int Entry_Number)
{
    int pid = fork();
    if(pid == -1)
    {
        perror("error in fork\n");
        exit(0);
    }
    //put it in the Process
    Process_Table[Entry_Number].id = pid;
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state = RUNNING;
    Process_Table[Entry_Number].waitingTime +=(getClk()-(Process_Table[Entry_Number].waiting_start_time));
    Process_Table[Entry_Number].running_start_time=getClk();

    fprintf(logFile, "At  time  %d  process  %i  started  arr  %d  total  %d  remain  %d  wait  %d\n", 
        Process_Table[Entry_Number].running_start_time,
        Entry_Number,
        Process_Table[Entry_Number].arrivalTime,
        Process_Table[Entry_Number].executionTime,    //to make sure ?!
        Process_Table[Entry_Number].remainingTime,
        Process_Table[Entry_Number].waitingTime 
    );
}
//rufaida-> why terminate???
void Terminate_Process(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    free(Process_Table + Entry_Number);

    int clk = getClk(); 

    fprintf(logFile, "At  time  %d  process  %i  finished  arr  %d  total  %d  remain  %d  wait  %d  TA  %d  WTA  %d\n", 
        clk,         //to make sure
        Entry_Number,
        Process_Table[Entry_Number].arrivalTime,
        Process_Table[Entry_Number].executionTime,    //to make sure ?!
        Process_Table[Entry_Number].remainingTime,
        Process_Table[Entry_Number].waitingTime,
        clk - Process_Table[Entry_Number].arrivalTime,
        (clk - Process_Table[Entry_Number].arrivalTime) / Process_Table[Entry_Number].executionTime
    );
}

void checkProcessArrival()
{
    //to implement --> IPC
    //hint --> we have to give this comming process an id = num_of_nodes in the readyQ
    //the id in the readyQ will be from 0 --> total_num_of_processes - 1
    //which differ from the id in the Process --> which will be the id returned from forking

    //Process_Table[Entry_Number].arrivalTime,
}

void handler_notify_scheduler_I_terminated(int signum)
{
    //TODO
    //implement what the scheduler should do when it gets notifies that a process is finished
    //scheduler should delete its data

    //call the function Terminate_Process
}


void handler_notify_scheduler_new_process_has_arrived(int signum)
{
    printf("Scehduler: I received!\n");
    fflush(0);
    int receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, !(IPC_NOWAIT));
    #if (NOTIFICATION == 1)
    printf("Notification (Scheduler): { \nProcess ID: %d,\nProcessArrival Time: %d\n}\n", msgbuf.id, msgbuf.arrivalTime);
    #endif

    *total_number_of_received_process += 1;

    Process_Table[msgbuf.id].id = msgbuf.id;
    Process_Table[msgbuf.id].waitingTime = msgbuf.waitingTime;
    Process_Table[msgbuf.id].remainingTime = msgbuf.remainingTime;
    Process_Table[msgbuf.id].executionTime = msgbuf.executionTime;
    Process_Table[msgbuf.id].priority = msgbuf.priority;
    Process_Table[msgbuf.id].cumulativeRunningTime = msgbuf.cumulativeRunningTime;
    Process_Table[msgbuf.id].waiting_start_time = msgbuf.waiting_start_time;
    Process_Table[msgbuf.id].running_start_time = msgbuf.running_start_time;
    Process_Table[msgbuf.id].arrivalTime = msgbuf.arrivalTime;
    Process_Table[msgbuf.id].state = msgbuf.state;

    /* Parent is systemd, which means the process_generator is died! */
    if (getppid() == 1) {
        printf("My father is died!\n");
        fflush(0);
        process_generator_finished = true;
    }

    signal(SIGUSR1, handler_notify_scheduler_new_process_has_arrived);
}