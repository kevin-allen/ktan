#include "dataBuffer.h"
#include <stdlib.h> 
#include <stdint.h>
#include <iostream>

dataBuffer::dataBuffer()
{
  cerr << "entering dataBuffer::dataBuffer()\n";

  // Allocate memory for an internal buffer that can be used by other objects 
  buffer_size= MAX_BUFFER_LENGTH;
  number_samples_read=0;
  number_samples_read=0; // total samples read since acq started
  oldest_sample_number=0; // oldest sample number currently in buffer
  number_samples_in_buffer=0; // number of valid samples in buffer
  max_number_samples_in_buffer=0; // number of valid samples in buffer when full
  index_lowest_sample=0;
  number_channels=0;
  buffer = new short int [buffer_size];
  cerr << "leaving dataBuffer::dataBuffer()\n";
}

dataBuffer::~dataBuffer()
{
  cerr << "entering dataBuffer::~dataBuffer()\n";
  delete[] buffer;
  cerr << "leaving  dataBuffer::~dataBuffer()\n";
}
