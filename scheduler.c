#include "headers.h"
#include "priority_queue.h"

FILE* logFile, *perfFile;
ALGORITHM algorithm;
PriorityQueue readyQ;
Process* Process_Table;
void RR(int quantum);

/* Systick callback the scheduler.updateInformation()
   1. Increase cummualtive running time for the running process
   2. Increase waiting time for the waited process
   3. Decrease the remaining time
   ---------
   4. Need to fork process (Uncle) to trace the clocks and interrupt the scheduler (Parent) to do the callback
   5. We need the context switching to change the state, kill, print.
   ----------------------------------------------------------------------------------
   Note:
   1. We need to make the receiving operation with notification with no blocking.
   2. Set a handler upon the termination.
   ----------------------------------------------------------------------------------
   ERROR: Clock Mismatch!!
*/

void updateInformation(void) {
    int process;
    
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


int main(int argc, char * argv[])
{
    #if (DEBUGGING == 1)
    printf("Debugging mode is ON!\n");
    #endif

    initClk();

    /* Create a message buffer between process_generator and scheduler */
    key_t key = ftok("key.txt" ,66);
    int msg_id =msgget( key, (IPC_CREAT | 0660) );

    if (msg_id == -1) {
        perror("Error in create!");
        exit(1);
    }
    #if (NOTIFICATION == 1)
    printf("Message Queue ID = %d\n", msg_id);
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
    #warning "For now, I used a super loop, but We should change it to be a callback function, called when the scheduler is notified that there is an arrived process! The warning came from Line 153 in scheduler.c!"
    #endif
    for(;;)
    {
        int receiveValue = msgrcv(msg_id, ADDRESS(msgbuf), sizeof(msgbuf) - sizeof(int), 7, !(IPC_NOWAIT));
        #if (NOTIFICATION == 1)
        printf("Notification: { \nProcess ID: %d,\nProcessArrival Time: %d\n}\n", msgbuf.id, msgbuf.arrivalTime);
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
            #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of SRTN algorithm. The warning came from Line 167 in scheduler.c!"
            #endif
            pq_push(&readyQ, &process, process.priority);
        }
        else if (algorithm == RR_ALGORITHM) {
            #if (WARNINGS == 1)
            #warning "Scheduler: You should decide what will be the priority parameter in the priority queue in case of RR algorithm. The warning came from Line 171 in scheduler.c!"
            #endif
            pq_push(&readyQ, &process, process.priority);
        }
    }
    #if (WARNINGS == 1)
    #warning "Scheduler: I think we should make the logging in periodic maner. I suggest to put it in updateInformation function which is calledback every clock."
    #endif
    logFile = fopen("Scheduler.log", "w");
    fprintf(logFile, "#At  time  x  process  y  state  arr  w  total  z  remain  y  wait  k\n");//should we ingnore this line ?
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    

    fclose(logFile);
    destroyClk(true);
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
                pid = fork();
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
void HPF(void) {
    int pid;
    int timeToStop;

    for(;;) // Super Loop
    {
        while(!pq_isEmpty(&readyQ)) {
            checkProcessArrival();

            Process* p = pq_pop(&readyQ);

            //meaning that it is the first time to be fun on the cpu
            pid = fork();
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


void handler_notify_scheduler_I_terminated(int signum)
{
    //TODO
    //implement what the scheduler should do when it gets notifies that a process is finished
    //scheduler should delete its data

    //call the function Terminate_Process
}


