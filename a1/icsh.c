// type exit to go out of the shell

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>

# define MAXCHAR 12
# define BACKG 0
# define FOREG 1
# define SUSPENDED 2
# define RUNNING 4
# define BUILT_IN_COUNT 6

typedef struct job{
	
	int stdin, stdout, stderr;
	struct job* next;
	pid_t p_groupid;
	int id;
	int type;
	int status;
	struct termios termodes;

} job;


job* f_job = NULL;
int job_count;
int status;
int job_count;
pid_t shell_pgid;
struct termios shell_termodes;
int shell_terminal;
int shell_is_int; // Interactive 


char* builtin_str[] = {
  "bg", "cd", "echo", "exit", "fg", "jobs"
};

// Foward declaration
void wait_for_job(job* j);
void child_sig_handler(int num, siginfo_t* info, void*);
void sig_handler(int num, siginfo_t* info, void*);
int launch_jobs(int filein, int fileout, int fileerr, int mod, char* argv[]);
int launch_proc(int filein, int fileout, int fileerr, int mod, char* argv[]);
void free_job(job* j);
char* lsh_read_line();
int if_builtin(char* argv[]);
int init_shell();
void init_loop();
int lsh_parse(char* line, int* argc, int* mode, char* argv[], int*, int*);

int shell_bg(char *[]);
int shell_cd(char *[]);
int shell_echo(char *[]);
int shell_exit(char *[]);
int shell_fg(char *[]);
int shell_jobs(char *[]);

int (*builtin_func[]) (char *[]) = {
  &shell_bg,
  &shell_cd,
  &shell_echo,
  &shell_exit,
  &shell_fg,
  &shell_jobs };


int main(int argc, char *argv[]){

	init_shell();
	init_loop();

	return 0;
}

job* find_job(pid_t p_groupid){

	job* j;

	for (j = f_job; j; j = j->next){
		if (j->p_groupid == p_groupid){
			return j;
		}
	}

	return NULL;
}

job* find_job_by_id(int id){

	job* j;

	for (j = f_job; j; j = j->next){
		if (j->id == id){
			return j;
		}
	}
	return NULL;
}

int init_shell(){
	
	shell_is_int = isatty(shell_terminal);

	if (shell_is_int){

		// GOTO forground
		while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())){
			kill( - shell_pgid, SIGTTIN);
		}


		struct sigaction child_sa = {0};

		child_sa.sa_sigaction = child_sig_handler;
		child_sa.sa_flags = SA_SIGINFO;
		

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        sigaction(SIGCHLD, &child_sa, NULL);


        // initializing process group
        shell_pgid = getpid();

        if (setpgid(shell_pgid, shell_pgid) < 0){
        	perror("Process group problem");
        	exit(1);
        }

        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_termodes);

        job_count = 0;

        return 1;
	}
	else{
		return 0;
	}
}

void init_loop(){

	char* line;
	char* argv[MAXCHAR];
	int mode;
	int argc;
	int filein;
	int fileout;

	pid_t pid = getpid();

	do{
		status = 0;
		printf("icsh>");
		line = lsh_read_line();
		argc = 0;
		mode = FOREG;

		lsh_parse(line, &argc, &mode,  argv, &filein, &fileout);
		argv[argc] = NULL; 

		if (argc != 0){
			if(!if_builtin(argv)){
				launch_jobs(filein, fileout, STDERR_FILENO,
					mode, argv); // CHECK APPROPRIATE INPUT
			}
		}

		free(line);
		fflush(stdout);
		//printf("%d\n", status);
	} while(status >= 0);
	//printf("end of loop\n");
}


void put_job_in_fg(job* j, int c){

	//printf("In FOREG\n");


	j->type = FOREG;
	tcsetpgrp(shell_terminal, j->p_groupid); // Put job in fg

	if (c){

		tcsetattr(shell_terminal, TCSADRAIN, &j->termodes);

		if (kill(- j->p_groupid, SIGCONT) < 0){
			perror("kill (SIGCONT)");
		}

	}
	wait_for_job(j);

		//Put shell in fg
	tcsetpgrp(shell_terminal, shell_pgid);
}

