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
	if (advanced == 0)
		syslog(LOG_NOTICE, "Odebrano sygnal %d", signo);
	
	if (signo == SIGUSR1)
	{
		longjmp(jump, 1);
	}
	else if (signo == SIGUSR2)
	{
		longjmp(jump, 2);
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

char* concat(const char *s1, const char *s2)
{
	char *result = malloc(strlen(s1) + strlen(s2) + 2);
	strcpy(result, s1);
	if (strcmp(s1, "/") != 0)
		strcat(result, "/");
	strcat(result, s2);
	return result;
}

void find_files(char *path, char **file, const int start, const int arg)
{
	struct dirent *entry;
	struct stat statbuf;
	int ret = 1;
	DIR *dir;
	
	if ((dir = opendir(path)) == NULL)
	{
		if (advanced)
			syslog(LOG_NOTICE, "cant open %s", path);
		return;
	}
	
	while ((entry = readdir(dir)) != NULL)
	{
		char *new_path = concat(path, entry->d_name);
		if (lstat(new_path, &statbuf) == -1)
			continue;
		
		if (S_ISDIR(statbuf.st_mode))
		{
			if (!strcmp(".", entry->d_name)
					|| !strcmp("..", entry->d_name))
				continue;
			
			if (advanced)
				syslog(LOG_NOTICE, "Obsluzenie folderu %s", new_path);
			
			find_files(new_path, file, start, arg);
		}
		else if (S_ISREG(statbuf.st_mode))
		{
			if (advanced)
				syslog(LOG_NOTICE, "Obsluzenie pliku %s", new_path);
			int i;
			for (i = start; i < arg; i++)
			{
				if (strstr(entry->d_name, file[i]) != NULL)
					syslog(LOG_NOTICE, "Path: %s Plik: %s Wzorzec: %s\n", path, entry->d_name, file[i]);
			}
		}
		free(new_path);
	}
	
	closedir(dir);
	return;
}

int main(int argc, char **argv)
{
	int sleep_time = 6000000;
	int arguments = 0;
	int forks = 0;
	char dir[20] = "/";
	char log[20] = "Daemon20";
	int f = 0;
	
	if (argc < 2)
	{
		/*
		//advanced = 1;
		openlog("daemon8", LOG_PID, LOG_DAEMON);
		find_files("/", ["bc"], argc);
		closelog();
		*/
		printf("ERROR");
		return 0;
	}

	// Daemon
	int d = daemon(0, 0);

	if (d < 0)
		printf("ERROR");
	
	// Argument handler
	if (argc - arguments >= 3 && strstr(argv[1 + arguments], "-d") != NULL)
	{
		strcpy(dir, argv[2 + arguments]);
		arguments += 2;
	}

	if (argc - arguments >= 3 && strstr(argv[1 + arguments], "-f") != NULL)
	{
		forks = atoi(argv[2 + arguments]);
		arguments += 2;
	}

	if (argc - arguments >= 3 && strstr(argv[1 + arguments], "-l") != NULL)
	{
		strcpy(log, argv[2 + arguments]);
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
		openlog(log, LOG_PID, LOG_DAEMON);
		while(1)
		{
			int j = setjmp(jump);
			if (j == 0 || j == 1)
			{
				if (advanced)
					syslog(LOG_NOTICE, "Obudzenie sie %s", argv[1 + arguments]);
				find_files("/", argv, 1 + arguments, argc - arguments);
			}
			syslog(LOG_NOTICE, "%d", j);
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
