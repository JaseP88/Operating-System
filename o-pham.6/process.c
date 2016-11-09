//////////////////////////////////////////////////////
/////////////////// PROCESS.C ////////////////////////
//////////////////////////////////////////////////////

#include "header.h"

#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

s_m *shared_struct;

int main(int argc, char *argv[]) {
    
    srand(time(NULL));
    int i;
    
    ///////////// Set up Shared Memory ///////////////////
    //////////////////////////////////////////////////////
    key_t shmkey;
		
    if ((shmkey = ftok("/classes/OS/o-pham/o-pham.6",100)) == (key_t)-1) {
        perror("ftok error");
        exit(1);
    }
		
    int shmID;
    s_m *sm;

    shmID = shmget(shmkey,sizeof(s_m),IPC_CREAT | 0666);
    if (shmID<0) {
        perror("shmget failed");
        exit(1);
    }

    sm = (s_m *)shmat(shmID,(void *)0,0);
    if (sm<(s_m *)(0)) {
        perror("shmat failed");
        exit(1);
    }

    shared_struct = sm;

    ////////////// Set up Semaphores ////////////////////
    /////////////////////////////////////////////////////
    int semID;
    key_t semkey;
		
    if ((semkey = ftok("/classes/OS/o-pham/o-pham.6",101)) == (key_t)-1) {
        perror("ftok error");
        exit(1);
    }
		
    semID = semget(semkey,19,IPC_CREAT | 0666);
    if (semID<0) {
        perror("semget failed");
        exit(1);
    }

    //////////////// Process Initialization ///////////////
    ///////////////////////////////////////////////////////
    int p = atoi(argv[1]);                      //process index 
    int process = sm->process_num;              //current process number
    int delimit = rand_gen(0,32);               //the max page amount for this process
    sm->p[p].page_delimit = delimit;            //set page limit for this process
    
    for(i=0; i<delimit; i++) {                  //upon process creation all.. set all members of a page to 0
        sm->p[p].pg_t[i].action = 0;
		sm->p[p].pg_t[i].val_bit = 0;           //val bit 0 is page not in frame
        sm->p[p].pg_t[i].dirty_bit = 0;         //dirty bit set to 0
        sm->p[p].pg_t[i].ref_counter = 0;   
		sm->p[p].pg_t[i].old = NO;
    } 

    sm->p[p].start_time = sm->lclock.nan_counter;   
	sm->p[p].cpu_util = 0;
	sm->p[p].wait_time = 0;
	sm->p[p].turnaround = 0;

    //sm->p[p].start_time_nan = sm->lclock.nan;
    
    //fprintf(stderr,"process %d page requirement %d forked at %d:%09d\n",sm->process_num,delimit,sm->lclock.sec,sm->lclock.nan);


    //////////////// Start Simulation Loop ////////////////
    ///////////////////////////////////////////////////////
    int ref_index;
    int mem_ref;
    int shouldTerm;
    mem_ref = rand_gen(900,1100);
    
	for(;;) {


        /////////// Checks Memory Reference If Process Should Term /////////
        ////////////////////////////////////////////////////////////////////
        if(mem_ref<=0) {
            shouldTerm = rand_gen(NO,YES);
            if(shouldTerm == YES) {
				//fprintf(stderr,"process %d is done\n",process);
    			for(i=0; i<delimit; i++) { //go through out all pages
        			int del_frame;
					sm->p[p].pg_t[i].action = 0;
        			sm->p[p].pg_t[i].val_bit = 0; //reset valid bit
					sm->p[p].pg_t[i].dirty_bit = 0;
					sm->p[p].pg_t[i].ref_counter = 0;
        			del_frame = sm->p[p].pg_t[i].frame_addr; 
       				clearBit(sm->bit_vector,del_frame); //remove from bit vector
   	 			}
				sm->p[p].page_delimit = 0; // reset the page requirement
    			sm->taken[p] = NO;
    			sm->process_running--;
				
				sm->p[p].end_time = sm->lclock.nan_counter;
				sm->p[p].turnaround = sm->p[p].end_time - sm->p[p].start_time;
				sm->p[p].cpu_util = sm->p[p].turnaround - sm->p[p].wait_time;
				
				sm->throughput++;

				savelog("log",process,sm->p[p].eat,sm->p[p].turnaround,sm->p[p].wait_time,sm->p[p].cpu_util);
				
				sleep(1);
				exit(1);
			}

            else
                mem_ref = rand_gen(900,1100);
        }
    

        ////////// Makes Memory Reference ///////////////
        /////////////////////////////////////////////////
        
        //this is the getpage routine

        sm->p[p].num_of_ref++;
		sm->p[p].mat_start = sm->lclock.nan_counter; //start memory access time

        int pg = rand_gen(0,delimit); //page that is being referenced
        int actn = rand_gen(READ,WRITE); //reading or writting action

		sm->p[p].referencing = YES;
		sm->p[p].page = pg;
		sm->p[p].action = actn;
		//then wait...

		sm->p[p].wait_start = sm->lclock.nan_counter;
        semwait(semID,p); //after making a reference wait;
        sm->p[p].wait_stop = sm->lclock.nan_counter;
		sm->p[p].wait_time += (sm->p[p].wait_stop - sm->p[p].wait_start);

		sync_clock(1,sm);
        mem_ref--;
    }//End of simulation

return 0;
}