void put_job_in_bg(job* j, int c){

	j->type = BACKG;

	if (c){
		if (kill(-j->p_groupid, SIGCONT) < 0){
			perror("kill (SIGCONT");
		}
	}
	
}

int if_builtin(char* argv[]){

	if (argv[0] == NULL){
		return -1;
	}

	for (int i = 0; i < BUILT_IN_COUNT; i++){

		if (strcmp(argv[0], builtin_str[i]) == 0){

			return (*builtin_func[i])(argv);
		}
	}

	return 0;
}

int shell_cd(char* argv[]){

	if (argv[1] == NULL){

		chdir(getenv("HOME"));
		setenv("OLDPWD", "PWD", 1);
		setenv("PWD", getenv("HOME"), 1);
	}
	else{

    	if (chdir(argv[1]) == -1) {
      		printf("%s: no such file or directory: %s\n", argv[0], argv[1]);
    }
    	else{
      		setenv("OLDPWD", "PWD", 1);
      		setenv("PWD", argv[1], 1);
    	}
	}

	return 1;
}

int shell_exit(char* argv[]){

	status = -1;
	return 1;
}

int shell_echo(char* argv[]){

	// TODO
}

int shell_jobs(char* argv[]){

	//printf("in shell jobs\n");

	job* current = f_job;

	while(current != NULL){
		printf("[%d]  %c ", current->id, '+');
		int current_status = current->status;

		if (current_status == SUSPENDED){
			printf("SUSPENDED ");
		}
		else if (current_status == RUNNING){
			printf("RUNNING ");
		}
		else{
			printf("idfk man ");
		}
		printf(" %d\n", current->id);
		current = current->next;
	}
	return 1;
}

int shell_fg(char* argv[]){

	if (argv[1] != NULL){

		// received as ID as string to converting to int
		int id = atoi(argv[1]);
		job* j = find_job_by_id(id);

		if (j == NULL){
			return 0;
		}
		if (j->status == SUSPENDED){
			put_job_in_fg(j, 1);
		}
		else{
			perror("Error in shell_fg");
		}
	}

	return 1;
}

int shell_bg(char* argv[]){

	if (argv[1] != NULL){

	// received as ID as string to converting to int
	int id = atoi(argv[1]);
	job* j = find_job_by_id(id);

	if (j == NULL){
		return 0;
	}

	if (j->status == SUSPENDED){
		put_job_in_bg(j, 1);
	}
	else{
		perror("Error in shell_fg");
	}
	}

	return 1;

}


void child_sig_handler(int num, siginfo_t* info, void* notused){

	int status = 0;
	pid_t info_pid = info->si_pid;
	pid_t pid = waitpid(info_pid, &status, WNOHANG | WUNTRACED);
	fflush(stdout); // FLushing the buffer

	if (pid == info_pid){

		if (WIFEXITED(status)){

			// child is dead rip..
			fflush(stdout);

			job* job = find_job(pid);

			if (job->type == BACKG){
				printf("\n[%d]  - %d completed", job->id, job->p_groupid);
				fflush(stdout);
			}
			free_job(job);
		}
		else if (WIFSIGNALED(status)){
			printf("child sent a signal\n");
			fflush(stdout);
		}
		else if (WIFSTOPPED(status)){

			job* job = find_job(pid);
			job->status = SUSPENDED;
			printf("[%d]  + %d suspended\n", job->id, pid);
			fflush(stdout);
		}
		else{
			// feck it flush anyway
			fflush(stdout);
		}
	}

}

void sig_handler(int num, siginfo_t* info, void* notused){

	switch(num){
		case SIGINT:
			status = 1;
			break;
	}
}



