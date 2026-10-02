#ifndef ATTIVATORE_H
#define ATTIVATORE_H

/* FD for mmap and truncate */
int shm_statistics_fd; 
int shm_pids_fd;       
int shm_configs_fd;

/* Sets the handler for SIGTERM and masks INHIBITOR_SIGNAL in the process */
void set_handlers();

/* Sets the IPC objects to be used in the process */
void set_ipcs();

/* index of the array to choose a random pid */
int target_atom_index; 

/* the pid, at target_atom_index, to whom ACTIVATE_ATOM_SIGNAL will be sent */
pid_t target_atom_pid; 

/* Handles SIGTERM */
struct sigaction sigterm_SA;
void activator_sig_handler(int signal);

/* Timer for requesting activation periodically, every STEP_ATTIVATORE nsec */
timer_t timer_ID;
struct sigevent sigev;
struct itimerspec timer_spec;
void init_timer();

/* sends ACTIVATE_ATOM_SIGNAL to random atom whenever timer_ID expires (it's called by a new thread). */
void request_activation();

/* if inhibitor is ON, divisions will happen or not according to this random value */
int probability;
struct timespec random_seed;

/* Variable to keep track of the blocked divisions when inhibitor is ON*/
int blocked_div_counter;

/* String used to send blocked_div_counter value to the inhibitor by msgq */
char blocked_divcount_msg[32];

#endif // ATTIVATORE_H