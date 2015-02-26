#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#define OSC_MAXIMUM_CHANNELS 256
#define OSC_BUFFER_LENGTH_SAMPLES 1000000  // oscilloscope_buffer 
#define OSC_TRANSFER_BUFFER_LENGTH_SAMPLES 100000
#define OSC_SHOW_BUFFER_LENGTH_SAMPLES 100000
#define OSC_GROUPS 4
#define MAX_CHANNELS_PER_GROUP 16
#define DEFAULT_CHANNELS_PER_GROUP 8
#define OSC_MAX_DISPLAY_LENGTH_SEC 4
#define OSC_TIME_BETWEEN_UPDATE_MS 100
#include <string>
#include "timeKeeper.h"
#include "dataBuffer.h"
#include "channelGroup.h"
#include <gtkmm.h>
using namespace std;

/***************************************************************
Class to take care of the oscilloscope in it own thread
***************************************************************/ 
class oscilloscope
{

 public:
  oscilloscope(dataBuffer* datab);
  ~oscilloscope();
  bool start_oscilloscope();
  bool stop_oscilloscope();
  int get_num_groups();
  int get_current_group();
  void set_current_group(int g);
  channelGroup* get_one_channel_group(int g);
  bool get_is_displaying();

 private:
  dataBuffer* db;
  bool is_drawing; // in a drawing function
  bool is_displaying; // oscilloscope active
  bool draw_only_mean;
  timeKeeper tk;

  // 3 buffers to have simpler functions
  double* buffer;
  double* transfer_buffer;
  double* show_buffer;
  int buffer_size;
  int transfer_buffer_size;
  int show_buffer_size;
  int num_channels;
  int sampling_rate;
  int num_groups;
  int max_channels_per_group;
  channelGroup* grp;
  int current_group;
  int max_samples_buffer;
  int max_samples_transfer_buffer;
  int max_samples_to_get;
  int ret_get_samples;
  unsigned long int number_samples_displayed;
  
  bool on_timeout();
  void set_channel_group_default();

 protected:
  sigc::slot<bool> tslot;
  sigc::connection timeout_connection; // for timeout


};



#endif
