#ifndef GLOBAL_H
#define GLOBAL_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#define INHIBITOR_SIGNAL SIGTSTP /* For the handling of CTRL + Z */
#define ACTIVATE_ATOM_SIGNAL SIGUSR1

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <sys/mman.h>
#include <bits/mman-linux.h>
#include <sys/msg.h>
#include <mqueue.h>
#include <semaphore.h>

/* Config parameters storage in protected shared memory */
struct shm_configs
{
    int ENERGY_DEMAND,
        ENERGY_EXPLODE_THRESHOLD, 
        MAX_N_ATOM,
        MIN_N_ATOM,
        N_ATOMI_INIT,
        STEP_ALIMENTAZIONE, 
        N_NEW_ATOMS,        
        SIM_DURATION,
        STEP_ATTIVATORE; 

        /* TERMINATION_CODE is a value in [0, 1, 2, 3, 4, 5] identifying the cause of termination. 
        See master.h --> print_termination for detail. */
    int TERMINATION_CODE;
};
char *shm_configs_path = "/shm_configs"; 
struct shm_configs *config_shm;          

char *semaphore_shm_configs_path = "/semaphore_shm_configs"; 
sem_t *semaphore_config_shm;


/* Protected shared memory for stats to be printed */
struct shm_statistics
{
    int total_required_activations;
    int last_sec_total_required_activations;
    int total_atoms_divisions;
    int last_sec_total_atoms_divisions;
    int total_freed_energy;
    int last_sec_total_freed_energy;
    int total_used_energy;
    int last_sec_total_used_energy;
    int total_available_energy;
    int last_sec_total_available_energy;
    int total_discharges;
    int last_sec_total_discharges;
    int total_absorbed_energy;
    int last_sec_total_absorbed_energy;
};
char *shm_statistics_path = "/shm_statistics"; 
struct shm_statistics *stats_memory;           

char *semaphore_shm_statistics_path = "/semaphore_shm_statistics"; 
sem_t *semaphore_stats_memory;           


/* Protected shared memory keeping the pids */
#define MAX_ARRAY_LENGTH 20000
struct shm_pids
{
    pid_t atoms_pids_array[MAX_ARRAY_LENGTH]; /* For atoms and their children */
    int free_position_index;
    pid_t pid_attivatore;
    pid_t pid_alimentazione;
    pid_t pid_inibitore;
    pid_t pid_master;
};

char *shm_pids_path = "/shm_pids"; 
struct shm_pids *pids_memory;      

char *semaphore_shm_pids_path = "/semaphore_shm_pids"; 
sem_t *semaphore_pids_memory; 

/* returns the index of the first free position in pids_memory->atoms_pids_array */
int search_free_index(){
    int index = 0; 
    while (index < MAX_ARRAY_LENGTH && pids_memory->atoms_pids_array[index] != 0)
    {
        index++;
    }
    return index;
};

/* Two semaphores synchronizing the start of all children */
char *semaphore_all_ready_path = "/semaphore_all_ready"; 
sem_t *semaphore_all_ready;                             

char *semaphore_all_go_path = "/semaphore_all_go"; 
sem_t *semaphore_all_go;                           


/* Message queue used by atoms, activator and inhibitor to keep track of inhibited divisions */
char *msgq_path = "/message_queue";
mqd_t msgq; 
struct mq_attr mq_attributes;

#endif // GLOBAL_H