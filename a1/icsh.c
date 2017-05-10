#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

typedef struct process{

	struct process* next;
	char** argv;
	pid_t pid; // Process ID
	char complete;
	char stopornot;
	int status;
	
} process;

typedef struct job{
	
	
	int stdin, stdout, stderr;
	struct job* next;
	char* cmd;
	process* f_proc;
	pid_t p_groupid;
	char isNotified;
	struct termios termodes;

} job;

job* f_job = NULL;
pid_t shell_pgid;
struct termios shell_termodes;
int shell_terminal;
int shell_is_int; // Interactive 


// Find job 
job* find_job(pid_t p_groupid){

	job* j;

	for (j = f_job; j; j = j->next){
		if (j->p_groupid == p_groupid){
			return j;
		}
	}
	return NULL;
}

int is_stopped(job* j){

	process* p;

	for (p = j->f_proc; p; p ->next){
		if (!p->complete && !p->stopornot){
			return 0;
		}
		return 1;
	}
}

int is_complete(job* j){

	process* p;

	for (p = j->f_proc; p; p ->next){
		if (!p->complete){
			return 0;
		}
		return 1;
	}
}

void init_shell(){
	
	shell_terminal = STDIN_FILENO;
	shell_is_int = isatty(shell_terminal);

	if (shell_is_int){

		// GOTO forground

		while(tcgetpgrp(shell_terminal) != shell_pgid = getpgrp()){
			kill( - shell_pgid, SIGTTIN);
		}


        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

        shell_pgid = getpid();

        if (setpgid(shell_pgid, shell_pgid) < 0){
        	perror("Process group problem");
        	exit(1);
        }

        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_termodes);


	}
}

// All processes are children of the shell, no ancestor 
// Process group leader present, we use the first process inthe groups ID as the group ID
// Each child must be in a group before it begins executing


void launch(process* p, pid_t p_groupid, int filein, fileout, int fileerr, int fg){
	pid_t pid;

	if (shell_is_int){


		// Put each process into the group 

		pid = getpid();

		if (p_groupid == 0) p_groupid = pid;

		setpgid(pid, p_groupid);
		if (fg) tcsetpgrp(shell_terminal, p_groupid);

		// Handling signals for job control
		signal(SIGINT, SIG_DFL);
      	signal(SIGQUIT, SIG_DFL);
      	signal(SIGTSTP, SIG_DFL);
      	signal(SIGTTIN, SIG_DFL);
      	signal(SIGTTOU, SIG_DFL);
      	signal(SIGCHLD, SIG_DFL);
	}

	if (filein != STDIN_FILENO){
		dup2(filein, STDIN_FILENO);
		close(filein);
	}
	if (fileout != STDOUT_FILENO){
		dup2(fileout, STDOUT_FILENO);
		close(fileout);
	}
	if (fileerr != STDERR_FILENO){
		dup2(fileerr, STDERR_FILENO);
		close(fileerr);
	}

	execvp(p->argv[0], p->argv);
	perror("execvp, launch");
	exit(2);

}

void launch_jobs(job* j, int fg){

	process* p;
	pid_t pid;
	int pipe[2], fileout;
	int filein = j->stdin;

	for (p = j->f_proc;p; p = p->next){

		if (p->next){

			if (pipe(pipe) < 0){
				perror("pipe");
				exit(1);
			}

			fileout = pipe[1];
		}
		else{
			fileout = j->stdout;
		}

		// fork the child processes
		pid = fork();
		if (pid == 0){
			// Launching the child process
			launch(p, j->p_groupid, filein, fileout, j->stderr, fg);
		}
		// If error
		else if (pid < 0){
			perror("fork");
			exit(3);
		}
		else{
			// Parent

			p->pid = pid;

			if (shell_is_int){

				if (!j->p_groupid){
					j->p_groupid = pid; // Set group id to the parent process ID
				}
				setpgid(pid, j->p_groupid);
			}

		}

		if (filein != j->stdin){
			close(filein);
		}
		if (fileout != j->stdout){
			close(fileout);
		}

		filein = pipe[0];
	}

	format_job_info(j, "launched");

	if (!shell_is_int){
		wait_for_job(j);
	}
	else if (fg){
		put_job_in_fg(j, 0);
	}
	else{
		put_job_in_bg(j, 0);
	}


}

int main(int argc, char *argv[]){


	
	return 0;
}