#include "headers.h"
#include "priority_queue.h"


process* Process_Table;


void Context_Switching_To_Run(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state=0;
    Process_Table[Entry_Number].waitingtime +=(getClk()-(Process_Table[Entry_Number].waiting_start_time));
    Process_Table[Entry_Number].running_start_time=getClk(); 
    kill(Process_id,SIGCONT); //continue the stopped process
}
void Context_Switching_To_Wait(int Entry_Number)
{
    int Process_id = Process_Table[Entry_Number].id;
    Process_Table[Entry_Number].state=1;
    Process_Table[Entry_Number].remainingtime-=(getClk()-(Process_Table[Entry_Number].running_start_time));
    Process_Table[Entry_Number].waiting_start_time=getClk();
    kill(Process_id,SIGSTOP); //stopping it to the waiting state
}



int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    
    destroyClk(true);
}
