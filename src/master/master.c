/* Handles the simulation of a chain reaction. */
#include "../common/global.h"
#include "../master/master.h"

int main()
{
	printf("Initializing IPC objects ...\n");
	set_ipcs();

	switch (set_config())
	{
	case 1:
		read_config_parameters("../config/config1.txt");
		break;

	case 2:
		read_config_parameters("../config/config2.txt");
		break;

	case 3:
		read_config_parameters("../config/config3.txt");
		break;

	case 4:
		read_config_parameters("../config/config4.txt");
		break;

	default:
		break;
	}

	printf("Initializing signal handlers ...\n");
	set_handlers();

	printf("Digit 1 to start simulation with inhibitor process active, 0 to keep it disabled.\n");
	fscanf(stdin, "%d", &inhibitor_YN);
	if (inhibitor_YN == 1)
	{
		raise(INHIBITOR_SIGNAL);
	}

	printf("Initializing the timer...\n");
	init_timer();

	printf("Creating initial processes ...\n");
	create_children_and_wait();

	printf("All set. Starting chain reaction. \n");
	/* When each child has released the all_Ready semaphore, it waits for the green light by the master ... */
	for (int i = 0; i < MAX_ARRAY_LENGTH; i++)

		/* master gives the green light to all waiting children */
		sem_post(semaphore_all_go);

	/* Starts the timer */
	if (timer_settime(timer_ID, 0, &timer_spec, NULL) == -1)
	{
		perror("MASTER: timer_settime - could not arm and start the timer");
		raise(SIGTERM);
	}

	/* timer will expire periodically until time past reaches SIM_DURATION */
	while (sec_count <= config_shm->SIM_DURATION)
	{
		;
	}
	sem_wait(semaphore_config_shm);
	config_shm->TERMINATION_CODE = 1; /* timeout termination */
	sem_post(semaphore_config_shm);

	raise(SIGTERM);

	return EXIT_FAILURE; /* Only reaching this line if errors occurred! */

} 

void handle_simulation()
{
	sec_count++;

    /* First checks for possible terminations */
	if (stats_memory->total_freed_energy > config_shm->ENERGY_EXPLODE_THRESHOLD) 
	{	
		sem_wait(semaphore_config_shm);
		config_shm->TERMINATION_CODE = 2; /* explode termination */
		sem_post(semaphore_config_shm);
		kill(getpid(), SIGTERM);
		printf("Terminating...");
	}
	else if (stats_memory->total_available_energy < config_shm->ENERGY_DEMAND) 
	{
		printf("Terminating...");
		sem_wait(semaphore_config_shm);
		config_shm->TERMINATION_CODE = 3; /* blackout termination */
		sem_post(semaphore_config_shm);
		kill(getpid(), SIGTERM);
	}
	else 
	{	
		/* Then it consumes ENERGY_DEMAND energy, updates the stats ... */
		sem_wait(semaphore_stats_memory);
		stats_memory->last_sec_total_available_energy -= config_shm->ENERGY_DEMAND;
		stats_memory->total_available_energy -= config_shm->ENERGY_DEMAND;
		stats_memory->total_used_energy += config_shm->ENERGY_DEMAND;
		stats_memory->last_sec_total_used_energy += config_shm->ENERGY_DEMAND;
		sem_post(semaphore_stats_memory);
	}

	/* ... and prints them */
	print_stats();
	reset_last_sec_var();
} 

void reset_last_sec_var()
{
	stats_memory->last_sec_total_required_activations = 0;
	stats_memory->last_sec_total_atoms_divisions = 0;
	stats_memory->last_sec_total_freed_energy = 0;
	stats_memory->last_sec_total_used_energy = 0;
	stats_memory->last_sec_total_available_energy = 0;
	stats_memory->last_sec_total_discharges = 0;
	stats_memory->last_sec_total_absorbed_energy = 0;
}

