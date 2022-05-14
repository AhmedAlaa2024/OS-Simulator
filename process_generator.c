#include "headers.h"
#include "priority_queue.h"
#include <string.h>
#define LINE_SIZE 300

int msg_id;

void clearResources(int);
void handler(int signum){
    signal(SIGUSR1, handler);
}

int main(int argc, char * argv[])
{
    signal(SIGUSR1, handler);
    signal(SIGINT, clearResources);
    #if (DEBUGGING == 1)
    printf("(Process_generator): Debugging mode is ON!\n");
    #endif

    

    /* Create a message buffer between process_generator and scheduler */
    key_t key = ftok("key.txt" ,66);
    msg_id = msgget( key, (IPC_CREAT | 0660) );

    if (msg_id == -1) {
        perror("Error in create!");
        exit(1);
    }

    #if (NOTIFICATION == 1)
    printf("Notification (Process_generator) : Message Queue ID = %d\n", msg_id);
    #endif

    MsgBuf msgbuf;

    #if (HANDLERS == 1)
    signal(SIGINT, clearResources);
    #endif





    /* TODO Initialization */
    // 1. Read the input files.
    int tot_pnum = 0;
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
        tot_pnum++;
    }



    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int clkPid;
    int scdPid;
    char algo;
    char Quantum;

    printf("Please, choose scheduling algorithm, enter:\n1.HPF\n2.SRTN\n3.RR");
    scanf("%c", &algo);

    if(algo == '3')
    {
        printf("Please, enter Quantum");
        scanf("%s", &Quantum);

    }



    // 3. Initiate and create the scheduler and clock processes.

    
    clkPid = fork();

    if (clkPid == -1) // I can't fork
    {
        perror("Error in forking!\n");
    }
    else if (clkPid == 0) // I am a child
    {
        if( execl("./clk.out", "clk.out", NULL) == -1)
            perror("Error in execl for clk forking\n");
    }


    scdPid = fork();

    if (scdPid == -1) // I can't fork again
    {
        perror("Error in forking!\n");
    }
    else if (scdPid == 0) // I am an another child
    {
        if(execl("./scheduler.out", "scheduler.out", tot_pnum, &algo, &Quantum, NULL) == -1)
            perror("Error in execl for scheduler forking\n");
    }

      
    


    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    #if (DEBUGGING == 1)
    printf("current time is %d\n", x);
    #endif
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
    
    while(!pq_isEmpty(&processQ))
    {
        if(pq_peek(&processQ)->arrivalTime <= getClk()) {
            #if(DEBUGGING == 1)
            int pid = pq_peek(&processQ)->id;
            int arrivalTime = pq_peek(&processQ)->arrivalTime;
            printf("DEBUGGING: { \nClock Now: %d,\nProcess ID: %d,\nArrival Time: %d\n}\n", getClk(), pid, arrivalTime);
            #endif

            // Send to scheduler
            msgbuf.mtype = 7;
            Process *ptr = pq_pop(&processQ);

            msgbuf.id = ptr->id;
            msgbuf.waitingTime = ptr->waitingTime;
            msgbuf.remainingTime = ptr->remainingTime;
            msgbuf.burstTime = ptr->burstTime;
            msgbuf.priority = ptr->priority;
            msgbuf.cumulativeRunningTime = ptr->cumulativeRunningTime;
            msgbuf.waiting_start_time = ptr->waitingTime;
            msgbuf.running_start_time = ptr->running_start_time;
            msgbuf.arrivalTime = ptr->arrivalTime;
            msgbuf.state = READY;
            printf("Process_generator: I sent!\n");
            int sendvalue = msgsnd(msg_id, &msgbuf, sizeof(msgbuf) - sizeof(int), !(IPC_NOWAIT));
            if (sendvalue == -1)
                printf("Error in sending!\n");
            else {
                kill(scdPid, SIGUSR1);
                printf("I send signal to my child scheduler!\n");
            }
        }
    }
    
    // 7. Clear clock resources
    // destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    shmctl(get_shmid(), IPC_RMID, (struct shmid_ds *)0);
    msgctl(msg_id, IPC_RMID, (struct msqid_ds *)0);
    signal(SIGINT, clearResources);
    exit(0);
}