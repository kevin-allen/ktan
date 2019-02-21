/**************************************************************
 class to start or stop recording/oscilloscope from a different 
 program running on the same computer
 **************************************************************/
//#define DEBUG_SHARE
#include "shared_memory.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>


shared_memory::shared_memory()
{
  #ifdef DEBUG_SHARE
  cerr << "entering shared_memory::shared_memory()\n";
  #endif
  
  shm_unlink(KTANSHAREMEMORYNAME); // just in case
  shared_memory_size=sizeof(struct ktan_sm_struct);
  shared_memory_des=shm_open(KTANSHAREMEMORYNAME, O_CREAT | O_RDWR | O_TRUNC, 0600);
  
  if(shared_memory_des ==-1)
     {
       cerr << "problem with shm_open in shared_memory::shared_memory()\n";
       return;
     } 
  if (ftruncate(shared_memory_des,shared_memory_size) == -1)
    {
      cerr << "problem with ftruncate in shared_memory::shared_memory()\n";
      return;
    }
  ksm = (struct ktan_sm_struct*) mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_des, 0);
  if (ksm == MAP_FAILED) 
    {
      cerr << "problem with mmap in shared_memory::shared_memory()\n;";
      return;
    }

  // set default values
  ksm->start_osc=0;
  ksm->stop_osc=0;
  ksm->start_rec=0;
  ksm->stop_rec=0;

  /* Initialise attribute to mutex. */
  pthread_mutexattr_init(&ksm->attrmutex);
  pthread_mutexattr_setpshared(&ksm->attrmutex, PTHREAD_PROCESS_SHARED);
  /* Initialise mutex. */
  pthread_mutex_init(&ksm->pmutex, &ksm->attrmutex);
}


shared_memory::~shared_memory()
{
  #ifdef DEBUG_SHARE
  cerr << "entering shared_memory::~shared_memory()\n";
  #endif
  pthread_mutex_destroy(&ksm->pmutex);
  pthread_mutexattr_destroy(&ksm->attrmutex);
  // unmap the shared memory
  if(munmap(ksm,shared_memory_size) == -1) 
    {
      cerr << "problem with munmap in shared_memory::~shared_memory()\n";
      return;
    }
  shm_unlink(KTANSHAREMEMORYNAME);
}
int shared_memory::get_start_osc(){return ksm->start_osc;}
int shared_memory::get_stop_osc(){return ksm->stop_osc;}
int shared_memory::get_start_rec(){return ksm->start_rec;}
int shared_memory::get_stop_rec(){return ksm->stop_rec;}

void shared_memory::set_start_osc(int val){ksm->start_osc=val;}
void shared_memory::set_stop_osc(int val){ksm->stop_osc=val;}
void shared_memory::set_start_rec(int val){ksm->start_rec=val;}
void shared_memory::set_stop_rec(int val){ksm->stop_rec=val;}
