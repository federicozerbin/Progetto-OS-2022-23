#ifndef MASTER_H
#define MASTER_H

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

/* Support function for read_config_parameters. It reads user input value [1, 2, 3, 4] and returns it. */
int set_config();

/* Generates a filePath, opens config file at filePath, initializes all config values with those in the file */
void read_config_parameters(const char *filePath);

/* Binary value to handle activation / deactivation of inhibitor. */
int inhibitor_YN;

/* Creates and initialize all the IPC objects to be used in master and children */
void set_ipcs();

/* FD for mmap and truncate */
int shm_statistics_fd; 
int shm_pids_fd;       
int shm_configs_fd;

/* Forks to create N_ATOM_INIT atoms, the activator, alimentatore. Then waits for all of them to be ready */
void create_children_and_wait();

/* Variables for the calculus and consistence of the atomic numer */
int n_atom;       
int n_atom_child; 
struct timespec random_seed;

/* Timer that expires every second. It generates a thread calling the function handle_simulation */
timer_t timer_ID;
struct sigevent sigev;
struct itimerspec timer_spec;

/* Initialize the periodic timer */
void init_timer();

/* Seconds passed since the start. Updated on every timer expiration. */
int sec_count;

/* Checks for eventual terminations, consumes ENERGY_DEMAND energy, calls print_stats every time timer_ID expires (it's called by a new thread). */
void handle_simulation();

/* Prints on stdout the stats stored in the shared memory pointed by *stats_memory */
void print_stats();

/* Resets all variables keeping track of the last second usage to keep them consistent */
void reset_last_sec_var();

/* Sets the handler for SIGTERM, SIGINT, INHIBITOR_SIGNAL */
void set_handlers();

/* Handling SIGTERM, SIGINT, INHIBITOR_SIGNAL */
struct sigaction sigterm_SA, inhib_SA, sigint_SA;
void all_signals_handler(int signal);

/* When terminating, it unlinks (closes) all open IPCs deallocating resources to free and clean up memory */
void unlink_ipcs();

/* When terminating, it disarms and removes the timer to clean up memory */
void remove_timer();

/* Kills all active children and waits for them to terminate */
void kill_children();

/* When terminating, it prints on stdout a last sentence with the cause of termination reading the shared value stored in config_shm->TERMINATION_CODE. 
TERMINATION_CODE is a value in [0, 1, 2, 3, 4, 5] where 
0 = error / unexpected condition, 
1 = timeout by reaching SIM_DURATION seconds 
2 = threshold limit for freed energy has been reached
3 = blackout due to unsufficient available energy
4 = meltdown due to the failure of a fork 
5 = forced termination from the user sending SIGINT
*/
void print_termination();

/* Activates inhibitor whether it's not active yet. */
void activate_inhibitor();

/* Deactivates inhibitor iff it's active. */
void deactivate_inhibitor();

#endif // MASTER_H