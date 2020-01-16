#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_GRP 10
#define MAX_CMD_ARG 10

/*
*************************************************************
*						VARIABLES							*
*************************************************************/
const char *prompt = "myshell> ";

char* cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ];

void fatal(char *str);
void execute_cmdline(char *cmdline);
int makelist(char *s, const char *delimiters, char** list, int MAX_LIST);
void cmd_cd(int argc, char** argv);


/*
*************************************************************
*							FATAL							*
*************************************************************/
void fatal(char *str) {
	perror(str);
	exit(1);
}
/*
*************************************************************
*						MAKELIST							*
*************************************************************/
int makelist(char *s, const char *delimiters, char** list, int MAX_LIST) {
	int i = 0;
	int numtokens = 0;
	char *snew = NULL;

	if ((s == NULL) || (delimiters == NULL))
		return -1;

	snew = s + strspn(s, delimiters);
	if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
		return numtokens;

	numtokens = 1;

	while (1) {
		if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
			break;
		if (numtokens == (MAX_LIST - 1))
			return -1;
		numtokens++;
	}
	return numtokens;
}
/*
*************************************************************
*							MAIN							*
*************************************************************/
int main(int argc, char** argv)
{
	int i = 0;

	while (1) {
		fputs(prompt, stdout);
		fgets(cmdline, BUFSIZ, stdin);
		cmdline[strlen(cmdline) - 1] = '\0';

		execute_cmdline(cmdline);
	}
	return 0;
}
/*
*************************************************************
*					EXECUTE_CMDLINE							*
*************************************************************/
void execute_cmdline(char* cmdline)
{
	int count = 0;
	int i = 0;
	pid_t pid;

	count = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

	if (strcmp("exit", cmdvector[0]) == 0) {
		exit(1);
	}
	else if (strcmp("cd", cmdvector[0]) == 0) {
		cmd_cd(count, cmdvector);
	}
	else if (strcmp("&", cmdvector[count - 1]) == 0) {
		pid = fork();
		if (pid == 0) {
			// & = null change and exec
			sleep(1000);
		}
		else if (pid == -1) {
			fatal("fork error");
		}
	}
	else {
		/*ls*/
		switch (fork()) {
			case -1:
				fatal("fork error");
			case 0:
				execvp(cmdvector[0], cmdvector);

			default:
				wait(NULL);
				fflush(stdout);
		}
	}
}