#include "headers.h"
#include "priority_queue.h"
#include <string.h>
#define LINE_SIZE 300

void clearResources(int);

int main(int argc, char * argv[])
{
    /* Create a message buffer between process_generator and scheduler */
    key_t key = ftok("key.txt" ,66);
    int msg_id =msgget( key, (IPC_CREAT | 0660) );

    MsgBuf msgbuf;

    #if(DEBUGGING == 1)
    printf("DEBUGGING: Here!");
    #endif
    // WARNING: Don't forget to uncomment the next line
    // signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    int process[4];
    int i;
    FILE * pFile;
    char* line = malloc(LINE_SIZE);
    int parameter;
    Process* const_p; 
    PriorityQueue processQ;

    pFile = fopen("processes.txt", "r");
    while(fgets(line, LINE_SIZE, pFile) != NULL){
        
        if(line[0] == '#'){continue;}
        process[0] = strtol(strtok(line, "\t"), NULL, 10);
        for (i = 1; i < 4; i++)
            process[i] = atoi(strtok(NULL, "\t"));
        for (i = 0; i < 4; i++)
            printf("%d\t", process[i]);
        printf("\n");
        const_p = Process_Constructor(process[0], process[1], process[2],process[3]);
        pq_push(&processQ, const_p, const_p->arrivalTime);
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
    
    while(!pq_isEmpty(&processQ))
    {
        #if(DEBUGGING == 1)
        printf("Clock Now: %d\n", getClk());
        int pid = pq_peek(&processQ)->id;
        int arrivalTime = pq_peek(&processQ)->arrivalTime;
        printf("DEBUGGING: { \ntime: %d,\nProcess ID: %d,\nArrival Time: %d\n}\n", getClk(), pid, arrivalTime);
        #endif

        if(pq_peek(&processQ)->arrivalTime == getClk()) {
            // Send to scheduler
            msgbuf.mtype = 0;
            Process *ptr = pq_pop(&processQ);

            Process process;
            process.id = ptr->id;
            process.waitingTime = ptr->waitingTime;
            process.remainingTime = ptr->remainingTime;
            process.executionTime = ptr->executionTime;
            process.priority = ptr->priority;
            process.cumulativeRunningTime = ptr->cumulativeRunningTime;
            process.waiting_start_time = ptr->waitingTime;
            process.running_start_time = ptr->running_start_time;
            process.arrivalTime = ptr->arrivalTime;

            msgbuf.mprocess = process;
            int sendvalue = msgsnd(msg_id, &msgbuf, sizeof(msgbuf.mprocess), !(IPC_NOWAIT));
        }
    }
    
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}