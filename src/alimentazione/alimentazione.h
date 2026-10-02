#ifndef ALIMENTAZIONE_H
#define ALIMENTAZIONE_H

/* Handles SIGTERM */
struct sigaction sigterm_SA;
void alimentatore_sig_handler(int signal);

/* FD for mmap and truncate */
int shm_statistics_fd; 
int shm_pids_fd;       
int shm_configs_fd;

/* Sets the handler for SIGTERM and masks INHIBITOR_SIGNAL in the process */
void set_handlers();

/* Sets the IPC objects to be used in the process */
void set_ipcs();

/* Timer for making periodic fuel */
timer_t timer_ID;
struct sigevent sigev;
struct itimerspec timer_spec;
void init_timer();

/* Forks N_NEW_ATOMS times whenever timer_ID expires (it's called by a new thread). */
void periodic_fuel();

/* Variables for the calculus and consistence of the atomic numer */
int n_atom;       
int n_atom_child; 
struct timespec random_seed;

#endif // ALIMENTAZIONE_H