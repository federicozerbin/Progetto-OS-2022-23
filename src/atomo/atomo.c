/* Every time ACTIVATE_ATOM_SIGNAL from activator process is received, atom forks and generates energy. */

#include "../common/global.h"
#include "../atomo/atomo.h"
#include <sys/prctl.h>

int main(int argc, char *argv[])
{
    set_handlers();
    set_ipcs();

    /* takes n_atom from the parent by execve() parameter */
    n_atom = atoi(argv[1]);
    freed_energy = 0;

    sem_post(semaphore_all_ready);
    sem_wait(semaphore_all_go);

    while (1)
    {
        ;
    }
}

void atom_sig_handler(int signal)
{
    switch (signal)
    {
    case ACTIVATE_ATOM_SIGNAL:
        divide();
        break;

    case SIGTERM:
        while (wait(NULL) != -1)
            ;

        exit(0);
        break;

    default:
        break;
    }
}

void set_handlers()
{
    sigset_t my_mask;
    sigemptyset(&my_mask);
    sigaddset(&my_mask, INHIBITOR_SIGNAL);
    sigprocmask(SIG_BLOCK, &my_mask, NULL);

    sigterm_SA.sa_handler = &atom_sig_handler;
    sigemptyset(&sigterm_SA.sa_mask);
    if (sigaction(SIGTERM, &sigterm_SA, NULL) == -1)
    {
        perror("ATOMO: Unable to register atom_sig_handler to handlers list");
        raise(SIGTERM);
    }

    activation_SA.sa_handler = &atom_sig_handler;
    sigemptyset(&activation_SA.sa_mask);
    activation_SA.sa_flags = SA_RESTART;
    if (sigaction(ACTIVATE_ATOM_SIGNAL, &activation_SA, NULL) == -1)
    {
        perror("ATOMO: Unable to register atom_sig_handler to handlers list");
        raise(SIGTERM);
    }
}

