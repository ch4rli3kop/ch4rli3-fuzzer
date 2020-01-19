// trace-pc-guard-cb.cc
#include <stdint.h>
#include <stdio.h>
#include <sanitizer/coverage_interface.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


void* shmAddr = NULL; 
int* cnt = NULL;

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
	cnt = shmAddr;
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
  if (!*guard) return;  // Duplicate the guard check.
  // If you set *guard to 0 this code will not be called again for this edge.
  // Now you can get the PC and do whatever you want:
  //   store it somewhere or symbolize it and print right away.
  // The values of `*guard` are as you set them in
  // __sanitizer_cov_trace_pc_guard_init and so you can make them consecutive
  // and use them to dereference an array or a bit vector.
  //void *PC = __builtin_return_address(0);
  //char PcDescr[1024];
  // This function is a part of the sanitizer run-time.
  // To use it, link with AddressSanitizer or other sanitizer.
  //__sanitizer_symbolize_pc(PC, "%p %F %L", PcDescr, sizeof(PcDescr));
  //printf("guard: %p %x PC %s\n", guard, *guard, PcDescr);

  int idx = (*guard / 8);
  char* pSharedBuffer = shmAddr + 0x10;
  pSharedBuffer[idx] |= 1 << (*guard % 8);
  *cnt += 1;
  printf("======== blockMap[%d] : 0x%x, cnt : %d\n", idx, pSharedBuffer[idx], *cnt);
  *guard = 0;
  //sprintf(shmAddr, "HHHHHHHHHH\n");
}