void create_children_and_wait()
{
	/* Creates N_ATOMI_INIT atoms */

	pid_t atomPID;

	for (int i = 0; i < config_shm->N_ATOMI_INIT; i++) 
	{    
		/* As init atoms don't have an atom parent to pass them n_atom, master will choose a random n_atom for its future child */
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
			/* parent saves child pid */
			sem_wait(semaphore_pids_memory);
			pids_memory->atoms_pids_array[search_free_index()] = atomPID;
            pids_memory->free_position_index = search_free_index();
			sem_post(semaphore_pids_memory);
		}
		else
		{
			/* Reaching up here if fork failed (meltdown) */
			printf("Terminating...");
			sem_wait(semaphore_config_shm);
			config_shm->TERMINATION_CODE = 4;
			sem_post(semaphore_config_shm);
			kill(pids_memory->pid_master, SIGTERM);
		}
	}

	/* Creates activator */
	pid_t attivatorePID;
	attivatorePID = fork();

	if (attivatorePID == 0)
	{
		char *args[] = {"../attivatore/./attivatore_executable", NULL};
		execve(args[0], args, NULL);
		perror("execve for attivatore failed");
		exit(EXIT_FAILURE);
	}
	else if (attivatorePID > 0)
	{
		sem_wait(semaphore_pids_memory);
		pids_memory->pid_attivatore = attivatorePID;
		sem_post(semaphore_pids_memory);
	}
	else
	{
		/* Reaching up here if fork failed (meltdown) */
		printf("Terminating...");
		sem_wait(semaphore_config_shm);
		config_shm->TERMINATION_CODE = 4;
		sem_post(semaphore_config_shm);
		kill(pids_memory->pid_master, SIGTERM);
	}

	/* Creates alimentazione */
	pid_t alimentazionePID;
	alimentazionePID = fork();

	if (alimentazionePID == 0)
	{
		char *args[] = {"../alimentazione/./alimentazione_executable", NULL};
		execve(args[0], args, NULL);
		perror("execve for alimentazione failed");
		exit(EXIT_FAILURE);
	}
	else if (alimentazionePID > 0)
	{
		sem_wait(semaphore_pids_memory);
		pids_memory->pid_alimentazione = alimentazionePID;
		sem_post(semaphore_pids_memory);
	}
	else
	{
		/* Reaching up here if fork failed (meltdown) */
		printf("Terminating...");
		sem_wait(semaphore_config_shm);
		config_shm->TERMINATION_CODE = 4;
		sem_post(semaphore_config_shm);
		kill(pids_memory->pid_master, SIGTERM);
	}

	/* Now it waits for all children to be initialized and ready to execute */
	for (int i = 0; i < config_shm->N_ATOMI_INIT + 2; i++)
	{
		sem_wait(semaphore_all_ready);
	}
} 

void init_timer()
{
	sigev.sigev_notify = SIGEV_THREAD;
	sigev.sigev_notify_function = handle_simulation;
	sigev.sigev_notify_attributes = NULL;
	sigev.sigev_value.sival_ptr = &timer_ID;

	if (timer_create(CLOCK_REALTIME, &sigev, &timer_ID) == -1)
	{
		perror("MASTER: timer_create - could not create timer.");
		raise(SIGTERM);
	}

	timer_spec.it_value.tv_sec = 1;
	timer_spec.it_value.tv_nsec = 0;
	timer_spec.it_interval.tv_sec = 1;
	timer_spec.it_interval.tv_nsec = 0;

	sec_count = 0;
} 

