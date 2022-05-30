#include "headers.h"
#include "priority_queue.h"
#include <string.h>
#include "LinkedList.h"
#define LINE_SIZE 300

int msg_id;
int sem1;
int clkPid;
int scdPid;
void clearResources(int);
// void handler(int signum){
//     signal(SIGUSR1, handler);
// }

int main(int argc, char *argv[])
{
    /* Create a message buffer between process_generator and scheduler */
    key_t key = ftok("key.txt", 66);
    msg_id = msgget(key, (IPC_CREAT | 0666));

    if (msg_id == -1)
    {
        perror("Error in create!");
        exit(1);
    }

    MsgBuf msgbuf;

    // signal(SIGINT, clearResources);

    /* TODO Initialization */
    // 1. Read the input files.
    int tot_pnum = 0;
    int process[5];
    int i;
    FILE *pFile;
    char *line = malloc(LINE_SIZE);
    int parameter;
    Process *const_p;
    PriorityQueue processQ;

    pFile = fopen("processes.txt", "r");
    while (fgets(line, LINE_SIZE, pFile) != NULL)
    {

        if (line[0] == '#')
        {
            continue;
        }
        process[0] = strtol(strtok(line, "\t"), NULL, 10);
        for (i = 1; i < 5; i++)
            process[i] = atoi(strtok(NULL, "\t"));
        for (i = 0; i < 5; i++)
            printf("%d\t", process[i]);
        printf("\n");
        const_p = Process_Constructor(process[0], process[1], process[2], process[3], process[4]);
        pq_push(&processQ, const_p, const_p->arrivalTime);
        tot_pnum++;
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    char algo[5];
    char Quantum[5];
    char pNum[7];
    int i_algo;
    int i_q;

    sprintf(pNum, "%d", tot_pnum);

    do
    {
        printf("Please, choose scheduling algorithm, enter:\n1.HPF\n2.SRTN\n3.RR\n");
        fgets(algo, sizeof(algo), stdin);
        i_algo = atoi(algo);
    } while (i_algo < 1 || i_algo > 3);

    if (i_algo == 3)
    {
        printf("Please, enter Quantum\n");
        fgets(Quantum, sizeof(Quantum), stdin);
        i_q = atoi(Quantum);
    }

    signal(SIGINT, clearResources);

    // 3. Initiate and create the scheduler and clock processes.

    scdPid = fork();

    if (scdPid == -1) // I can't fork again
        perror("Error in forking!\n");
    else if (scdPid == 0) // I am an another child
        if (execl("./scheduler.out", "scheduler.out", &pNum, &algo, &Quantum, (char *)NULL) == -1)
            perror("Error in execl for scheduler forking\n");

    clkPid = fork();

    if (clkPid == -1) // I can't fork
        perror("Error in forking!\n");
    else if (clkPid == 0) // I am a child
        if (execl("./clk.out", "clk.out", NULL) == -1)
            perror("Error in execl for clk forking\n");

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    int x = getClk();

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.

    int clk_dummy = -1;
    while (!pq_isEmpty(&processQ))
    {
        bool if_fill_q = false;
        while (pq_peek(&processQ)->arrivalTime <= getClk())
        {
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

            int sendvalue = msgsnd(msg_id, &msgbuf, sizeof(msgbuf) - sizeof(int), !(IPC_NOWAIT));
            if (sendvalue == -1)
                printf("Error in sending!\n");
            else
            {
                if_fill_q = true;
                printf("I sent to scheduler a process with arrival = %d\n", msgbuf.arrivalTime);
            }
            if (pq_isEmpty(&processQ))
                break;
        }
        if (if_fill_q)
        {
            int ifsent = kill(scdPid, SIGUSR1);
            if (ifsent == 0)
                printf("I send signal to my child scheduler!\n");
        }
    }

    int stat_loc;
    waitpid(scdPid, &stat_loc, 0);
    destroyClk(true);
    clearResources(0);
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    shmctl(get_shmid(), IPC_RMID, (struct shmid_ds *)0);
    msgctl(msg_id, IPC_RMID, (struct msqid_ds *)0);

    kill(scdPid, SIGKILL);
    destroyClk(true);
    signal(SIGINT, clearResources);

    exit(0);
}