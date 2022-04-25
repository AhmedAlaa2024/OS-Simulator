#include "headers.h"
#include "priority_queue.h"
#include <string.h>
#define LINE_SIZE 300

void clearResources(int);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    int process[4];
    int i;
    FILE * pFile;
    char* line = malloc(LINE_SIZE);
    int parameter;
    struct Process* const_p; 
    struct PriorityQueue* processQ;

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
    
    /*while(!processQ.isEmpty())
    {
        if(processQ.peek().arrival==getclk())
        {
            //send it to scheduler
        }
    }*/
    
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}
