#include "headers.h"
#include "priority_queue.h"
#include <string.h>
#include "LinkedList.h"
#define LINE_SIZE 300

int msg_id;
int sem1;

void clearResources(int);
// void handler(int signum){
//     signal(SIGUSR1, handler);
// }

int main(int argc, char * argv[])
{
    #if (DEBUGGING == 1)
    printf("(Process_generator): Debugging mode is ON!\n");
    #endif

    

    /* Create a message buffer between process_generator and scheduler */
    key_t key = ftok("key.txt" ,66);
    msg_id = msgget(key, (IPC_CREAT | 0666) );

    if (msg_id == -1) {
        perror("Error in create!");
        exit(1);
    }
    // union Semun semun;
    // key_t key1 = ftok("key.txt",11); 
    // sem1 = semget(key1, 1, 0666 | IPC_CREAT );

    // if(sem1 == -1)
    // {
    //     perror("Error in create sem");
    //     exit(-1);
    // }
    // semun.val = 0; /intial value of the semaphore, Binary semaphore/
    // if(semctl(sem1,0,SETVAL,semun) == -1)
    // {
    //     perror("Error in semclt");
    //     exit(-1);
    // }

    #if (NOTIFICATION == 1)
    // printf("Notification (Process_generator) : Message Queue ID = %d\n", msg_id);
    #endif

    MsgBuf msgbuf;

    #if (HANDLERS == 1)
    signal(SIGINT, clearResources);
    #endif


/* TODO Initialization */
    // 1. Read the input files.
    int tot_pnum = 0;
    int process[5];
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
        for (i = 1; i < 5; i++)
            process[i] = atoi(strtok(NULL, "\t"));
        for (i = 0; i < 5; i++)
            printf("%d\t", process[i]);
        printf("\n");
        const_p = Process_Constructor(process[0], process[1], process[2],process[3], process[4]);
        pq_push(&processQ, const_p, const_p->arrivalTime);
        tot_pnum++;
    }


    // printf("the length of the queue is : %d \n", pq_getLength(&processQ));

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int clkPid;
    int scdPid;
    char algo;
    char Quantum = '\0';
    char pNum[7];

    sprintf(pNum, "%d", tot_pnum);

    printf("Please, choose scheduling algorithm, enter:\n1.HPF\n2.SRTN\n3.RR\n");
    scanf("%c", &algo);

    if(algo == '3')
    {
        printf("Please, enter Quantum\n");
        scanf("%s", &Quantum);

    }

    //signal(SIGUSR1, handler);
    signal(SIGINT, clearResources);

    // 3. Initiate and create the scheduler and clock processes.




    

        
    scdPid = fork();

    if (scdPid == -1) // I can't fork again
    {
        perror("Error in forking!\n");
    }
    else if (scdPid == 0) // I am an another child
    {
        if(execl("./scheduler.out", "scheduler.out", &pNum, &algo, &Quantum, (char *) NULL) == -1)
        {
            perror("Error in execl for scheduler forking\n");
            // printf("\nerror -------------------------\n\n");
        }
            
    }

    
    
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


    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    //sleep(0.5);

    // To get time use this
    int x = getClk();
    #if (DEBUGGING == 1)
    printf("current time is %d\n", x);
    #endif
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
    
    
    int clk_dummy=-1;
    while(!pq_isEmpty(&processQ))
    {
        
        
        bool if_fill_q = false;
        while(pq_peek(&processQ)->arrivalTime <= getClk()) {
            
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
            msgbuf.sizeNeeded = ptr->sizeNeeded;
            msgbuf.state = READY;
            //printf("\nProcess_generator: I sent!\n");
            int sendvalue = msgsnd(msg_id, &msgbuf, sizeof(msgbuf) - sizeof(int), !(IPC_NOWAIT));
            if (sendvalue == -1)
                printf("Error in sending!\n");
            else{
                    if_fill_q = true;
                    printf("I sent to scheduler a process with arrival = %d\n", msgbuf.arrivalTime);

                }
                if(pq_isEmpty(&processQ)) break;
            
        }
        if(if_fill_q)
        {
             int ifsent = kill(scdPid, SIGUSR1);
            if(ifsent == 0)
            {
                // printf("Child id : %d\n", scdPid);
                printf("I send signal to my child scheduler!\n");

            }

        }
       
           
                // else if(clk_dummy!=getClk())
        //     {
        //          printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$44uped inside upper while$$$$$$$$$$$$$$$$$$$$$$$$$");
        //         up(sem1);
        //         clk_dummy=getClk();
        //     }
    }
   
    while(true)
    {
        // if(clk_dummy!=getClk())
        //     {
        //         printf("u$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4ped inside lower while$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
        //         up(sem1);
        //         clk_dummy=getClk();
        //     }
    };
    // 7. Clear clock resources
    //shmctl(get_shmid(), IPC_RMID, (struct shmid_ds *)0);
    //destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    shmctl(get_shmid(), IPC_RMID, (struct shmid_ds *)0);
    msgctl(msg_id, IPC_RMID, (struct msqid_ds *)0);
//     if(semctl(sem1,0,IPC_RMID,NULL) == -1 )
//    {
//         perror("Error in semclt");
//         exit(-1);
//     }
    signal(SIGINT, clearResources);

    exit(0);
}