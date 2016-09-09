#ifndef SHARE_H
#define SHARE_H
#include <pthread.h> // to be able to create threads
#define KTANSHAREMEMORYNAME "/tmpktansharememory"
using namespace std;

struct ktan_sm_struct
{
  int start_osc;
  int stop_osc;
  int start_rec;
  int stop_rec;
  pthread_mutexattr_t attrmutex;
  int is_mutex_allocated;
  pthread_mutex_t pmutex;
};

class shared_memory
{
 public:
  shared_memory();
  ~shared_memory();
  int get_start_osc();
  int get_stop_osc();
  int get_start_rec();
  int get_stop_rec();
  void set_start_osc(int val);
  void set_stop_osc(int val);
  void set_start_rec(int val);
  void set_stop_rec(int val);

 private:
  struct ktan_sm_struct* ksm; // to share memory with other processes
  int shared_memory_size;
  int shared_memory_des;
};



#endif


  
