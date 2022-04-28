#include "headers.h"
#include "priority_queue.h"

FILE* logFile, *perfFile;
ALGORITHM algorithm;
PriorityQueue readyQ;
Process* Process_Table;
int current_process_id;
int total_number_of_received_process;

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

int parent(void);
int child(void);

void RR(int quantum);
void HPF(void);
void SRTN(void);

void updateInformation(void);

void Context_Switching_To_Run(int Entry_Number);
void Context_Switching_To_Wait(int Entry_Number);
void Context_Switching_To_Start(int Entry_Number);

void Terminate_Process(int Entry_Number);
void checkProcessArrival(void);

int main(int argc, char * argv[])
{
    #if (DEBUGGING == 1)
    printf("Debugging mode is ON!\n");
    #endif

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
    int msg_id =msgget( key, (IPC_CREAT | 0660) );

    if (msg_id == -1) {
        perror("Error in create!");
        exit(1);
    }
    #if (NOTIFICATION == 1)
    printf("Notification (Scheduler): Message Queue ID = %d\n", msg_id);
    #endif

    MsgBuf msgbuf;

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

    Process process;
    #if (WARNINGS == 1)
    #warning "For now, I used a super loop, but We should change it to be a callback function, called when the scheduler is notified that there is an arrived process!"
    #endif
    for(;;)
    {
        int receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, !(IPC_NOWAIT));
        #if (NOTIFICATION == 1)
        printf("Notification (Scheduler): { \nProcess ID: %d,\nProcessArrival Time: %d\n}\n", msgbuf.id, msgbuf.arrivalTime);
        #endif

        process.id = msgbuf.id;
        process.waitingTime = msgbuf.waitingTime;
        process.remainingTime = msgbuf.remainingTime;
        process.executionTime = msgbuf.executionTime;
        process.priority = msgbuf.priority;
        process.cumulativeRunningTime = msgbuf.cumulativeRunningTime;
        process.waiting_start_time = msgbuf.waiting_start_time;
        process.running_start_time = msgbuf.running_start_time;
        process.arrivalTime = msgbuf.arrivalTime;
        process.state = msgbuf.state;

        if (algorithm == HPF_ALGORITHM)
            pq_push(&readyQ, &process, process.priority);
        else if (algorithm == SRTN_ALGORITHM) { /* WARNING: This needs change depends on the SRTN algorithm */
            #if (WARNINGS == 1)
            #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm."
            #endif
            pq_push(&readyQ, &process, process.priority);
        }
        else if (algorithm == RR_ALGORITHM) {
            #if (WARNINGS == 1)
            #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm."
            #endif
            pq_push(&readyQ, &process, process.priority);
        }
    }
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
    /* Super Loop to keep track the clock */
    int clk = 0;
    for(;;)
    {
        initClk();

        /* To detect the new cycle */
        if(getClk() != clk) {
            printf("I am the child! Time here is: %d\n", clk);
            fflush(0);
            clk = getClk();
            #if (NOTIFICATION == 1)
            printf("Notification (Scheduler): Processes' Information have been updated successfully!\n");
            fflush(0);
            #endif
            // updateInformation();
        }
    }
}

void RR(int quantum)
{
    int pid;
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
                pid = execl(".", "./process.out", (char*) NULL); //rufaida-> not sure
                if(pid == -1)
                {
                    perror("error in fork\n");
                    exit(0);
                }
                //put it in the Process
                Process_Table[p->id].id = pid;
                
                
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

    for(;;) // Super Loop
    {
        while(!pq_isEmpty(&readyQ)) {
            checkProcessArrival();

            Process* p = pq_pop(&readyQ);

            //meaning that it is the first time to be fun on the cpu
            pid = execl(".", "./process.out", (char*) NULL); //rufaida-> not sure
            if(pid == -1)
            {
                perror("Error in the process fork!\n");
                exit(0);
            }
            //put it in the Process
            Process_Table[p->id].id = pid;
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
    int pid;
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

            pid = execl(".", "./process.out", (char*) NULL); //rufaida-> not sure
            if(pid == -1)
            {
                perror("Error in the process fork!\n");
                exit(0);
            }
            //put it in the Process
            Process_Table[current->id].id = pid;

            Context_Switching_To_Start(current->id);

        }
        //when terminates --> set current to NULL.
    } while (1);

}

void updateInformation(void) {
    /* Update information for the currently running process */
    Process_Table[current_process_id].cumulativeRunningTime += 1;

    /* Update information for the waiting processes */
    for(int i = 0; i < total_number_of_received_process; i++)
    {
        if (i == current_process_id)
            continue;

        Process_Table[i].waitingTime += 1;
        Process_Table[i].remainingTime -= 1;
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
    int clk = getClk();
    kill(Process_id,SIGKILL);

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


