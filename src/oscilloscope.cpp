#define DEBUG_OSC
#include "oscilloscope.h"
#include <stdlib.h> 
#include <stdint.h>
#include <iostream>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <gtkmm.h>
#include <stdlib.h> 
#include <stdint.h>
#include <glibmm.h>


oscilloscope::oscilloscope(dataBuffer* datab)
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::oscilloscope()\n";
#endif

  db=datab;
  // Allocate memory for an internal buffer that can be used by other objects 
  buffer_size= OSC_BUFFER_LENGTH_SAMPLES*OSC_MAXIMUM_CHANNELS; // to rewind
  buffer = new double [buffer_size];
  transfer_buffer_size=OSC_TRANSFER_BUFFER_LENGTH_SAMPLES*OSC_MAXIMUM_CHANNELS;
  transfer_buffer= new double [transfer_buffer_size];
  show_buffer_size =  OSC_SHOW_BUFFER_LENGTH_SAMPLES*OSC_MAXIMUM_CHANNELS;
  show_buffer = new double [show_buffer_size];

  is_drawing=false;
  is_displaying=false;
  draw_only_mean=false;
  
  num_channels=db->getNumChannels();
  num_groups= OSC_GROUPS;
  max_channels_per_group = MAX_CHANNELS_PER_GROUP;
  grp = new channelGroup[num_groups];
  current_group=0;
  set_channel_group_default();
  sampling_rate=db->get_sampling_rate();

  max_samples_buffer=buffer_size/num_channels;
  max_samples_transfer_buffer= transfer_buffer_size/num_channels;
  number_samples_displayed=0;

  // for timer
  tslot = sigc::mem_fun(*this, &oscilloscope::on_timeout);
  

#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::oscilloscope()\n";
#endif
}

oscilloscope::~oscilloscope()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::~oscilloscope()\n";
#endif
  delete[] buffer;
  delete[] transfer_buffer;
  delete[] show_buffer;
  delete[] grp;
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::~oscilloscope()\n";
#endif
}

bool oscilloscope::on_timeout()
{
  #ifdef DEBUG_OSC
  cerr << "entering oscilloscope::on_timeout()\n";
#endif

#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::on_timeout()\n";
#endif
  return true;
}
bool oscilloscope::start_oscilloscope()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::start_oscilloscope()\n";
#endif
  is_displaying=true;
  timeout_connection = Glib::signal_timeout().connect(tslot,OSC_TIME_BETWEEN_UPDATE_MS); 
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::start_oscilloscope()\n";
#endif
  return true;
}
bool oscilloscope::stop_oscilloscope()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::stop_oscilloscope()\n";
#endif
  timeout_connection.disconnect();
  is_displaying=false;

#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::stop_oscilloscope()\n";
#endif
  return true;
}


void oscilloscope::set_channel_group_default()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::set_channel_group_default()\n";
#endif
  // set default values in channel groups
  for(int i = 0; i < num_groups; i++)
    {
      grp[i].set_num_channels(DEFAULT_CHANNELS_PER_GROUP);
      for(int j = 0; j < DEFAULT_CHANNELS_PER_GROUP; j++)
	grp[i].set_channel_id(j,i*DEFAULT_CHANNELS_PER_GROUP+j);
    }
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::set_channel_group_default()\n";
#endif
}

 int oscilloscope::get_num_groups()
{
  return num_groups;
}
 int oscilloscope::get_current_group()
 {
  return current_group;
}
bool oscilloscope::get_is_displaying()
{
  return is_displaying;
}
 void oscilloscope::set_current_group(int g)
{
  if(g<0)
    {
      cerr << "oscilloscope::set_current_group(int g), g is smaller than 0\n";
      return;
    }
  if(g>=num_groups)
    {
      cerr << "oscilloscope::set_current_group(int g), g is larger or equal to num_groups\n";
      return;
    }
  current_group=g;
}

channelGroup* oscilloscope::get_one_channel_group(int g)
{
  if(g<0)
    {
      cerr << "oscilloscope::get_one_channel_group(int g), g is smaller than 0\n";
      return NULL;
    }
  if(g>=num_groups)
    {
      cerr << "oscilloscope::get_one_channel_group(int g), g is larger or equal to num_groups\n";
      return NULL;
    }

  return &grp[g];
}
