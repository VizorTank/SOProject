#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <string.h>
#include <dirent.h>

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
			
			//printf("%s \n", entry->d_name);
			printdir(entry->d_name, s);
		}
		else
		{
			if (strstr(entry->d_name, s) != NULL)
				printf("%s \n", entry->d_name);
		}
	}
	chdir("..");
	close(dp);
}



int main(int argc, char **argv)
{

	/*
	int d = daemon(0, 0);
	if (d < 0)
		printf("NO");
	
	openlog("test", LOG_PID, LOG_DAEMON);
	
	syslog(LOG_NOTICE, "T1");
	sleep(20);
	syslog(LOG_NOTICE, "T2");
	closelog();
	
	return EXIT_SUCCESS;
	*/
	if (argc < 2)
		printdir("/", "bc");
	else
		printdir("/", argv[1]);
	
	
	return 0;
}
