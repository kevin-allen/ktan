#ifndef POSITRACK_SHARED_MEMORY_H
#define POSITRACK_SHARED_MEMORY_H

#include <time.h>
#include <pthread.h>

// variable used for the shared memory with other processes
#define POSITRACKSHARE "/tmppositrackshare" 
#define POSITRACKSHARENUMFRAMES 100

struct positrack_shared_memory
{
  int numframes;
  unsigned long int id [POSITRACKSHARENUMFRAMES]; // internal to this object, first valid = 1, id 0 is invalid
  unsigned long int frame_no [POSITRACKSHARENUMFRAMES]; // from tracking system, frame sequence number
  struct timespec ts [POSITRACKSHARENUMFRAMES];
  pthread_mutexattr_t attrmutex;
  int is_mutex_allocated;
  pthread_mutex_t pmutex;  
};

// these functions are defined in acquisition.c, see end of file
void psm_add_frame(struct positrack_shared_memory* psm, unsigned long int fid, struct timespec fts);
void psm_init(struct positrack_shared_memory* psm);
void psm_free(struct positrack_shared_memory* psm);


#endif
