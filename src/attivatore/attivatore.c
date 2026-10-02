/* Every STEP_ATTIVATORE nsec sends activation request to a random atom */

#include "../common/global.h"
#include "../attivatore/attivatore.h"

int main()
{
    srand(time(NULL));
    set_handlers();
    set_ipcs();
    init_timer();
    sem_post(semaphore_all_ready);
    sem_wait(semaphore_all_go);

    if (timer_settime(timer_ID, 0, &timer_spec, NULL) == -1)
    {
        perror("ATTIVATORE: timer_settime - could not arm and start the timer");
        raise(SIGTERM);
    }

    while (getppid() != 1)
    {
        ;
    }
    raise(SIGTERM);

    return EXIT_FAILURE; /* Only reaching this line if errors occurred! */
} 

void set_handlers()
{
    sigset_t my_mask;
    sigemptyset(&my_mask);                  
    sigaddset(&my_mask, INHIBITOR_SIGNAL);  
    sigprocmask(SIG_BLOCK, &my_mask, NULL); 

    sigterm_SA.sa_handler = &activator_sig_handler;
    sigemptyset(&sigterm_SA.sa_mask);
    sigterm_SA.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sigterm_SA, NULL) == -1)
    {
        perror("Unable to register activator_sig_handler to handlers list");
        raise(SIGTERM);
    }
}

void set_ipcs()
{
    blocked_div_counter = 0;

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
        perror("ATTIVATORE: sem_open, couldn't open semaphore_all_ready at path /semaphore_all_ready");
        raise(SIGTERM);
    }

    semaphore_all_go = sem_open(semaphore_all_go_path, O_CREAT, 0666, 1);
    if (semaphore_all_go == SEM_FAILED)
    {
        perror("ATTIVATORE: sem_open, couldn't open semaphore_all_go at path /semaphore_all_go");
        raise(SIGTERM);
    }

    /* Opening the pids' shm ... */
    shm_pids_fd = shm_open(shm_pids_path, O_RDWR, 0666);
    if (shm_pids_fd == -1)
        perror("ATTIVATORE: shm_open, couldn't open shm with path /shm_pids");

    pids_memory = mmap(NULL, sizeof(*pids_memory),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_pids_fd, 0);
    if (pids_memory == MAP_FAILED)
        perror("ATTIVATORE: mmap, couldn't map memory region for shm with path /shm_pids");

    /* ... and its mutex */
    semaphore_shm_pids_path = "/semaphore_shm_pids";

    semaphore_pids_memory = sem_open(semaphore_shm_pids_path, 0666, 1);
    if (semaphore_pids_memory == SEM_FAILED)
    {
        perror("ATTIVATORE: sem_open, couldn't open semaphore_atomPids at path /semaphore_shm_pids");
        raise(SIGTERM);
    }

    /* Opening shm with the stats ... */
    shm_statistics_fd = shm_open(shm_statistics_path, O_RDWR, 0666);
    if (shm_statistics_fd == -1)
        perror("ATTIVATORE: shm_open, couldn't open shm with path /shm_statistics");

    stats_memory = mmap(NULL, sizeof(*stats_memory),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, shm_statistics_fd, 0);
    if (stats_memory == MAP_FAILED)
        perror("ATTIVATORE: mmap, couldn't map memory region for shm with path /shm_statistics");

    /* ... and its mutex */
    semaphore_stats_memory = sem_open(semaphore_shm_statistics_path, 0666, 1);
    if (semaphore_stats_memory == SEM_FAILED)
    {
        perror("ATTIVATORE: sem_open, couldn't open semaphore_stats_memory at path /semaphore_shm_statistics");
        raise(SIGTERM);
    }

    /* Opening the message queue */
    msgq = mq_open(msgq_path, O_RDWR, 0666, 1);
    if (msgq == -1)
    {
        perror("ATTIVATORE: msgq, couldn't open msgq");
        raise(SIGTERM);
    }
}

