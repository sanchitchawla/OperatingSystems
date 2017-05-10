#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>


void shell_loop();

int main(){

	shell_loop();

	return 0;

}


void shell_loop(){

	char* line;
	char** args;
	int status = 1;

	while (status){
		printf("icsh> ");
		printf("\n");
		// Read line
	}

}


char* read_line(){

	char* line = NULL;
	size_t bufsize = 0;
	getline(&line, &bufsize, stdin);
	return line;

}

char** split_line(char* line){

	int bufsize = 64;
	int i = 0;

	char** all_tokens = (char**)malloc(bufsize * sizeof(char*));
	char* each_token = NULL;

	each_token = strtok(line, " ");

	while(each_token!= NULL){
		all_tokens[i] = each_token;
		i++;

		if (i >= bufsize){
			bufsize+= 64;
			all_tokens = realloc(all_tokens, bufsize * sizeof(char*));

			if (!all_tokens){
				printf("Something went wrong, Windows Style");
				exit(1);
			}
		}


		each_token = strtok(NULL, " ");
	}

	all_tokens[i] = NULL;
	return all_tokens;
}

int launch(char** args){

	pid_t pid;
	pid_t wpid;
	int status;


	pid = fork();

	if (pid == 0){

		if (execvp(args[0], args) == -1){
			perror("shell");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0){
		// Error forking
		perror("shell");
	}
	else{
		while(!WIFEXITED(status) && !WIFSIGNALED(status)){
			wpid = waitpid(pid, &status, WUNTRACED);
		}
	}

	return 1;

}