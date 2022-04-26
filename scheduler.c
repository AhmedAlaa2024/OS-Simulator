#include "headers.h"
#include "priority_queue.h"

FILE* logFile, *perfFile;
PriorityQueue readyQ;
PCB* Process_Table;
void RR(int quantum);

void Context_Switching_To_Run(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state = running;
    Process_Table[Entry_Number].waitingTime += (getClk()-(Process_Table[Entry_Number].waiting_start_time));
    Process_Table[Entry_Number].running_start_time = getClk(); 
    fprintf(logFile, "At  time  %f  process  %i  resumed  arr  %f  total  %f  remain  %f  wait  %f\n", 
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
    Process_Table[Entry_Number].state = waiting;
    Process_Table[Entry_Number].remainingTime-=(getClk()-(Process_Table[Entry_Number].running_start_time));
    Process_Table[Entry_Number].waiting_start_time=getClk();
    fprintf(logFile, "At  time  %f  process  %i  stopped  arr  %f  total  %f  remain  %f  wait  %f\n", 
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
    //put it in the PCB
    Process_Table[Entry_Number].id = pid;
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state = running;
    Process_Table[Entry_Number].waitingTime +=(getClk()-(Process_Table[Entry_Number].waiting_start_time));
    Process_Table[Entry_Number].running_start_time=getClk();

    fprintf(logFile, "At  time  %f  process  %i  started  arr  %f  total  %f  remain  %f  wait  %f\n", 
        Process_Table[Entry_Number].running_start_time,
        Entry_Number,
        Process_Table[Entry_Number].arrivalTime,
        Process_Table[Entry_Number].executionTime,    //to make sure ?!
        Process_Table[Entry_Number].remainingTime,
        Process_Table[Entry_Number].waitingTime 
    );
    kill(Process_id,SIGCONT); //continue the stopped process
}


void Terminate_Process(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    int clk = getClk();
    kill(Process_id,SIGKILL);

    fprintf(logFile, "At  time  %f  process  %i  finished  arr  %f  total  %f  remain  %f  wait  %f  TA  %f  WTA  %f\n", 
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
    //which differ from the id in the PCB --> which will be the id returned from forking

    //Process_Table[Entry_Number].arrivalTime,
}


int main(int argc, char * argv[])
{
    initClk();
    

    logFile = fopen("Scheduler.log", 'w');
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

            process* p = pq_pop(&readyQ);
            
            if(p->executionTime == p->remainingTime)
            {
                //meaning that it is the first time to be fun on the cpu
                pid = fork();
                if(pid == -1)
                {
                    perror("error in fork\n");
                    exit(0);
                }
                //put it in the PCB
                Process_Table[p->id].id = pid;
                
                
            }

            Context_Switching_To_Run(p->id);

            clk = Process_Table[p->id].running_start_time;

            timeToStop = clk + quantum;

            if(Process_Table[p->id].remainingTime <= quantum)
            {
                timeToStop = clk + Process_Table[p->id].remainingTime;
                //then run it to completion then remove from the system ---> but the schedular doesn't terminate a process --> what should i do ?
                while(getclk() != timeToStop);

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

            process* p = pq_pop(&readyQ);

            //meaning that it is the first time to be fun on the cpu
            pid = fork();
            if(pid == -1)
            {
                perror("Error in the process fork!\n");
                exit(0);
            }
            //put it in the PCB
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


