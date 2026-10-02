/* Every STEP_ALIMENTAZIONE nsec, it forks to add up N_NEW_ATOMS as fuel. */

#include "../common/global.h"
#include "../alimentazione/alimentazione.h"

int main()
{
    set_handlers();
    set_ipcs();
    init_timer();

    sem_post(semaphore_all_ready);
    sem_wait(semaphore_all_go);

    if (timer_settime(timer_ID, 0, &timer_spec, NULL) == -1)
    {
        perror("ALIMENTAZIONE: timer_settime - could not arm and start the timer");
        raise(SIGTERM);
    }

    while (getppid() != 1)
    {
        ;
    }
    raise(SIGTERM);

    return EXIT_FAILURE; /* Only reaching this line if errors occurred! */

} 

void alimentatore_sig_handler(int signal)
{
    switch (signal)
    {
    case SIGTERM:

        if (timer_delete(timer_ID) == -1)
        {
            perror("ALIMENTAZIONE: timer_delete - could not delete the timer");
            raise(SIGTERM);
        }

        while (wait(NULL) != -1)
            ;
        exit(0);
        break;

    default:
        break;
    }
}

void init_timer()
{
    sigev.sigev_notify = SIGEV_THREAD; 
    sigev.sigev_notify_function = periodic_fuel;
    sigev.sigev_notify_attributes = NULL;
    sigev.sigev_value.sival_ptr = &timer_ID;

    if (timer_create(CLOCK_REALTIME, &sigev, &timer_ID) == -1)
    {
        perror("ALIMENTAZIONE: timer_create - could not create timer.");
        raise(SIGTERM);
    }

    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = config_shm->STEP_ALIMENTAZIONE;
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = config_shm->STEP_ALIMENTAZIONE;
}

void periodic_fuel()
{
    pid_t atomPID;

    for (int i = 0; i < config_shm->N_NEW_ATOMS; i++)
    {
        /* As fuel atoms don't have an atom parent to pass them n_atom, alimentazione will choose a random n_atom for its future child */
        clock_gettime(CLOCK_REALTIME, &random_seed);		
		n_atom = random_seed.tv_nsec % (config_shm->MAX_N_ATOM - 1) + 1;
		char n_atom_string[20];
    	snprintf(n_atom_string, sizeof(n_atom_string), "%d", n_atom);

		atomPID = fork();

		if (atomPID == 0)
		{
            /* passes calculated n_atom to the future child as string parameter */
			char *args[] = {"../atomo/./atomo_executable", n_atom_string, NULL};
			execve(args[0], args, NULL);
			perror("execve for atomo failed");
			exit(EXIT_FAILURE);
		}
        else if (atomPID > 0)
        {
            /* saves pid */
            if (pids_memory->free_position_index < MAX_ARRAY_LENGTH)
            {
                sem_wait(semaphore_pids_memory); 
                pids_memory->atoms_pids_array[search_free_index()] = atomPID;
                pids_memory->free_position_index = search_free_index();
                sem_post(semaphore_pids_memory);
            }
        }
        else
        {   
            /* Reaching up here if fork failed (meltdown) */
            sem_wait(semaphore_config_shm);
            config_shm->TERMINATION_CODE = 4;
            sem_post(semaphore_config_shm);
            kill(pids_memory->pid_master, SIGTERM); 
        }
    }
}

void set_handlers()
{
    sigset_t my_mask;
    sigemptyset(&my_mask);                  
    sigaddset(&my_mask, INHIBITOR_SIGNAL);  
    sigprocmask(SIG_BLOCK, &my_mask, NULL); 

    sigterm_SA.sa_handler = &alimentatore_sig_handler;
    sigemptyset(&sigterm_SA.sa_mask);
    sigterm_SA.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sigterm_SA, NULL) == -1)
    {
        perror("ALIMENTAZIONE: Unable to register alimentatore_sig_handler to handlers list");

        raise(SIGTERM);
    }
}

void set_ipcs()
{   
    /* Opening config shm ... */
    shm_configs_fd = shm_open(shm_configs_path, O_RDWR, 0666);
    if (shm_configs_fd == -1)
        perror("shm_open, couldn't open shm with path /shm_configs");

    if (ftruncate(shm_configs_fd, sizeof(struct shm_configs)) == -1)
        perror("ftruncate, couldn't size memory for shm with path /shm_configs");

    config_shm = mmap(NULL, sizeof(*config_shm),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, shm_configs_fd, 0);
    if (config_shm == MAP_FAILED)
        perror("mmap, couldn't map memory region for shm with path /shm_configs");
    
    /* ... and its mutex */
    semaphore_config_shm = sem_open(semaphore_shm_configs_path, O_CREAT, 0666, 1);
    if (semaphore_config_shm == SEM_FAILED)
    {
        perror("sem_open, couldn't open semaphore_config_shm at path /semaphore_shm_configs_path");
        raise(SIGTERM);
    }

    /* Setting the two semaphores for aligned start */
    semaphore_all_ready = sem_open(semaphore_all_ready_path, 0666, 1);
    if (semaphore_all_ready == SEM_FAILED)
    {
        perror("ALIMENTAZIONE: sem_open, couldn't open semaphore_all_ready at path /semaphore_all_ready");
        raise(SIGTERM);
    }

    semaphore_all_go = sem_open(semaphore_all_go_path, O_CREAT, 0666, 1);
    if (semaphore_all_go == SEM_FAILED)
    {
        perror("ALIMENTAIONE: sem_open, couldn't open semaphore_all_go at path /semaphore_all_go");
        raise(SIGTERM);
    }

    /* Opening the pids' shm ... */
    shm_pids_fd = shm_open(shm_pids_path, O_RDWR, 0666);
    if (shm_pids_fd == -1)
        perror("ALIMENTAZIONE: shm_open, couldn't open shm with path /shm_pids");

    pids_memory = mmap(NULL, sizeof(*pids_memory),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_pids_fd, 0);
    if (pids_memory == MAP_FAILED)
        perror("ALIMENTAZIONE: mmap, couldn't map memory region for shm with path /shm_pids");

    /* ... and its mutex */
    semaphore_pids_memory = sem_open(semaphore_shm_pids_path, 0666, 1);
    if (semaphore_pids_memory == SEM_FAILED)
    {
        perror("ALIMENTAZIONE: sem_open, couldn't open semaphore_atomPids at path /semaphore_shm_pids");
        raise(SIGTERM);
    }
}