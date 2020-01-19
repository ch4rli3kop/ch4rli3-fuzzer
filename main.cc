/*
this is the basic openjpeg fuzzer
by ch4rli3kop. 2020.01.19 
*/

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
#include <list>
#include <dirent.h>
#include <string.h>
#include <hash_map>
#include <openssl/sha.h>
#include "sha256.h"

using namespace std;

#define TARGET  "/home/bob/openjpeg/build/bin/opj_decompress"
//#define TARGET  "test/test"
char sp[] = "/-\|";

void* shmAddr; 
void* pmmap; 
int totalpath = 0;
int totalexec = 0;
int crash = 0;

queue<uint8_t *> input;
vector<uint8_t *> corpus;
vector<uint8_t *> path;

bool is_newPath(){
	uint8_t *digest = sha256((unsigned char*)shmAddr, 0x1000);

	for (int i = 0; i < path.size(); i++){
		if (!memcmp(digest, path[i], 32)) return false;
	}

	path.push_back(digest);
	totalpath++;
	
	// debug
	// cout << "Path : " << totalpath << endl;
	// char buf[2*SHA256::DIGEST_SIZE+1];
    // buf[2*SHA256::DIGEST_SIZE] = 0;
    // for (int i = 0; i < SHA256::DIGEST_SIZE; i++)
    //     sprintf(buf+i*2, "%02x", digest[i]);
	// cout << buf << endl;

	return true;
}

bool is_crashed(pid_t state){
	if(WIFSIGNALED(state)) {
		return true;
	}
	return false;
}

void Add_cur_input(){
	ifstream in("out/cur_input.j2k", ios::ate);
	if (in.fail()){
        perror("open cur_input error!\n");
        exit(-1);
    }
	uint32_t length = in.tellg();
	in.seekg(0, ios::beg);
	uint8_t* buf = new uint8_t[length+1];
    in.read((char*)buf, length);
	
	string fileName = "out/crash/input_" + to_string(crash);
	ofstream out(fileName, ios::binary);
	out.write((char*)buf, length);
	out.close();
}

void Add_state(int state){
	ofstream out("out/crash_log", ios::app);
	out << "======================================" << endl;
	out << "crash " << crash << endl;
	out << "  signal : " << (int)WTERMSIG(state) << endl;
	out.close();
	crash++;
}

void Add_to_report(int state){
	Add_cur_input();
	Add_state(state);
}

void Add_to_corpus(){
	ifstream in("out/cur_input.j2k", ios::ate);
	if (in.fail()){
        perror("open cur_input error!\n");
        exit(-1);
    }
	uint32_t length = in.tellg();
	in.seekg(0, ios::beg);

	uint8_t* buf = new uint8_t[length+1+4];
    memcpy(buf, &length, 4);
	in.read((char*)buf+4, length);
	corpus.insert(corpus.begin(), buf);
}

void run_target(char* target, char* argvs[]){

	pid_t pid = fork();
	int state;

	if (pid < 0){
		perror("fork() error!\n");
		exit(-1);
	}
	else if (pid == 0){
		execv(target, argvs);
		totalexec++;
		exit(0);
	}
	else {
		wait(&state);

		// Check if the testcase causes crash.
		if (is_crashed(state)){
			Add_to_report(state);
		}


		// Check if the testcase hits new code path.
		if (is_newPath()){
			// If it hits new code path, Add to corpus.
			Add_to_corpus();	
		}

		//printf("status : %d\n", state);
		//printf("child exited!\n");
	}	
	
	
}

// void memory_init(){
// 	int fd;
// 	pmmap = mmap((void*)0, 0x10000, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);	
// }

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

		uint8_t* buf = new uint8_t[length+1+4];
		memcpy(buf, &length, 4);
		in.read((char*)buf + 4, length);

		// cout << "./in/" << entry->d_name << endl;
		// cout << "File length : " << length << endl;
		// cout << buf << endl;
		corpus.push_back(buf);
		//input.push(buf);
	}

}

void use_radamsa(){
	system("radamsa out/cur_input.j2k > out/tmp");
	system("mv out/tmp out/cur_input.j2k");
}

void mutation(uint8_t* _data){
	uint8_t* data = _data + 4;
	uint32_t length;
	
	memcpy((char*)&length, _data, 4);

    uint8_t* new_data;
	
    // switch(random){
    //     case 1: 
 	// 		// check buffer overflow
	// 		// for (int i = 1; i < 0x10; i++){
	// 		// 	int _len = length * i;
	// 		// 	char* buf = new char[4 + _len];
	// 		// 	memcpy(buf, (char*)&_len, 4);
	// 		// 	for (int j = 0; j < i; j++){
	// 		// 		memcpy(buf + 4 + length * j, data, length);
	// 		// 	}
	// 		// }
    //     case 2:
    //         // check Known Value (-1, 0, 1, INT_MAX, ...)
    //     case 3:
    //         // bit flip
    //     case 4:
    //         //...
    // }
	// input.push(new_data);
	
	input.push(_data);

}

void create_input(){
	// make cur_input file by <vector>input
	uint8_t* data = input.front();
	uint32_t length;

	memcpy((char*)&length, data, 4);

	ofstream out("out/cur_input.j2k", ios::binary);
	out.write((char*)data + 4, length);
	out.close();
	input.pop();
}

int main(int argc, char* argv[]){

	char* target = TARGET;
	char* argvs[] = {TARGET, "-i", "out/cur_input.j2k", "-o", "out/tmp.pgm", NULL};

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
				//use_radamsa();
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

	// char* Data = reinterpret_cast<char*>(shmAddr);
	// printf("%p\n", shmAddr);
	// printf("%p\n", Data);

	// printf("Result : %s\n", shmAddr);
	// for (int i = 0; i < 0x100; i++){
	// 	if (i % 0x10 == 0) printf("\n");
	// 	printf("%x ", Data[i]);
	// }

	return 0;
}
