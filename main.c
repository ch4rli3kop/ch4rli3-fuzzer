#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#define TARGET  "/home/bob/openjpeg/build/bin/opj_decompress"
#define ARGS  {"test", "ttt"}

void sigfault(){
	signal(SIGSEGV, sigfault);
	printf("get sigfault!");
}

void run_target(char* target, char* argvs[]){

	pid_t pid = fork();
	pid_t state;

	if (pid < 0){
		fprintf(stderr, "fork() error!\n");
		exit(-1);
	}
	else if (pid == 0){
		//signal(SIGSEGV, sigfault);
		//sleep(3);
		printf("AAA : %s\n", argvs[0]);
		execv(target, argvs);
		exit(0);
	}
	else {
		wait(&state);
		if (WIFEXITED(state)){
			printf("normal terminated!\n");
		} else if (WIFSIGNALED(state)) {
			printf("abnormal terminated!\n");
			printf("signal : %d\n", WTERMSIG(state));
		}
		
		
		//printf("status : %d\n", state);
		//printf("child exited!\n");
	}	
	
	
}

int main(int argc, char* argv[]){

	pid_t pid;
	int state;
	char* target = "test/test";
	//char* target = TARGET;
	char* argvs[] = {"test/test", "test/log2", NULL};

	printf("[+] target : %s\n", target);
	run_target(target, argvs);	

	return 0;
}