int launch_jobs(int filein, int fileout, int fileerr, int mode, char* argv[]){

	pid_t pid = fork();
	job* current;

	if (pid == 0){

		launch_proc(filein, fileout, fileerr, mode, argv);
	}
	else if (pid > 0){

		setpgid(pid, pid);

		// Setting up my job object
		job* njob = malloc(sizeof(job));
		njob->p_groupid = pid;
		njob->stdin = filein;
		njob->stdout = fileout;
		njob->stderr = fileerr;
		njob->type = mode;
		njob->next = NULL;
		njob->status = RUNNING;
		tcgetattr(shell_terminal, &njob->termodes);

		// If it's the first job
		if (f_job == NULL){
			job_count++;
			njob->id = job_count;
			f_job = njob;
		}
		else{
			// Else loop through to find the place where next is NULL
			// Then you set it as the next
			current = f_job;

			while(current->next != NULL){
				current = current->next;
			}
			njob->id = (current->id) + 1;
			current->next = njob;
			job_count++;
		}

		if (mode == FOREG){
			put_job_in_fg(njob, 0);
		}	
		else{
			put_job_in_bg(njob, 0);
			tcsetpgrp(shell_terminal, shell_pgid);
		}

	}
	else{
		perror("fork");
		status = -1;
		exit(EXIT_FAILURE);
	}
	return 1;

}
int launch_proc(int filein, int fileout, int fileerr, int mode, char* argv[]){

	pid_t pid = getpid();
	setpgid(pid, pid);
	//printf("%d\n", pid);

	if (mode == FOREG){
		tcsetpgrp(shell_terminal, pid);
	}

	struct sigaction child_sa = {0};


	child_sa.sa_sigaction = child_sig_handler;
	child_sa.sa_flags = SA_SIGINFO;

    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    sigaction(SIGCHLD, &child_sa, NULL);


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
	if (execvp(argv[0], argv) < 0){
		printf("\t- %s\n", strerror(errno));

	}
	//printf("LAUNCHED PROCESS\n");
	exit(EXIT_SUCCESS);

}

void free_job(job* j){

	job* prev = f_job;
	job* current = f_job->next;

	if (j->id == prev->id){
		prev = prev->next;
		f_job = current;
	}
	else{
		while (current != NULL) {
      		if (current->id == j->id) {
        		prev->next = current->next;
      		}

      		prev = current;
      		current = current->next;
    	}
	}
	job_count--;
	free(j);
}


void wait_for_job(job* j){

	//printf("in wait\n");

	int status;
	while(waitpid(j->p_groupid, &status, WNOHANG) == 0){
		if (j->status == SUSPENDED){
			return;
		}
	}

	free_job(j);
}


char* lsh_read_line(){

	char* line;
	size_t bufsize = 0;
	errno = 0;
	int c = getline(&line, &bufsize, stdin);
	//printf(" C u %d\n", c);
	if (c < 0) {

		printf("\n");
	} 
	else {
		line[c-1] = '\0';
	}
	return line;
}

int lsh_parse(char* line, int* argc, int* mode, char* argv[], int* filein,
				int* fileout){

	char* buffer;
	buffer = strtok(line, " "); // For seperating each space
	int file_num;

	*filein = STDIN_FILENO;
	*fileout = STDOUT_FILENO;

	// Peters sorcery DELETE IF DONT UNDERSTAND
	while(buffer != NULL && *argc < MAXCHAR){

		argv[(*argc)++] = buffer;
		buffer = strtok(NULL, " ");
	}

	// if the last char is &, set it to BACKG
	if ((*argc) > 2 && strcmp(argv[(*argc) - 1], "&") == 0){

		*mode = BACKG;
		// Set & to NULL after you ACK
		argv[(*argc - 1)] = NULL; 
		(*argc)--;
	}
	else{
		// ELSE just set last to NULL for exec
		argv[(*argc)] = NULL;
	}
	// stdin
	if (*argc > 3 && strcmp(argv[(*argc)-2], "<") == 0) {

	    file_num = open(argv[(*argc)-1], O_RDONLY);

	    if (file_num > 0){
	      *filein = file_num;
	    }

	    argv[(*argc)-2] = NULL;
	    (*argc) -= 2;
	 } 

	else if (*argc > 3 && strcmp(argv[(*argc)-2], ">") == 0) {

	    file_num= open(argv[(*argc)-1], O_CREAT | O_WRONLY,
	    	 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);	

	    if (file_num > 0){
	    	printf("%d\n", file_num);
	      	*fileout = file_num;
	    }

	    argv[(*argc)-2] = NULL;
	    (*argc) -= 2;
	}

	return *argc;

}
