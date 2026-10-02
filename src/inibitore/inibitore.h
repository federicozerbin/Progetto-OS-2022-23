#ifndef INIBITORE_H
#define INIBITORE_H

/* Sets the handler for SIGTERM and masks INHIBITOR_SIGNAL in the process */
void set_handlers();

/* Handles SIGTERM */
struct sigaction sigterm_SA;
void inhibitor_sig_handler(int signal);

/* Sets the IPC objects to be used in the process */
void set_ipcs();

/* FD for mmap and truncate */
int shm_statistics_fd;
int shm_configs_fd;

/* Local value keeping count of all blocked_divisions */
int blocked_divisions;

/* String used to read integers sent to the inhibitor by msgq from atoms or activator */
char msg_buf[32];

/* Value keeping track of the length of our msg*/
ssize_t bytes_read;

/* Marks the type [1, 2] of sender of the msg. 2 stands for atoms, 1 for activator */
unsigned int priority; 

#define handle_receive_while_empty_queue(msg) \
    do                    \
    {                     \
        perror(msg);      \
        raise(SIGTERM);   \
    } while (0)

#endif // INIBITORE_H