/* Can be activated or deactivated at runtime from master process. When active, it conditions the instructions of atom.c and attivatore.c so that in result some of the atom divisions will be blocked and half of the energy will be absorbed from those succeding instead. */

#include "../common/global.h"
#include "../inibitore/inibitore.h"

int main()
{
    set_handlers();
    set_ipcs();
    blocked_divisions = 0;

    /* Costantly checking for new messages to read */
    while (1)
    {   
        /* saves the recived string in local var msg_buf */
        bytes_read = mq_receive(msgq, msg_buf, sizeof(msg_buf), &priority);
        if (bytes_read == -1)
        {
            handle_receive_while_empty_queue("mq_receive: queue is empty, inhibitor terminates.");
        }

        /* adds termination char to have consistent string */
        msg_buf[bytes_read] = '\0';

        /* checks for the type of the sender ... */
        if (priority == 2) 
        {   
            /* if msg comes from an atom [2], its division succeded. It keeps track of the absorbed energy in the stats. */
            sem_wait(semaphore_stats_memory);
            stats_memory->total_absorbed_energy += atoi(msg_buf);
            stats_memory->last_sec_total_absorbed_energy += atoi(msg_buf);
            sem_post(semaphore_stats_memory);

            printf("LOG inhibitor: %d units of energy have been absorbed\n", atoi(msg_buf));
            fflush(stdout);
        }
        else 
        {
            /* if msg comes from the activator [1], a division was blocked! It keeps track of the blocked_divisions. */
            blocked_divisions = atoi(msg_buf);
            printf("LOG inhibitor: an activation was blocked\n");
            fflush(stdout);
        }
    }
    raise(SIGTERM);

    return EXIT_FAILURE; /* Only reaching this line if errors occurred! */
} 

void set_handlers()
{
    sigterm_SA.sa_handler = &inhibitor_sig_handler;
    sigemptyset(&sigterm_SA.sa_mask);
    sigterm_SA.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sigterm_SA, NULL) == -1)
    {
        perror("Unable to register inhibitor_sig_handler to handlers list");
        raise(SIGTERM);
    }
}

void set_ipcs()
{
    /* Opening the message queue */
    msgq = mq_open(msgq_path, O_RDONLY, 0666, 1);
    if (msgq == -1)
    {
        perror("INIBITORE: msgq, couldn't open msgq");
        raise(SIGTERM);
    }

    /* Opening shm with the stats ... */
    shm_statistics_fd = shm_open(shm_statistics_path, O_RDWR, 0666);
    if (shm_statistics_fd == -1)
        perror("inibitore: shm_open, couldn't open shm with path /shm_statistics");

    stats_memory = mmap(NULL, sizeof(*stats_memory),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, shm_statistics_fd, 0);
    if (stats_memory == MAP_FAILED)
        perror("inibitore: mmap, couldn't map memory region for shm with path /shm_statistics");

    /* ... and its mutex */
    semaphore_stats_memory = sem_open(semaphore_shm_statistics_path, 0666, 1);
    if (semaphore_stats_memory == SEM_FAILED)
    {
        perror("inibitore: sem_open, couldn't open semaphore_stats_memory at path /semaphore_shm_statistics");
        raise(SIGTERM);
    }
}

void inhibitor_sig_handler(int signal)
{
    switch (signal)
    {
    case SIGTERM:
        /* before terminating, inhibitor prints up the total number of blocked_divisions in its runtime. */
        printf("LOG inibitore: totale scissioni bloccate: %d\n", blocked_divisions);
        fflush(stdout);
        exit(0);
        break;

    default:
        break;
    }
}