void set_ipcs()
{
	/* Opening config shm ... */
	shm_configs_fd = shm_open(shm_configs_path, O_CREAT | O_RDWR, 0666);
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
	sem_init(semaphore_config_shm, 1, 1);

    /* Setting the two semaphores for aligned start */
	semaphore_all_ready = sem_open(semaphore_all_ready_path, O_CREAT, 0666, 1);
	if (semaphore_all_ready == SEM_FAILED)
	{
		perror("sem_open, couldn't open semaphore_all_ready at path /semaphore_all_ready");
		raise(SIGTERM);
	}
	sem_init(semaphore_all_ready, 1, 0);

	semaphore_all_go = sem_open(semaphore_all_go_path, O_CREAT, 0666, 1);
	if (semaphore_all_go == SEM_FAILED)
	{
		perror("sem_open, couldn't open semaphore_all_go at path /semaphore_all_go");
		raise(SIGTERM);
	}
	sem_init(semaphore_all_go, 1, 1);

	/* Opening shm with the stats ... */
	shm_statistics_fd = shm_open(shm_statistics_path, O_CREAT | O_RDWR, 0666);
	if (shm_statistics_fd == -1)
		perror("shm_open, couldn't open shm with path /shm_statistics");

	if (ftruncate(shm_statistics_fd, sizeof(struct shm_statistics)) == -1)
		perror("ftruncate, couldn't size memory for shm with path /shm_statistics");

	stats_memory = mmap(NULL, sizeof(*stats_memory),
						PROT_READ | PROT_WRITE,
						MAP_SHARED, shm_statistics_fd, 0);
	if (stats_memory == MAP_FAILED)
		perror("mmap, couldn't map memory region for shm with path /shm_statistics");

	/* Initializing the stats stored in the shm just created */
	stats_memory->total_required_activations = 0;
	stats_memory->last_sec_total_required_activations = 0;
	stats_memory->total_atoms_divisions = 0;
	stats_memory->last_sec_total_atoms_divisions = 0;
	stats_memory->total_freed_energy = 0;
	stats_memory->last_sec_total_freed_energy = 0;
	stats_memory->total_used_energy = 0;
	stats_memory->last_sec_total_used_energy = 0;
	stats_memory->total_available_energy = 0;
	stats_memory->last_sec_total_available_energy = 0;
	stats_memory->total_discharges = 0;
	stats_memory->last_sec_total_discharges = 0;
	stats_memory->total_absorbed_energy = 0;
	stats_memory->last_sec_total_absorbed_energy = 0;

	/* Opening its mutex */
	semaphore_stats_memory = sem_open(semaphore_shm_statistics_path, O_CREAT, 0666, 1);
	if (semaphore_stats_memory == SEM_FAILED)
	{
		perror("sem_open, couldn't open semaphore_stats_memory at path /semaphore_shm_statistics");
		raise(SIGTERM);
	}
	sem_init(semaphore_stats_memory, 1, 1);

	/* Opening the pids' shm ... */
	shm_pids_fd = shm_open(shm_pids_path, O_CREAT | O_RDWR, 0666);
	if (shm_pids_fd == -1)
		perror("shm_open, couldn't open shm with path /shm_pids");

	if (ftruncate(shm_pids_fd, sizeof(struct shm_pids)) == -1)
		perror("ftruncate, couldn't size memory for shm with path /shm_pids");

	pids_memory = mmap(NULL, sizeof(*pids_memory),
					   PROT_READ | PROT_WRITE,
					   MAP_SHARED, shm_pids_fd, 0);
	if (pids_memory == MAP_FAILED)
		perror("mmap, couldn't map memory region for shm with path /shm_pids");

	/* Initializing the main pids in the shm just created */
	pids_memory->pid_attivatore = 0;
	pids_memory->pid_alimentazione = 0;
	pids_memory->pid_inibitore = 0;
	pids_memory->pid_master = getpid();

	for (int i = 0; i < MAX_ARRAY_LENGTH; i++)
	{
		pids_memory->atoms_pids_array[i] = 0;
	}

	pids_memory->free_position_index = search_free_index();

	/* Opening its mutex */
	semaphore_pids_memory = sem_open(semaphore_shm_pids_path, O_CREAT, 0666, 1);
	if (semaphore_pids_memory == SEM_FAILED)
	{
		perror("sem_open, couldn't open semaphore_atomPids at path /semaphore_shm_pids");
		raise(SIGTERM);
	}
	sem_init(semaphore_pids_memory, 1, 1);

    /* Opening the message queue for atoms, activator and inhibitor */
	mq_attributes.mq_curmsgs = 0;
	mq_attributes.mq_flags = O_NONBLOCK;
	mq_attributes.mq_maxmsg = 10;
	mq_attributes.mq_msgsize = sizeof(int);

	msgq = mq_open(msgq_path, O_CREAT, 0666, &mq_attributes);
	if (msgq == (mqd_t)-1)
	{
		fprintf(stderr, "mq_open error %d: %s\n", errno, strerror(errno));
		perror("mq_open, couldn't open msgq");
		raise(SIGTERM);
	}

} 

void set_handlers()
{
	sigterm_SA.sa_handler = &all_signals_handler;
	sigemptyset(&sigterm_SA.sa_mask);
	sigterm_SA.sa_flags = SA_RESTART;
	if (sigaction(SIGTERM, &sigterm_SA, NULL) == -1)
	{
		perror("Unable to register all_signals_handler to handlers list");
		raise(SIGTERM);
	}

	inhib_SA.sa_handler = &all_signals_handler;
	sigemptyset(&inhib_SA.sa_mask);
	inhib_SA.sa_flags = SA_RESTART;
	if (sigaction(INHIBITOR_SIGNAL, &inhib_SA, NULL) == -1)
	{
		perror("Unable to register all_signals_handler to handlers list");
		raise(SIGTERM);
	}

	sigint_SA.sa_handler = &all_signals_handler;
	sigemptyset(&sigint_SA.sa_mask);
	sigint_SA.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sigint_SA, NULL) == -1)
	{
		perror("Unable to register all_signals_handler to handlers list");
		raise(SIGTERM);
	}
}

