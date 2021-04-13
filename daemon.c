#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <setjmp.h>

#include <string.h>
#include <dirent.h>

jmp_buf jump;

int advanced = 0;

static void signal_handler(int signo)
{
	if (advanced)
		syslog(LOG_NOTICE, "Odebrano sygnal %d", signo);
	
	if (signo == SIGUSR1)
	{
		longjmp(jump, 0);
	}
	else if (signo == SIGUSR2)
	{
		longjmp(jump, 1);
	}
	else
	{
		syslog(LOG_NOTICE, "Nieoczekiwany sygnal!");
		exit (EXIT_FAILURE);
	}
	exit (EXIT_SUCCESS);
}

static void signal_handler_controller(int signo)
{
	if (signo == SIGUSR1)
	{
		kill(-1, signo);
	}
	else if (signo == SIGUSR2)
	{
		kill(-1, signo);
	}
	else
	{
		syslog(LOG_NOTICE, "Nieoczekiwany sygnal!");
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
		syslog(LOG_NOTICE, "cant %s", dir);
		return;
	}
	syslog(LOG_NOTICE, "can %s", dir);
	close(dp);
	if ((dp = opendir(dir)) == NULL)
	{
		//fprintf(stderr, "cant %s", dir);
		syslog(LOG_NOTICE, "cant %s", dir);
		return;
	}
	
	chdir(dir);
	while ((entry = readdir(dp)) != NULL) 
	{
		if (advanced)
			syslog(LOG_NOTICE, "Obsluzenie pliku/folderu %s", entry->d_name);
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode))
		{
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;
			
			//printf("Folder: %s \n", entry->d_name);
			//syslog(LOG_NOTICE, "Folder: %s \n", entry->d_name);
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
	if (argc < 2)
	{
		printdir("/", "bc");
		return 0;
	}

	int d = daemon(0, 0);
	if (d < 0)
		printf("ERROR");
	
	
	
	int sleep_time = 60;
	int arguments = 0;
	int forks = 0;
	
	if (argc - arguments >= 3 && strstr(argv[1 + arguments], "-f") != NULL)
	{
		forks = atoi(argv[2 + arguments]);
		arguments += 2;
	}
	
	if (argc - arguments >= 3 && strstr(argv[1 + arguments], "-t") != NULL)
	{
		sleep_time = atoi(argv[2 + arguments]);
		arguments += 2;
	}
	
	if (argc - arguments >= 2 && strstr(argv[1 + arguments], "-v") != NULL)
	{
		advanced = 1;
		arguments += 1;
	}
	int f = 0;
	if (forks != 0)
	{
		int i;
		f = 1;
		for (i = 0; i < forks; i++)
		{
			if (f != 0)
			{
				f = fork();
			}
		}
	}
	void (*fun_sig_han)(int);
	if (f == 0)
		fun_sig_han = &signal_handler;
	else
		fun_sig_han = &signal_handler_controller;
		
		
	if (signal (SIGUSR1, *fun_sig_han) == SIG_ERR)
	{
		fprintf(stderr, "Nie mozna obsluzyc sygnalu SIGUSR1!");
		exit (EXIT_FAILURE);
	}
	if (signal (SIGUSR2, *fun_sig_han) == SIG_ERR)
	{
		fprintf(stderr, "Nie mozna obsluzyc sygnalu SIGUSR2!");
		exit (EXIT_FAILURE);
	}
	
	
	if (f == 0)
	{
		openlog("test", LOG_PID, LOG_DAEMON);
		while(1)
		{
			if (setjmp(jump) == 0)
			{
				if (advanced)
					syslog(LOG_NOTICE, "Obudzenie sie %s", argv[1 + arguments]);
				printdir("/", argv[1 + arguments]);
			}
			if (advanced)
				syslog(LOG_NOTICE, "Uspienie sie");
			sleep(sleep_time);
		}
		
		closelog();
	}
	else
	{
		while(1);
	}
	
	return EXIT_SUCCESS;
}
