//#define DEBUG_BUF
#include "dataBuffer.h"
#include <stdlib.h> 
#include <stdint.h>
#include <iostream>

dataBuffer::dataBuffer()
{
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::dataBuffer()\n";
#endif

  // Allocate memory for an internal buffer that can be used by other objects 
  buffer_size= MAX_BUFFER_LENGTH;
  number_samples_read=0; // total samples read since acq started
  oldest_sample_number=0; // oldest sample number currently in buffer
  number_samples_in_buffer=0; // number of valid samples in buffer
  max_number_samples_in_buffer=0; // number of valid samples in buffer when full
  index_next_sample=0;
  number_channels=0;
  addAtEnd=0;
  sampling_rate=0;
  buffer = new short int [buffer_size];
  pthread_mutex_unlock(&data_buffer_mutex);
#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::dataBuffer()\n";
#endif
}

dataBuffer::~dataBuffer()
{
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::~dataBuffer()\n";
#endif

  delete[] buffer;
  
#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::~dataBuffer()\n";
#endif
}

void dataBuffer::resetData()
{
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::resetData()\n";
#endif

  number_samples_read=0; // total samples read since acq started
  oldest_sample_number=0; // oldest sample number currently in buffer
  number_samples_in_buffer=0; // number of valid samples in buffer
  index_next_sample=0;
  pthread_mutex_unlock(&data_buffer_mutex);
#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::resetData()\n";
#endif

}

void dataBuffer::setNumChannels(int numChannels)
{
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::setNumChannels(int )\n";
#endif


  if(numChannels<1)
    {
      cerr << "dataBuffer::setNumChannels, numChannels should be larger than 0 but is " << numChannels << '\n';
      return;
    }
  number_channels=numChannels;
  max_number_samples_in_buffer=buffer_size/number_channels; // number of valid samples in buffer

#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::setNumChannels(int )\n";
#endif

}
int dataBuffer::getNumChannels()
{
  return number_channels;
}
unsigned long int dataBuffer::get_number_samples_read()
{
  return number_samples_read;
}

void dataBuffer::addNewData(int numSamples,short int* data)
{
  // add new data in the buffer
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::addNewData(int, short int* )\n";
#endif

 

  if(numSamples>max_number_samples_in_buffer)
    {
      cerr <<  "dataBuffer::addNewData; numSamples: " << numSamples << " max_number_samples_in_buffer: " << max_number_samples_in_buffer << '\n';
      return;
    }

  if(numSamples<0)
    {
      cerr << "dataBuffer::addNewData; numSamples: " << numSamples << "\n";
      return;
    }
  if(numSamples==0)
    return;

  // prevent changing the buffer while working with it
  pthread_mutex_lock(&data_buffer_mutex);

  // add the new data after the old data in the buffer, no wrapping
  if(index_next_sample+numSamples<=max_number_samples_in_buffer)
    {
      for(int sample = 0; sample < numSamples; sample++)
	for(int channel = 0; channel < number_channels; channel++)
	  buffer[((sample+index_next_sample)*number_channels)+channel]=data[sample*number_channels+channel];
    }
  else // need some wrapping
    {
      // add beginning new data at the end of buffer
      addAtEnd=max_number_samples_in_buffer-index_next_sample;
      for(int sample = 0; sample < max_number_samples_in_buffer-index_next_sample ; sample++)
	for(int channel = 0; channel < number_channels; channel++)
	  buffer[((sample+index_next_sample)*number_channels)+channel]=data[sample*number_channels+channel];
      
      // add the end of new data at the beginning of the buffer
      for(int sample = addAtEnd; sample < numSamples; sample++)
	for(int channel = 0; channel < number_channels; channel++)
	  buffer[((sample-addAtEnd)*number_channels)+channel]=data[sample*number_channels+channel];
    }
     
#ifdef DEBUG_BUF
  cerr << "before\n"
       << "numSamples: " << numSamples << '\n'
       << "max_number_samples_in_buffer: " << max_number_samples_in_buffer << '\n'
       << "index_next_sample before: " << index_next_sample << '\n'
       << "number_samples_read: " << number_samples_read << '\n';
#endif
 
  number_samples_read+=numSamples;
  index_next_sample=(index_next_sample+numSamples)%max_number_samples_in_buffer;

  // unlock the mutex
  pthread_mutex_unlock(&data_buffer_mutex);

#ifdef DEBUG_BUF
  cerr << "after\n"
       << "index_next_sample before: " << index_next_sample << '\n'
       << "number_samples_read: " << number_samples_read << '\n';
#endif
  
#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::addNewData(int, short int* )\n";
#endif
}


