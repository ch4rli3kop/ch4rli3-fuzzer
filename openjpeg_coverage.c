// trace-pc-guard-cb.cc
#include <stdint.h>
#include <stdio.h>
#include <sanitizer/coverage_interface.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


void* shmAddr = NULL; 
int max = 0;

void memory_init(){
	int shmID;

	if ((shmID = shmget((key_t)1337, 0x1000, IPC_CREAT | 0666)) < 0){
		fprintf(stderr, "shmget() error!\n");
		exit(-1);
	}

	if ((shmAddr = shmat(shmID, (void*) 0, 0)) < 0 ){
		fprintf(stderr, "shmat() error\n");
		exit(-1);
	}

	memset(shmAddr, 0, 0x1000);
}


void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
  static uint64_t N;  // Counter for the guards.
  if (start == stop || *start) return;  // Initialize only once.
  
  memory_init();
  
  for (uint32_t *x = start; x < stop; x++)
    *x = N++;  // Guards should start from 0.

  printf("INIT: %p %p\n", start, stop);
  printf("Shared Memory : %p\n", shmAddr);
}



void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
  if (!*guard) return;

  uint8_t* pSharedBuffer = shmAddr;
  int idx = (*guard / 8);

  max = (idx > max) ? idx : max; 

  pSharedBuffer[idx] |= 1 << (*guard % 8);
  *guard = 0;
  //printf("max : %d\n", max);
}
