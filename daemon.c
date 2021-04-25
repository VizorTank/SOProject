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

char* concat(const char *s1, const char *s2)
{
	char *result = malloc(strlen(s1) + strlen(s2) + 2);
	strcpy(result, s1);
	if (strcmp(s1, "/") != 0)
		strcat(result, "/");
	strcat(result, s2);
	return result;
}

void find_files(const char *path, const char *file, const int arg)
{
	struct dirent *entry;
	struct stat statbuf;
	int ret = 1;
	DIR *dir;
	
	if ((dir = opendir(path)) == NULL)
	{
		syslog(LOG_NOTICE, "cant open %s", path);
		return;
	}
	
	while ((entry = readdir(dir)) != NULL)
	{
		if (lstat(entry->d_name, &statbuf))
		{
			syslog(LOG_NOTICE, "ERROR");
			continue;
		}

		if (advanced)
			syslog(LOG_NOTICE, "Obsluzenie pliku/folderu %s  %s Typ: %d", path, entry->d_name, statbuf.st_mode);
		
		
		if (S_ISLNK(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode))
			continue;
		
		if (S_ISDIR(statbuf.st_mode))
		{
			if (!strcmp(".", entry->d_name)
					|| !strcmp("..", entry->d_name))
				continue;
			
			//syslog(LOG_NOTICE, "Folder: %s \n", entry->d_name);
			char *new_path = concat(path, entry->d_name);
			//syslog(LOG_NOTICE, "new path %s", new_path);
			find_files(new_path, file, arg);
			free(new_path);
		}
		else if (S_ISREG(statbuf.st_mode))
		{
			if (strstr(entry->d_name, file) != NULL)
				syslog(LOG_NOTICE, "Path: %s Plik: %s Wzorzec: %s\n", path, entry->d_name, file);
		}
	}
	
	closedir(dir);
	return;
}

int main(int argc, char **argv)
{
	int d = daemon(0, 0);
	if (d < 0)
		printf("ERROR");
	
	if (argc < 2)
	{
		openlog("daemon6", LOG_PID, LOG_DAEMON);
		find_files("/", "bc", argc);
		closelog();
		return 0;
	}
	
	int sleep_time = 6000000;
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
	/*
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
	*/
	
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
		openlog("daemon3", LOG_PID, LOG_DAEMON);
		while(1)
		{
			if (setjmp(jump) == 0)
			{
				if (advanced)
					syslog(LOG_NOTICE, "Obudzenie sie %s", argv[1 + arguments]);
				find_files("/", argv[1 + arguments], argc - arguments);
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