void set_ipcs()
{
    /* Opening the pids' shm ... */
    shm_pids_fd = shm_open(shm_pids_path, O_RDWR, 0666);
    if (shm_pids_fd == -1 && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (shm_pids_fd == -1)
    {
        perror("ATOMO: shm_open, couldn't open shm with path /shm_pids");
    }

    pids_memory = mmap(NULL, sizeof(*pids_memory),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_pids_fd, 0);
    if (pids_memory == MAP_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (pids_memory == MAP_FAILED)
        perror("ATOMO: mmap, couldn't map memory region for shm with path /shm_pids");

    /* ... and its mutex */
    semaphore_shm_pids_path = "/semaphore_shm_pids";
    semaphore_pids_memory = sem_open(semaphore_shm_pids_path, 0666, 1);
    if (semaphore_pids_memory == SEM_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (semaphore_pids_memory == SEM_FAILED)
    {
        perror("ATOMO: sem_open, couldn't open semaphore_atomPids at path /semaphore_shm_pids");
        raise(SIGTERM);
    }

    /* Setting the two semaphores for aligned start */
    semaphore_all_ready = sem_open(semaphore_all_ready_path, 0666, 1);
    if (semaphore_all_ready == SEM_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (semaphore_all_ready == SEM_FAILED)
    {
        perror("ATOMO: sem_open, couldn't open semaphore_all_ready at path /semaphore_all_ready");
        raise(SIGTERM);
    }

    semaphore_all_go = sem_open(semaphore_all_go_path, O_CREAT, 0666, 1);
    if (semaphore_all_go == SEM_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (semaphore_all_go == SEM_FAILED)
    {
        perror("ATOMO: sem_open, couldn't open semaphore_all_go at path /semaphore_all_go");
        raise(SIGTERM);
    }

    /* Opening config shm ... */
    shm_configs_fd = shm_open(shm_configs_path, O_RDWR, 0666);
    if (shm_configs_fd == -1 && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (shm_configs_fd == -1)
        perror("ATOMO: shm_open, couldn't open shm with path /shm_configs");

    if (ftruncate(shm_configs_fd, sizeof(struct shm_configs)) == -1 && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }

    config_shm = mmap(NULL, sizeof(*config_shm),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, shm_configs_fd, 0);
    if (config_shm == MAP_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (config_shm == MAP_FAILED)
        perror("ATOMO: mmap, couldn't map memory region for shm with path /shm_configs");

    /* ... and its mutex */
    semaphore_config_shm = sem_open(semaphore_shm_configs_path, O_CREAT, 0666, 1);
    if (semaphore_config_shm == SEM_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (semaphore_config_shm == SEM_FAILED)
    {
        perror("ATOMO: sem_open, couldn't open semaphore_config_shm at path /semaphore_shm_configs_path");
        raise(SIGTERM);
    }

    /* Opening shm with the stats ... */
    shm_statistics_fd = shm_open(shm_statistics_path, O_RDWR, 0666);
    if (shm_statistics_fd == -1 && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (shm_statistics_fd == -1)
        perror("ATOMO: shm_open, couldn't open shm with path /shm_statistics");

    stats_memory = mmap(NULL, sizeof(*stats_memory),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, shm_statistics_fd, 0);
    if (stats_memory == MAP_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (stats_memory == MAP_FAILED)
        perror("ATOMO: mmap, couldn't map memory region for shm with path /shm_statistics");

    /* ... and its mutex */
    semaphore_stats_memory = sem_open(semaphore_shm_statistics_path, 0666, 1);
    if (semaphore_stats_memory == SEM_FAILED && pids_memory->pid_master == 0)
    {
        raise(SIGTERM);
    }
    else if (semaphore_stats_memory == SEM_FAILED)
    {
        perror("ATOMO: sem_open, couldn't open semaphore_stats_memory at path /semaphore_shm_statistics");
        raise(SIGTERM);
    }

    /* Opening the message queue */
    msgq = mq_open(msgq_path, O_RDWR, 0666, 1);
    if (msgq == -1)
    {
        perror("ATOMO: msgq, couldn't open msgq");
        raise(SIGTERM);
    }
}

void divide()
{
    if (n_atom <= config_shm->MIN_N_ATOM)
    {
        /* If the atom is exhausted it will terminate and discharges will be updated */
        sem_wait(semaphore_stats_memory);
        for (int i = 0; i < MAX_ARRAY_LENGTH; i++)
        {
            if (pids_memory->atoms_pids_array[i] == getpid())
            {
                pids_memory->atoms_pids_array[i] = 0;
            }
        }
        stats_memory->total_discharges++;
        stats_memory->last_sec_total_discharges++;
        sem_post(semaphore_stats_memory);

        /* Unblocks the semaphore just in case so the next dividing won't wait */
        sem_post(semaphore_all_go);

        raise(SIGTERM);
    }
    else
    {
        /* chooses a random n_atom_child for its future child */
        clock_gettime(CLOCK_REALTIME, &random_seed);		
        n_atom_child = random_seed.tv_nsec % (n_atom - 1) + 1;
        char n_atom_child_string[20];
        snprintf(n_atom_child_string, sizeof(n_atom_child_string), "%d", n_atom_child);

        pid_t childPid = fork();

        if (childPid > 0)
        {
            /* saves pid */
            if (pids_memory->free_position_index < MAX_ARRAY_LENGTH)
            {
                sem_wait(semaphore_pids_memory);
                pids_memory->atoms_pids_array[search_free_index()] = childPid;
                pids_memory->free_position_index = search_free_index();
                sem_post(semaphore_pids_memory);
            }

            /* updates its own atomic number */
            n_atom -= n_atom_child;

            /* frees energy */
            freed_energy = n_atom * n_atom_child - max(n_atom, n_atom_child);

            /* before updating the stats, it checks fo inhibitor process */
            if (pids_memory->pid_inibitore != 0)
            {
                /* if inhibitor is active some of the freed energy will not count ... */
                inhibited_energy = freed_energy / 2;
                absorbed_energy = freed_energy - inhibited_energy;

                sem_wait(semaphore_stats_memory);
                stats_memory->total_atoms_divisions++;
                stats_memory->last_sec_total_atoms_divisions++;

                stats_memory->total_freed_energy += inhibited_energy;
                stats_memory->last_sec_total_freed_energy += inhibited_energy;
                stats_memory->total_available_energy += inhibited_energy;
                stats_memory->last_sec_total_available_energy += inhibited_energy;
                sem_post(semaphore_stats_memory);

                /* ... absorbed_energy will be put in a string (message) ... */
                snprintf(absorbed_energy_msg, sizeof(absorbed_energy_msg), "%d", absorbed_energy);

                /* ... and it will be sent to inhibitor so it can update the stats */
                if (mq_attributes.mq_curmsgs < mq_attributes.mq_maxmsg)
                {
                    mq_send(msgq, absorbed_energy_msg, strlen(absorbed_energy_msg) + 1, 2);
                }
                else
                {
                    mq_attributes.mq_maxmsg += 10;
                    mq_send(msgq, absorbed_energy_msg, strlen(absorbed_energy_msg) + 1, 2);
                }
            }
            else
            {
                /* if inhibitor is not active, it will just update the stats */
                sem_wait(semaphore_stats_memory);
                stats_memory->total_atoms_divisions++;
                stats_memory->last_sec_total_atoms_divisions++;

                stats_memory->total_freed_energy += freed_energy;
                stats_memory->last_sec_total_freed_energy += freed_energy;
                stats_memory->total_available_energy += freed_energy;
                stats_memory->last_sec_total_available_energy += freed_energy;
                sem_post(semaphore_stats_memory);
            }
        }
        else if (childPid == 0) 
        {   
            /* passes calculated n_atom to the future child as string parameter */
			char *args[] = {"../atomo/./atomo_executable", n_atom_child_string, NULL};
            execve(args[0], args, NULL);
            perror("execve for atomo failed");
            exit(EXIT_FAILURE);
        }
        else if (childPid < 0)
        {
            /* Reaching up here if fork failed (meltdown) */
            sem_wait(semaphore_config_shm);
            config_shm->TERMINATION_CODE = 4;
            sem_post(semaphore_config_shm);
            kill(pids_memory->pid_master, SIGTERM);
        }

        /* remains active after the fork() */
        while (1)
        {
            ;
        }

    }
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}