void init_timer()
{
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = request_activation;
    sigev.sigev_notify_attributes = NULL;
    sigev.sigev_value.sival_ptr = &timer_ID;

    if (timer_create(CLOCK_REALTIME, &sigev, &timer_ID) == -1)
    {
        perror("ATTIVATORE: timer_create - could not create timer.");
        raise(SIGTERM);
    }

    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = config_shm->STEP_ATTIVATORE;
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = config_shm->STEP_ATTIVATORE;
}

void request_activation()
{   
    /* if there's no inhibitor active ... */
    if (pids_memory->pid_inibitore == 0)
    {
        /* ... it chooses a random atom from atoms_pids_array ... */
        clock_gettime(CLOCK_REALTIME, &random_seed);		
		target_atom_index = random_seed.tv_nsec % (MAX_ARRAY_LENGTH - 1) + 1;
        target_atom_pid = pids_memory->atoms_pids_array[target_atom_index];

        /* that has to be non-null */
        while (target_atom_pid == 0)
        {   
            clock_gettime(CLOCK_REALTIME, &random_seed);		
            target_atom_index = random_seed.tv_nsec % (MAX_ARRAY_LENGTH - 1) + 1;
            target_atom_pid = pids_memory->atoms_pids_array[target_atom_index];
        }

        /* ... and sends ACTIVATE_ATOM_SIGNAL to it */
        kill(target_atom_pid, ACTIVATE_ATOM_SIGNAL);

        sem_wait(semaphore_stats_memory);
        stats_memory->total_required_activations++;
        stats_memory->last_sec_total_required_activations++;
        sem_post(semaphore_stats_memory);
    }
    else /* if inhibitor is active ... */
    {
        /* ... first it generates a random probability. */
        clock_gettime(CLOCK_REALTIME, &random_seed);
        probability = random_seed.tv_nsec % 500;

        /* Division will only be effective iff probability is a even number. */
        if (probability % 2 == 0)
        {
            /* same as lines 161-180 */
            clock_gettime(CLOCK_REALTIME, &random_seed);		
		    target_atom_index = random_seed.tv_nsec % (MAX_ARRAY_LENGTH - 1) + 1;
            target_atom_pid = pids_memory->atoms_pids_array[target_atom_index];

            while (target_atom_pid == 0)
            {   
                clock_gettime(CLOCK_REALTIME, &random_seed);		
                target_atom_index = random_seed.tv_nsec % (MAX_ARRAY_LENGTH - 1) + 1;
                target_atom_pid = pids_memory->atoms_pids_array[target_atom_index];
            }

            kill(target_atom_pid, ACTIVATE_ATOM_SIGNAL);

            sem_wait(semaphore_stats_memory);
            stats_memory->total_required_activations++;
            stats_memory->last_sec_total_required_activations++;
            sem_post(semaphore_stats_memory);
        }
        else
        {   
            /* if it's odd, it will be count as blocked.*/
            blocked_div_counter++;

            /* blocked_div_counter value will be put in a string (message) ... */
            snprintf(blocked_divcount_msg, sizeof(blocked_divcount_msg), "%d", blocked_div_counter);

            /* ... and it will be sent to inhibitor by msgq so it can keep track */
            if (mq_attributes.mq_curmsgs < mq_attributes.mq_maxmsg)
            {
                mq_send(msgq, blocked_divcount_msg, strlen(blocked_divcount_msg) + 1, 1);
            }
            else
            {
                mq_attributes.mq_maxmsg = +10;
                mq_send(msgq, blocked_divcount_msg, strlen(blocked_divcount_msg) + 1, 1);
            }
        }
    }
}

void activator_sig_handler(int signal)
{
    switch (signal)
    {
    case SIGTERM: 
        if (timer_delete(timer_ID) == -1)
        {
            perror("ATTIVATORE: timer_delete - could not delete the timer");
            raise(SIGTERM);
        }

        exit(0);
        break;

    default:
        break;
    }
}