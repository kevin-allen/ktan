#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#define OSC_MAXIMUM_CHANNELS 128
#define OSC_BUFFER_LENGTH_SAMPLES 100000  // oscilloscope_buffer 
#define OSC_SHOW_BUFFER_LENGTH_SAMPLES 100000
#define OSC_GROUPS 4
#define MAX_CHANNELS_PER_GROUP 16
#define DEFAULT_CHANNELS_PER_GROUP 8
#define MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE 2
#define MIN_TIME_SEC_IN_OSCILLOSCOPE_PAGE 0.03125 
#define OSCILLOSCOPE_DEFAULT_TIME_SEC_IN_PAGE 1 // refresh osciloscope every 1 sec
#define OSCILLOSCOPE_DEFAULT_GAIN .10
#define OSCILLOSCOPE_MAX_GLOBAL_GAIN 1
#define OSCILLOSCOPE_MIN_GLOBAL_GAIN .001
#define OSCILLOSCOPE_GLOBAL_GAIN_CHANGE_FACTOR 1.25
#define OSCILLOSCOPE_X_MARGIN_LEFT 50
#define OSCILLOSCOPE_X_MARGIN_RIGHT 25
#define OSCILLOSCOPE_Y_MARGIN_TOP 50
#define OSCILLOSCOPE_Y_MARGIN_BOTTOM 50
#define OSCILLOSCOPE_PIXELS_PER_DATA_POINT_TO_DRAW 1 // integer ranging from 1 to 10, to reduce drawing time
#define OSCILLOSCOPE_MAX_SAMPLING_RATE 60000
#define TIME_SEC_IN_OSCILLOSCOPE_PAGE_CHANGE_FACTOR 2

#define OSC_TIME_BETWEEN_UPDATE_MS 20
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

  void increase_gain();
  void decrease_gain();
  void increase_time_shown();
  void decrease_time_shown();



  channelGroup* get_one_channel_group(int g);
  bool get_is_displaying();
  void reset();
  void refresh();



 private:
  dataBuffer* db;
  bool is_drawing; // in a drawing function
  bool is_displaying; // oscilloscope active
  bool draw_only_mean;
  timeKeeper tk;

  // 3 buffers to have simpler functions
  double* buffer;
  double* buffer_ptr;
  double* page_ptr;
  double* show_buffer;
  unsigned int* all_channels_list;
  int buffer_size;
  int show_buffer_size;
  int num_channels;
  int sampling_rate;
  int num_groups;
  int max_channels_per_group;
  channelGroup* grp; // arrays of groups controlled via gui
  channelGroup grp_for_display; // groups being displayed
  int current_group;
  int max_samples_buffer;
  int max_samples_to_get;
  int ret_get_samples;
  unsigned long int num_samples_displayed;
  int displayed_pages;
  
  
  double seconds_per_page;
  int samples_per_page;
  int page_size;
  int num_pages_buffer;
  int current_page;
  int new_samples_buffer;
  int pages_in_memory;
  


  double* x_axis_data;
  int x_margin_left;
  int x_margin_right;
  int y_margin_top;
  int y_margin_bottom;
  int pixels_per_data_point_to_draw;

  bool on_timeout();
  void set_channel_group_default();
  int get_data();
  int show_new_data();
  int show_data(int page);
  int fill_show_buffer(int page);
  void update_time_gain();
  
  double global_gain;
  double min_global_gain;
  double max_global_gain;
  double global_gain_factor;
  double gui_seconds_per_page;
  double gui_global_gain;

 protected:
  sigc::slot<bool> tslot;
  sigc::connection timeout_connection; // for timeout


};



#endif