void unlink_ipcs()
{
	sem_unlink(semaphore_all_ready_path);
	sem_unlink(semaphore_all_go_path);

	if (munmap(config_shm, sizeof(config_shm)) == -1)
	{
		perror("munmap couldn't unmap memory region for shm with path /shm_configs");
		raise(SIGTERM);
	}
	shm_unlink(shm_configs_path);
	sem_unlink(semaphore_shm_configs_path);

	if (munmap(stats_memory, sizeof(stats_memory)) == -1)
	{
		perror("munmap couldn't unmap memory region for shm with path /shm_statistics");
		raise(SIGTERM);
	}
	shm_unlink(shm_statistics_path);
	sem_unlink(semaphore_shm_statistics_path);

	if (munmap(pids_memory, sizeof(pids_memory)) == -1)
	{
		perror("munmap couldn't unmap memory region for shm with path /shm_pids");
		raise(SIGTERM);
	}

	shm_unlink(shm_pids_path);
	sem_unlink(semaphore_shm_pids_path);

	mq_unlink(msgq_path);
}

void remove_timer()
{
	if (timer_delete(timer_ID) == -1)
	{
		perror("MASTER: timer_delete - could not delete the timer");
		raise(SIGTERM);
	}
}

void kill_children()
{
	kill(pids_memory->pid_attivatore, SIGTERM);
	kill(pids_memory->pid_alimentazione, SIGTERM);
	if (pids_memory->pid_inibitore > 0)
		kill(pids_memory->pid_inibitore, SIGTERM);

	/*	sends an unpolite SIGKILL to all non-null children listed in pids_memory->atoms_pids_array */
	for (int i = 0; i < MAX_ARRAY_LENGTH; i++)
	{
		if (pids_memory->atoms_pids_array[i] != 0)
		{	
			kill(pids_memory->atoms_pids_array[i], SIGKILL);
			waitpid(pids_memory->atoms_pids_array[i], NULL, 0);
		}
	}
}

void all_signals_handler(int signal)
{
	switch (signal)
	{
	case SIGTERM:
		pids_memory->pid_master = 0;
		remove_timer();
		kill_children();
		print_termination();
		unlink_ipcs();
		exit(0);
		break;

	case SIGINT: 
		pids_memory->pid_master = 0;
		remove_timer();
		kill_children();

		sem_wait(semaphore_config_shm);
		config_shm->TERMINATION_CODE = 5;
		sem_post(semaphore_config_shm);
		print_termination();

		unlink_ipcs();
		exit(0);
		break;

	case INHIBITOR_SIGNAL: /* receiving this signal will turn off or activate inhibitor */
		if (pids_memory->pid_inibitore > 0)
		{
			deactivate_inhibitor();
		}
		else
		{
			activate_inhibitor();
		}
		break;

	default:
		break;
	}
}

int set_config()
{
	int confVal;

	printf("Which config do you want to load? (1, 2, 3, 4) \n");
	fscanf(stdin, "%d", &confVal);

	/* If meltdown config is chosen, it limits the number of possible active processes to be sure a fork fails */
	if (confVal == 4)
	{
		struct rlimit new_proc_limit;
		struct rlimit *newp = NULL;

		new_proc_limit.rlim_cur = 2500;
		new_proc_limit.rlim_max = 2500;
		newp = &new_proc_limit;

		setrlimit(RLIMIT_NPROC, newp);
	}

	return confVal;
}

