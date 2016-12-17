
/* structure for condition variable
 * implement shared mem with condition variable
*/
typedef struct data_struct{
	int num_waiting;
	int next_count;
} condition_var;

/* struct the operation named WAIT and SIGNAL */
struct sembuf WAIT;
struct sembuf SIGNAL;

/* contains the semaphores that will be used
 * in the semaphore set 
*/
enum {mutex, next, sem}; // condition sem;

/* function to initialize semaphores in the semaphore set */
int initelement(int,int,int);

/* functions do the necessary semaphore locking wait mechanism */
void Pwait(int);
void Vsignal(int);

/* condition functions in the monitor */
void cwait(int);
void csignal(int);