int dataBuffer::getNewData(unsigned long int firstSample, short int* data, int maxSamples,int numChannels, unsigned int* channelList)
{ // copy data from the buffer to data
  // returns the number of new samples copied in data
  
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::getNewData(int, short int* )\n";
  cerr << "firstSample: " << firstSample << " maxSamples: " << maxSamples << " numChannels: " << numChannels << '\n';
  cerr << "number_samples_read: " << number_samples_read << " index_next_sample:" << index_next_sample <<  " max_number_samples_in_buffer:" << max_number_samples_in_buffer << '\n'; 
#endif


  if(number_samples_read==0)
    { // buffer is empty
      return 0;
    }
  // check if firstSample is in the buffer
  if(firstSample>number_samples_read)
    {
      cerr << "dataBuffer::getNewData(), firstSample: " << firstSample << " is larger than number of samples read: " << number_samples_read << '\n';
      return 0;
    }
  if(number_samples_read>max_number_samples_in_buffer)
    if(firstSample<number_samples_read-max_number_samples_in_buffer)
      {
	cerr << "dataBuffer::getNewData(), num_samples_read:" << number_samples_read << " firstSample: " << firstSample << " is smaller than the first sample in the buffer. So some data are missing\n";
	return -1;
      }


  // check if channels are all present in the buffer
  for(int i = 0; i < numChannels;i++)
    {
      if(channelList[i]>=number_channels)
	{
	  cerr << "dataBuffer::getNewData(), channel " << i << " of the channelList is out of range: " << channelList[i] << "\n";
	  cerr << "it should be between 0 and " << number_channels << '\n';
	  return -1;
	}
    }

  // prevent changing the buffer while working with it
  pthread_mutex_lock(&data_buffer_mutex);

  // number of samples to copy
  samplesToCopy=number_samples_read-firstSample;
  // get index of start of coppy
  index_copy_start=(index_next_sample)-samplesToCopy; 
  if(index_copy_start<0)
    index_copy_start=max_number_samples_in_buffer+index_copy_start;

  if(samplesToCopy>maxSamples)
    {
      samplesToCopy=maxSamples;
    }

  

#ifdef DEBUG_BUF
  cerr << "number_samples_read: " << number_samples_read << " samplesToCopy: " << samplesToCopy << " index_copy_start: " << index_copy_start << '\n';
#endif


  // copy the data
  if(index_copy_start+samplesToCopy<max_number_samples_in_buffer)
    {// copy the data without wrapping
      for(int sample = 0; sample < samplesToCopy;sample++)
	for(int channel = 0; channel < numChannels;channel++)
	  data[(sample*numChannels)+channel]=buffer[(index_copy_start+sample)*number_channels+channelList[channel]];
      
    }
  else
    {// copy the data in two gos (end of buffer, then start of buffer)
      copyAtEnd=max_number_samples_in_buffer-index_copy_start;
      for(int sample = 0; sample < copyAtEnd;sample++)
	for(int channel = 0; channel < numChannels;channel++)
	  data[(sample*numChannels)+channel]=buffer[(index_copy_start+sample)*number_channels+channelList[channel]];

      for(int sample = copyAtEnd; sample < samplesToCopy;sample++)
	for(int channel = 0; channel < numChannels;channel++)
	  data[(sample*numChannels)+channel]=buffer[(sample-copyAtEnd)*number_channels+channelList[channel]];
    }
  
  pthread_mutex_unlock(&data_buffer_mutex);
#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::getNewData(int, short int* )\n";
#endif
  return samplesToCopy;
}

