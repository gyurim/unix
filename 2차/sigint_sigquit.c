#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CMD_GRP 10
#define MAX_CMD_ARG 10

/*************************************************************************
*           VARIABLES
*************************************************************************/
const char *prompt = "myshell> ";

char* cmdvector[MAX_CMD_ARG];
char* cmdgrps[MAX_CMD_GRP];
char cmdline[BUFSIZ];

void fatal(char* str);
void execute_cmdline(char* cmdline);
int makelist(char* s, const char *delimiters, char** list, int MAX_LIST);

void cmd_cd(int argc, char** argv);

void zombie_handler();
void sigint_handler(int sig);
void execute_cmdgrp(char* cmdline);

int background_flag;

/*************************************************************************
*                       FATAL
*************************************************************************/
void fatal(char* str) {
	perror(str);
	exit(1);
}

/*************************************************************************
*                               MAKELIST
*************************************************************************/
int makelist(char* s, const char* delimiters, char** list, int MAX_LIST) {
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

/*************************************************************************
*                               MAIN
*************************************************************************/
int main(int argc, char** argv) {
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = zombie_handler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	while (1) {
		fputs(prompt, stdout);
		fgets(cmdline, BUFSIZ, stdin);
		cmdline[strlen(cmdline) - 1] = '\0';

		execute_cmdline(cmdline);
	}
	return 0;
}

/*************************************************************************
*                              EXECUTE_CMDLINE
*************************************************************************/
void execute_cmdline(char* cmdline) {
	int i = 0;
	int count = 0;
	int count_vector = 0;
	int status;
	pid_t pid;
	char cmdgrptmp[BUFSIZ];

	background_flag = 0;

	count = makelist(cmdline, ";", cmdgrps, MAX_CMD_GRP);

	for (i = 0; i < count; i++) {
		memcpy(cmdgrptmp, cmdgrps[i], strlen(cmdgrps[i]) + 1);

		count_vector = makelist(cmdgrptmp, " \t", cmdvector, MAX_CMD_ARG);

		if (strcmp("&", cmdvector[count_vector - 1]) == 0) {
			cmdvector[count_vector - 1] = '\0';
			background_flag = 1;
		}

		if (strcmp("exit", cmdvector[0]) == 0)
			exit(1);
		else if (strcmp("cd", cmdvector[0]) == 0) {
			cmd_cd(count, cmdvector);
		}
		else {
			pid = fork();
			switch (pid) {
			case -1:
				fatal("fork error");

			case 0:
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				setpgid(0, 0);
				if (!background_flag) {
					tcsetpgrp(STDIN_FILENO, getpgid(0));
				}
				execvp(cmdvector[0], cmdvector);
				break;
			default:
				if (background_flag)
					break;
				if (!background_flag) {
					waitpid(pid, NULL, 0);
					tcsetpgrp(STDIN_FILENO, getpgid(0));
				}
				//fflush(stdout);
			}
			fflush(NULL);
		}
	}
}

/*************************************************************************
*                           CMD_CD
*************************************************************************/
void cmd_cd(int argc, char** argv) {
	if (argc == 1)
		chdir(getenv("HOME"));
	else if (argc == 2) {
		if (chdir(argv[1]))
			printf("No directory\n");
	}
	else
		printf("error\n");
}

/*************************************************************************
*                           EXECUTE_CMDGRP
*************************************************************************/
void execute_cmdgrp(char *cmdgrp) {
	int count = 0;
	count = makelist(cmdgrp, " \t", cmdvector, MAX_CMD_ARG);
	execvp(cmdvector[0], cmdvector);
	fatal("exec error\n");
}

/*************************************************************************
*                           ZOMBIE_HANDLER
*************************************************************************/
void zombie_handler() {
	waitpid(-1, NULL, WNOHANG);
}