/*$Author: o-pham $*/
/*$Date: 2016/04/05 05:58:06 $*/
/*$Log: resource.h,v $
 *Revision 1.4  2016/04/05 05:58:06  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/04/03 02:31:52  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/04/02 10:50:51  o-pham
 *Initial revision
 *
 *Revision 1.1  2016/04/01 20:06:58  o-pham
 *Initial revision
 **/

#include <stdbool.h>
#include <unistd.h>

#define YES 1
#define NO 0


struct sembuf WAIT;
struct sembuf SIGNAL;

/******* the resource structure *********/
typedef struct resource_structure {
	//int res_desc;	//resource descriptor number
	int shareable;
	int instance;
	int avail[20], availC[20];
	int req[18], reqC[18];
	int allo[18], alloC[18];
	int need[18], needC[18];
	int max[18], maxC[18];	
} resource_descriptor;

/********** Process Structure *************/
typedef struct process_structure {
	unsigned int waiting_time;
	unsigned int turna_time_s;
	unsigned int turna_time_n;
	unsigned int cpu_util;
} process;


/******* The shared memory structure ******/
typedef struct shared_memory_structure {
	resource_descriptor rs[20];
	process p[18];	
	int p_num; //contain the overall number of process ran
	int process_running; //contains the number of process currently running
	int taken[18]; //boolean array to see if the activity array is taken or no
	int throughput;

	//Logical clock
	struct logical_clock {
		unsigned int sec;
		unsigned int nan;
	} lclock;
	
	int rand_fork_time;

} s_m; 

pid_t r_wait(int *);
void exit_proc(int);
void update_clock(s_m *);
int rand_gen(int,int);
int seminit(int,int,int);
void semwait(int,int);
void semsignal(int,int);
int index_open(s_m *);
bool req_lt_avail(int [], int [], int, int);
bool deadlock(int [], int, int, int [], int []);
void test(int,int,s_m *);
