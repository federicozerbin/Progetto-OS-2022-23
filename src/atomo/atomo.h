#ifndef ATOMO_H
#define ATOMO_H

/* Variables for the calculus and consistence of the atomic numer */
int n_atom;       
int n_atom_child; 
struct timespec random_seed;

/* Energy being freed during atoms' division */
int freed_energy; 

/* Support function used to calculate freed energy */
int max(int a, int b);

/* Sets the handler for ACTIVATE_ATOM_SIGNAL, SIGTERM and masks INHIBITOR_SIGNAL */
void set_handlers();

/* Sets the IPC objects to be used in the process */
void set_ipcs();

/* Handles ACTIVATE_ATOM_SIGNAL and SIGTERM */
struct sigaction sigterm_SA, activation_SA;
/* if ACTIVATE_ATOM_SIGNAL is received, divide() function will be called */
void atom_sig_handler(int signal);

/* FD for mmap and truncate */
int shm_statistics_fd; 
int shm_pids_fd;      
int shm_configs_fd;

/* Checks for eventual atom exhaustion and divides the atom by forking */
void divide();

/* Variables to keep track of the difference in freed energy when inhibitor is ON*/
int absorbed_energy, inhibited_energy;

/* String used to send absorbed_energy value to the inhibitor by msgq */
char absorbed_energy_msg[32];

#endif // ATOMO_H