int dataBuffer::getNewData(unsigned long int firstSample, double* data, int maxSamples,int numChannels, unsigned int* channelList)
{ // copy data from the buffer to data
  // returns the number of new samples copied in data
  
#ifdef DEBUG_BUF
  cerr << "entering dataBuffer::getNewData(int, short int* )\n";
  cerr << "firstSample: " << firstSample << " maxSamples: " << maxSamples << " numChannels: " << numChannels << '\n';
  cerr << "number_samples_read: " << number_samples_read << " index_next_sample:" << index_next_sample <<  " max_number_samples_in_buffer:" << max_number_samples_in_buffer << '\n'; 
#endif
  if(number_samples_read==0)
    { // buffer is empty
      return 0;
    }
  // check if firstSample is in the buffer
  if(firstSample>number_samples_read)
    {
      cerr << "dataBuffer::getNewData(), firstSample: " << firstSample << " is larger than number of samples read: " << number_samples_read << '\n';
      return 0;
    }
  if(number_samples_read>max_number_samples_in_buffer)
    if(firstSample<number_samples_read-max_number_samples_in_buffer)
      {
	cerr << "dataBuffer::getNewData(), num_samples_read:" << number_samples_read << " firstSample: " << firstSample << " is smaller than the first sample in the buffer. So some data are missing\n";
	return -1;
      }

  // check if channels are all present in the buffer
  for(int i = 0; i < numChannels;i++)
    {
      if(channelList[i]>=number_channels)
	{
	  cerr << "dataBuffer::getNewData(), channel " << i << " of the channelList is out of range: " << channelList[i] << "\n";
	  cerr << "it should be between 0 and " << number_channels << '\n';
	  return -1;
	}
    }

  // prevent changing the buffer while working with it
  pthread_mutex_lock(&data_buffer_mutex);

  // number of samples to copy
  samplesToCopy=number_samples_read-firstSample;
  // get index of start of coppy
  index_copy_start=(index_next_sample)-samplesToCopy; 
  if(index_copy_start<0)
    index_copy_start=max_number_samples_in_buffer+index_copy_start;

  if(samplesToCopy>maxSamples)
    {
      samplesToCopy=maxSamples;
    }


#ifdef DEBUG_BUF
  cerr << "number_samples_read: " << number_samples_read << " samplesToCopy: " << samplesToCopy << " index_copy_start: " << index_copy_start << '\n';
#endif


  // copy the data
  if(index_copy_start+samplesToCopy<max_number_samples_in_buffer)
    {// copy the data without wrapping
      for(int sample = 0; sample < samplesToCopy;sample++)
	for(int channel = 0; channel < numChannels;channel++)
	  data[(sample*numChannels)+channel]=buffer[(index_copy_start+sample)*number_channels+channelList[channel]];
      
    }
  else
    {// copy the data in two gos (end of buffer, then start of buffer)
      copyAtEnd=max_number_samples_in_buffer-index_copy_start;
      for(int sample = 0; sample < copyAtEnd;sample++)
	for(int channel = 0; channel < numChannels;channel++)
	  data[(sample*numChannels)+channel]=buffer[(index_copy_start+sample)*number_channels+channelList[channel]];

      for(int sample = copyAtEnd; sample < samplesToCopy;sample++)
	for(int channel = 0; channel < numChannels;channel++)
	  data[(sample*numChannels)+channel]=buffer[(sample-copyAtEnd)*number_channels+channelList[channel]];
    }
  
  pthread_mutex_unlock(&data_buffer_mutex);
#ifdef DEBUG_BUF
  cerr << "leaving dataBuffer::getNewData(int, short int* )\n";
#endif
  return samplesToCopy;
}




void dataBuffer::set_sampling_rate(int sr)
{
  sampling_rate=sr;
}
int dataBuffer::get_sampling_rate()
{
  return sampling_rate;
}
