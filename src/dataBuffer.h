#ifndef DATABUFFER_H
#define DATABUFFER_H

#define MAX_BUFFER_LENGTH 2000000 // data_buffer 

#include <string>
#include <queue>
#include "timeKeeper.h"
#include <pthread.h> // to be able to create threads

using namespace std;

/***************************************************************
buffer to hold the data comming from the acquisition process
that will be used by the recording and oscilloscope.

for efficiency new data will overwrite the older ones
in a wrap around manner.

This is more work to code but is more efficient
***************************************************************/ 
class dataBuffer
{
 public:
  dataBuffer();
  ~dataBuffer();
  unsigned long int number_samples_read; // total samples read since acq started
  unsigned long int oldest_sample_number; // oldest sample number currently in buffer
  int number_samples_in_buffer; // number of valid samples in buffer
  int max_number_samples_in_buffer; // number of valid samples in buffer when full
  int index_lowest_sample;
  int buffer_size;
  int number_channels;
  short int* buffer;// buffer to get data from comedi devices
 private:

};



#endif
