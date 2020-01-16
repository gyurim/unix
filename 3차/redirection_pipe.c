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
*							VARIABLES
*************************************************************************/
const char *prompt = "myshell> ";

char* cmdvector[MAX_CMD_ARG];
char* cmdgrps[MAX_CMD_GRP];
char* cmdlist[MAX_CMD_ARG];
char cmdline[BUFSIZ];

void fatal(char* str);
void execute_cmdline(char* cmdline);
int makelist(char* s, const char *delimiters, char** list, int MAX_LIST);

void cmd_cd(int argc, char** argv);

void zombie_handler();
void execute_cmdgrp(char* cmdline);
void execute_cmd(char* cmdline);
void execute_redirection(char* cmdline);
int cmd_background(char* cmdline);

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
	int i;
	int count;
	int count_vector;
	int status;
	pid_t pid;

	count = makelist(cmdline, ";", cmdgrps, MAX_CMD_GRP);

	for (i = 0; i < count; i++) {

		background_flag = cmd_background(cmdgrps[i]);

		if (cmdgrps[i][0] == 'c' && cmdgrps[i][1] == 'd') {
			count_vector = makelist(cmdgrps[i], " \t", cmdvector, MAX_CMD_ARG);
			cmd_cd(count_vector, cmdvector);
		}
		else if (cmdgrps[i][0] == 'e' && cmdgrps[i][1] == 'x' && cmdgrps[i][2] == 'i' && cmdgrps[i][3] == 't')
			exit(1);
		else {
			pid = fork();
			switch (pid) {
			case -1:
				fatal("fork error");

			case 0:
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				setpgid(0, 0);
				if (!background_flag) {
					tcsetpgrp(STDIN_FILENO, getpgid(0));
				}
				//execvp(cmdvector[0], cmdvector);
				execute_cmdgrp(cmdgrps[i]);
				break;
			default:
				if (background_flag)
					break;
				waitpid(pid, NULL, 0);
				tcsetpgrp(STDIN_FILENO, getpgid(0));
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
*                           CMD_BACKGROUND
*************************************************************************/
int cmd_background(char* cmdline) {
	int i;
	for (i = 0; i < strlen(cmdline); i++) {
		if (cmdline[i] == '&') {
			cmdline[i] = '\0';
			return 1;
		}
	}
	return 0;
}

/*************************************************************************
*                           EXECUTE_CMDGRP
*************************************************************************/
void execute_cmdgrp(char* cmdline) {
	int p[2];
	int count;
	int i;
	pid_t pid;

	count = makelist(cmdline, "|", cmdlist, MAX_CMD_ARG);

	for (i = 0; i < count - 1; i++) {
		pipe(p);
		pid = fork();
		switch (pid) {
		case -1:
			fatal("fork error");
		case 0:
			close(p[0]);
			dup2(p[1], 1);
			execute_cmd(cmdlist[i]);
		default:
			close(p[1]);
			dup2(p[0], 0);
		}
	}
	execute_cmd(cmdlist[i]);
}
/*************************************************************************
*                           EXECUTE_CMD
*************************************************************************/
void execute_cmd(char* cmdline) {
	int count = 0;
	execute_redirection(cmdline);

	count = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

	execvp(cmdvector[0], cmdvector);
	fatal("exec error\n");
}


/*************************************************************************
*                           ZOMBIE_HANDLER
*************************************************************************/
void zombie_handler() {
	waitpid(-1, NULL, WNOHANG);
}

/*************************************************************************
*                           EXECUTE_REDIRECTION
*************************************************************************/
void execute_redirection(char* cmdline) {
	char* file;
	int fd;
	int i;
	int length = strlen(cmdline);

	for (i = 0; i < length; i++) {
		switch (cmdline[i]) {
		case '<':
			file = strtok(&cmdline[i + 1], " \t");
			if ((fd = open(file, O_RDONLY | O_CREAT, 0777)) < 0)
				fatal("open error");
			dup2(fd, 0);
			cmdline[i] = '\0';
			break;

		case '>':
			file = strtok(&cmdline[i + 1], " \t");
			if ((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0)
				fatal("open error");
			dup2(fd, 1);
			cmdline[i] = '\0';
			break;
		}
	}
}