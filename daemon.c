#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <string.h>
#include <dirent.h>

int advanced = 0;
int sleep_time = 0;

static void signal_handler(int signo)
{
	if (signo == SIGUSR1)
	{
		//continue;
		printf("SIGNUSR1");
	}
	else if (signo == SIGUSR2)
	{
		sleep(sleep_time);
		printf("SIGNUSR2");
	}
	else
	{
		fprintf(stderr, "Nieoczekiwany sygnal!\n");
		exit (EXIT_FAILURE);
	}
	exit (EXIT_SUCCESS);
}

void printdir(char *dir, char *s)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	
	if ((dp = opendir(dir)) == NULL)
	{
		//fprintf(stderr, "cant %s", dir);
		return;
	}
	
	chdir(dir);
	while ((entry = readdir(dp)) != NULL) 
	{
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode))
		{
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;
			
			//printf("Folder: %s \n", entry->d_name);
			syslog(LOG_NOTICE, "Folder: %s \n", entry->d_name);
			printdir(entry->d_name, s);
		}
		else
		{
			if (strstr(entry->d_name, s) != NULL)
				syslog(LOG_NOTICE, "Plik: %s Wzorzec: %s\n", entry->d_name, s);
				//printf("Plik: %s \n", entry->d_name);
		}
	}
	chdir("..");
	close(dp);
}



int main(int argc, char **argv)
{

		
	int d = daemon(0, 0);
	if (d < 0)
	{
		
	
		
		printf("NO");
	}
	
	if (signal (SIGUSR1, signal_handler) == SIG_ERR)
	{
		fprintf(stderr, "Nie mozna obsluzyc sygnalu SIGUSR1!");
		exit (EXIT_FAILURE);
	}
	if (signal (SIGUSR2, signal_handler) == SIG_ERR)
	{
		fprintf(stderr, "Nie mozna obsluzyc sygnalu SIGUSR2!");
		exit (EXIT_FAILURE);
	}
	
	openlog("test", LOG_PID, LOG_DAEMON);
	
	
	/*
	syslog(LOG_NOTICE, "T1");
	sleep(20);
	syslog(LOG_NOTICE, "T2");
	*/
	
	while(1)
	{
		if (argc < 2)
			printdir("/", "bc");
		else
		{
			int arguments = 0;
			
			if (argv[1 + arguments] == "-v")
			{
				advanced = 1;
				arguments += 1;
			}
			else
				advanced = 2;
			
			if (argv[1 + arguments] == "-t")
			{
				sleep_time = atoi(argv[2 + arguments]);
				arguments += 2;
			}
			else
				sleep_time = 1000;
			
			
			printdir("/", argv[1 + arguments]);
		}
	}
	
	closelog();
	return EXIT_SUCCESS;
	return 0;
}
