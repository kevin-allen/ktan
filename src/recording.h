#ifndef RECORDING_H
#define RECORDING_H

#define RECORDING_MAXIMUM_CHANNELS 256
#define MAX_REC_BUFFER_LENGTH 200000 // recording_buffer 
#define FILLING_PROPORTION_BEFORE_SAVE .30
#define DEFAULT_NUM_CHANNELS_SAVE 32

#include <string>
#include <queue>
#include "timeKeeper.h"
#include <pthread.h> // to be able to create threads
#include "dataBuffer.h"

using namespace std;

/***************************************************************
Class to take care of the recording in it own thread
***************************************************************/ 
class recording
{

 public:
  recording(dataBuffer* datab);
  ~recording();
  bool start_recording();
  bool stop_recording();
  bool set_recording_channels(int numChannels, unsigned int* channelList);
  static void *recording_thread_helper(void *context) // helper function to start the new thread
  {
    ((recording *)context)->recording_thread_function();
  }

    
 private:
  dataBuffer* db;
  bool is_recording; // to send signal to the recording thread
  FILE *file;
  char* file_name; // directory + file name from gui
  char* directory; // 
  off_t file_size; // to check if the file size makes sense
  off_t predicted_file_size; // to check the size of file at the end
  int number_channels_save; // number of channels that will be saved
  unsigned int channel_list[RECORDING_MAXIMUM_CHANNELS]; // list of channels that will be saved
  int buffer_size;
  int max_samples_in_buffer;
  double proportion_buffer_filled_before_save;
  short int* buffer;
  short int* rec_buffer_ptr;
  int max_samples_to_get;
  int ret_get_samples;
  struct timespec inter_recording_sleep_timespec; // sleeping time between attemping to save
  struct timespec start_recording_time_timespec;
  struct timespec now_timespec;
  struct timespec duration_recording_timespec;
  struct timespec req;
  double inter_recording_sleep_ms;
  timeKeeper tk;

  void *recording_thread_function(void);
  bool save_buffer_to_file();
  bool open_file();
  bool close_file();

  // following variables that changes during recording process
  int new_samples_in_buffer;
  unsigned long int number_samples_saved; // this is the 0-based index of first sample in the recording buffer
  double recording_time_sec;
};



#endif