void read_config_parameters(const char *filePath)
{
	FILE *file = fopen(filePath, "r");
	if (file == NULL)
	{
		printf("Error opening config file.\n");
		return;
	}
	printf("Loading %s \n", filePath);

	char line[256];
	while (fgets(line, sizeof(line), file))
	{
		char key[50];
		char value[50];

		if (sscanf(line, "%49[^:]: %49s", key, value) == 2)
		{
			if (strcmp(key, "ENERGY_DEMAND") == 0)
				config_shm->ENERGY_DEMAND = atoi(value);
			else if (strcmp(key, "ENERGY_EXPLODE_THRESHOLD") == 0)
				config_shm->ENERGY_EXPLODE_THRESHOLD = atoi(value);
			else if (strcmp(key, "MAX_N_ATOM") == 0)
				config_shm->MAX_N_ATOM = atoi(value);
			else if (strcmp(key, "MIN_N_ATOM") == 0)
				config_shm->MIN_N_ATOM = atoi(value);
			else if (strcmp(key, "N_ATOMI_INIT") == 0)
				config_shm->N_ATOMI_INIT = atoi(value);
			else if (strcmp(key, "STEP_ALIMENTAZIONE") == 0)
				config_shm->STEP_ALIMENTAZIONE = atoi(value);
			else if (strcmp(key, "N_NEW_ATOMS") == 0)
				config_shm->N_NEW_ATOMS = atoi(value);
			else if (strcmp(key, "STEP_ATTIVATORE") == 0)
				config_shm->STEP_ATTIVATORE = atoi(value);
			else if (strcmp(key, "SIM_DURATION") == 0)
				config_shm->SIM_DURATION = atoi(value);
		}
	}

	config_shm->TERMINATION_CODE = 0;

	printf("Config parameters values loaded successfully.\n");
	fflush(stdout);
	fclose(file);
}

void print_stats()
{
	printf("\nStats Dump:\n");
	if(stats_memory->total_required_activations < 100)
	printf("total required activations : %d\t\t\tlast second: %d\n", stats_memory->total_required_activations, stats_memory->last_sec_total_required_activations);
	else printf("total required activations : %d\t\tlast second: %d\n", stats_memory->total_required_activations, stats_memory->last_sec_total_required_activations);
	printf("total atoms' divisions : %d\t\t\tlast second: %d\n", stats_memory->total_atoms_divisions, stats_memory->last_sec_total_atoms_divisions);
	printf("total freed energy : %d\t\t\tlast second: %d\n", stats_memory->total_freed_energy, stats_memory->last_sec_total_freed_energy);
	printf("total used energy : %d\t\t\t\tlast second: %d\n", stats_memory->total_used_energy, stats_memory->last_sec_total_used_energy);
	printf("total avaiable energy : %d\t\t\tlast second: %d\n", stats_memory->total_available_energy, stats_memory->last_sec_total_available_energy);
	printf("total discharges : %d\t\t\t\tlast second: %d\n", stats_memory->total_discharges, stats_memory->last_sec_total_discharges);
	printf("total absorbed energy : %d\t\t\tlast second: %d\n", stats_memory->total_absorbed_energy, stats_memory->last_sec_total_absorbed_energy);
	printf("\n");
	fflush(stdout);
}

void print_termination()
{
	switch (config_shm->TERMINATION_CODE)
	{
	case 1:
		printf("\nProgram terminated reaching timeout limit (SIM DURATION).\n");
		break;
	case 2:
		printf("\nProgram terminated due to the explosion of freed energy, it exceeded the imposed threshold limit (EXPLODE).\n");
		break;
	case 3:
		printf("\nProgram terminated because there was unsufficient available energy for the periodic demand (BLACKOUT).\n");
		break;
	case 4:
		printf("\nProgram terminated due to the failure of a fork (MELTDOWN).\n");
		break;
	case 5:
		printf("\nProgram terminated with SIGINT (CTRL + C).\n");
		break;
	default:
		printf("\nProgram terminated due to an error or unexpected condition.\n");
		break;
	}
	printf("\nTerminated.\n");
}

void activate_inhibitor()
{
	printf("\nActivating inhibitor.\n");
	pid_t inibitorePID;
	inibitorePID = fork();
	if (inibitorePID == 0)
	{
		char *args[] = {"../inibitore/./inibitore_executable", NULL};
		execve(args[0], args, NULL);
		perror("execve for inibitore failed");
		exit(EXIT_FAILURE);
	}
	else if (inibitorePID > 0)
	{
		sem_wait(semaphore_pids_memory); 
		pids_memory->pid_inibitore = inibitorePID;
		sem_post(semaphore_pids_memory);
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

void deactivate_inhibitor()
{
	/* terminates the exixting inhibitor process */
	kill(pids_memory->pid_inibitore, SIGTERM); 
	printf("\nDeactivating inhibitor.\n");
	waitpid(pids_memory->pid_inibitore, NULL, 0); 
	pids_memory->pid_inibitore = 0;				  
}