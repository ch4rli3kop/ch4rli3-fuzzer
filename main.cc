#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <queue>
#include <vector>
#include <dirent.h>
#include <string.h>
#include <hash_map>

using namespace std;

//#define TARGET  "/home/bob/openjpeg/build/bin/opj_decompress"
#define TARGET  "test/test"

void* shmAddr;
void* pmmap; 
int totalpath = 0;
int totalexec = 0;



queue<char*> input;
vector<char*> corpus;
vector<char*> path;

bool is_newPath(){
	return true;
}

void run_target(char* target, char* argvs[]){

	pid_t pid = fork();
	pid_t state;

	if (pid < 0){
		perror("fork() error!\n");
		exit(-1);
	}
	else if (pid == 0){
		execv(target, argvs);
		exit(0);
		totalexec++;
	}
	else {
		wait(&state);

		

		// Check if the testcase causes crash.
		if (WIFSIGNALED(state)){
			// It is crashed.
			// Add to report.
			printf("signal : %d\n", WTERMSIG(state));

		} else { // WIFEXITED(state)
			// Normal terminated or paused.

		}


		// Check if the testcase hits new code path.
		if (is_newPath()){
			// If it hits new code path, Add to corpus.
			// Add to corpus
		}

		//printf("status : %d\n", state);
		//printf("child exited!\n");
	}	
	
	
}

void memory_init(){
	int fd;
	pmmap = mmap((void*)0, 0x10000, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);	
}


// get coveraged information from shared memory buffer
void shared_memory_init(){
	int shmID;
	
	if ((shmID = shmget((key_t)1337, 0x1000, IPC_CREAT | 0666)) < 0 ){
		perror("shmget() error!\n");
		exit(-1);
	}
	
	if ((shmAddr = shmat(shmID, (void*)0, 0)) == (void*)-1){
		perror("shmat() error!\n");
		exit(-1);
	}
}

void init_corpus(){
	
	DIR *dir;
	struct dirent *entry;
	string in_dir = "./in/";

	if (!(dir = opendir(in_dir.c_str()))){
		perror("open dir error!\n");
	}

	while ((entry = readdir(dir))){
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name,"..")) continue;
		string filename(entry->d_name);
		ifstream in(in_dir + filename, ios::ate);

		if (in.fail()){
			perror("open file error!\n");
			exit(-1);
		}

		uint32_t length = in.tellg();
		in.seekg(0, ios::beg);

		char* buf = new char[length+1+4];
		memcpy(buf, &length, 4);
		in.read(buf + 4, length);

		// cout << "./in/" << entry->d_name << endl;
		// cout << "File length : " << length << endl;
		// cout << buf << endl;
		corpus.push_back(buf);
		input.push(buf);
	}

}

void mutation(char* _data){
	char* data = _data + 4;
	int length;
	
	memcpy((char*)&length, _data, 4);
	
	// check buffer overflow
	for (int i = 1; i < 0x10; i++){
		int _len = length * i;
		char* buf = new char[4 + _len];

		memcpy(buf, (char*)&_len, 4);
		for (int j = 0; j < i; j++){
			memcpy(buf + 4 + length * j, data, length);
		}
		input.push(buf);
	}

}

void create_input(){
	// make cur_input file by <vector>input
	char* data = input.front();
	int length;

	memcpy((char*)&length, data, 4);

	ofstream out("out/cur_input", ios::binary);
	out.write(data + 4, length);
	out.close();
	input.pop();
}

int main(int argc, char* argv[]){

	char* target = TARGET;
	char* argvs[] = {TARGET, "out/cur_input", NULL};
	char* data;
	int length;

	printf("[+] target : %s\n", target);

	shared_memory_init();
	init_corpus();

	while (true){
	
		for (int i = 0; i < corpus.size(); i++){
			mutation(corpus[i]);

			// run target with the input created by mutation.
			int input_size = input.size();
			for (int j = 0; j < input_size; j++){
				create_input();
				run_target(target, argvs);
			}
		}
	}

	// cout << corpus.size() << endl;
	// int len = corpus.size();
	// for (int i = 0; i < len; i++){
	// 	char* tmp = corpus.front();
	// 	corpus.pop();
	// 	printf("%p\n", tmp);
	// 	printf("%s\n", tmp);
	// }

	char* Data = reinterpret_cast<char*>(shmAddr);
	printf("%p\n", shmAddr);
	printf("%p\n", Data);

	printf("Result : %s\n", shmAddr);
	for (int i = 0; i < 0x100; i++){
		if (i % 0x10 == 0) printf("\n");
		printf("%x ", Data[i]);
	}

	return 0;
}
