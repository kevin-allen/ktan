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
  show_buffer_size =  OSC_SHOW_BUFFER_LENGTH_SAMPLES*OSC_MAXIMUM_CHANNELS;
  show_buffer = new double [show_buffer_size];

  is_drawing=false;
  is_displaying=false;
  draw_only_mean=false;
  
  num_channels=db->getNumChannels();
  all_channels_list = new unsigned int [num_channels];
  for(int i = 0; i < num_channels;i++)
    all_channels_list[i]=i;

  num_groups= OSC_GROUPS;
  max_channels_per_group = MAX_CHANNELS_PER_GROUP;
  grp = new channelGroup[num_groups];
  current_group=0;
  set_channel_group_default();
  sampling_rate=db->get_sampling_rate();
  seconds_per_page=OSCILLOSCOPE_DEFAULT_TIME_SEC_IN_PAGE;
  gui_seconds_per_page=seconds_per_page;
  samples_per_page=sampling_rate*seconds_per_page;
  page_size=samples_per_page*num_channels;
  num_pages_buffer=buffer_size/page_size;
  current_page=0;
  pages_in_memory=0;
  displayed_pages=0;
  pixels_per_data_point_to_draw=OSCILLOSCOPE_PIXELS_PER_DATA_POINT_TO_DRAW;
  x_margin_left=OSCILLOSCOPE_X_MARGIN_LEFT;
  x_margin_right=OSCILLOSCOPE_X_MARGIN_RIGHT;
  y_margin_top=OSCILLOSCOPE_Y_MARGIN_TOP;
  y_margin_bottom=OSCILLOSCOPE_Y_MARGIN_BOTTOM;

  x_axis_data= new double[OSCILLOSCOPE_MAX_SAMPLING_RATE*MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE];
  for (int i=0; i<OSCILLOSCOPE_MAX_SAMPLING_RATE*MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE;i++)
    x_axis_data[i]=i;
  
  max_samples_buffer=buffer_size/num_channels;
  new_samples_buffer=0;
  num_samples_displayed=0;
  current_group=0;
  
  global_gain=OSCILLOSCOPE_DEFAULT_GAIN;
  gui_global_gain=global_gain;
  min_global_gain=OSCILLOSCOPE_MIN_GLOBAL_GAIN;
  max_global_gain=OSCILLOSCOPE_MAX_GLOBAL_GAIN;
  global_gain_factor=OSCILLOSCOPE_GLOBAL_GAIN_CHANGE_FACTOR;
  
  // for timer
  tslot = sigc::mem_fun(*this, &oscilloscope::on_timeout);
  

#ifdef DEBUG_OSC
  cerr << "osc sampling_rate: " << sampling_rate << '\n';
  cerr << "osc num_channels: " << num_channels << '\n';
  cerr << "osc seconds_per_page: " << seconds_per_page << '\n';
  cerr << "osc samples_per_page: " << samples_per_page << '\n';
  cerr << "osc page size: " << page_size << '\n';
  cerr << "osc buffer size: " << buffer_size << '\n';
  cerr << "osc num_pages_buffer: " << num_pages_buffer << '\n';
  cerr << "leaving oscilloscope::oscilloscope()\n";

#endif
}

oscilloscope::~oscilloscope()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::~oscilloscope()\n";
#endif
  delete[] buffer;
  delete[] show_buffer;
  delete[] grp;
  delete[] all_channels_list;
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::~oscilloscope()\n";
#endif
}

bool oscilloscope::on_timeout()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::on_timeout()\n";
#endif
  if(is_displaying==false)
    { 
      return false;
    }
  if(is_drawing==true)
    { // only once at a time
      return true;
    }
  is_drawing==true;

  // get a copy of the display group so that any change during 
  // the displaying process does not cause segmentation faults
  // so for displaying use grp_for_display
  grp[current_group].copy_channelGroup(grp_for_display);
  
  
  // gui can affect time and gain only when this function is called
  // prevent gui changes from affecting drawing until it is completed
  update_time_gain();
  
  // transfer data from db to buffer
  if(get_data()<0)
    {
      cerr << "oscilloscope::on_timeout(), problem getting new data\n";
      reset();
    }
  
  // display the new data if we have a complete page to show
  show_new_data();

  
  is_drawing==false;
  
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::on_timeout()\n";
#endif
  return true;
}

void oscilloscope::update_time_gain()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::update_time_gain()\n";
#endif
  if (seconds_per_page!=gui_seconds_per_page||global_gain!=gui_global_gain)
    {
      global_gain=gui_global_gain;
      seconds_per_page=gui_seconds_per_page;
      samples_per_page=sampling_rate*seconds_per_page;
      page_size=samples_per_page*num_channels;
      num_pages_buffer=buffer_size/page_size;
      current_page=0;
      new_samples_buffer=0;
      num_samples_displayed=db->get_number_samples_read();
      pages_in_memory=0;
    }
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::update_time_gain()\n";
#endif

}

void oscilloscope::increase_gain()
{
  
}
void oscilloscope::decrease_gain()
{
  
}
void oscilloscope::increase_time_shown()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::increase_time_shown()\n";
#endif

  if(gui_seconds_per_page/TIME_SEC_IN_OSCILLOSCOPE_PAGE_CHANGE_FACTOR>=MIN_TIME_SEC_IN_OSCILLOSCOPE_PAGE)
    {
      gui_seconds_per_page=gui_seconds_per_page/TIME_SEC_IN_OSCILLOSCOPE_PAGE_CHANGE_FACTOR;
    }
#ifdef DEBUG_OSC
  cerr << "new gui_seconds_per_page: " << gui_seconds_per_page << '\n';
  cerr << "leaving oscilloscope::increase_time_shown()\n";
#endif

}
void oscilloscope::decrease_time_shown()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::decrease_time_shown()\n";
#endif
   if(gui_seconds_per_page*TIME_SEC_IN_OSCILLOSCOPE_PAGE_CHANGE_FACTOR<=MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE)
    {
      gui_seconds_per_page=gui_seconds_per_page*TIME_SEC_IN_OSCILLOSCOPE_PAGE_CHANGE_FACTOR;
    }
#ifdef DEBUG_OSC
   cerr << "new gui_seconds_per_page: " << gui_seconds_per_page << '\n';
  cerr << "entering oscilloscope::decrease_time_shown()\n";
#endif
}

void oscilloscope::refresh()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::refresh()\n";
#endif


#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::refresh()\n";
#endif
}

int oscilloscope::fill_show_buffer(int page)
{
  // in the show buffer all the data
  // from one channel are next to each
  // other 

#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::fill_show_buffer()\n";
#endif

  page_ptr=buffer+(page*samples_per_page*num_channels);
  for(int i =0; i < grp_for_display.get_num_channels();i++)
    for(int j =0; j < samples_per_page; j++)
      show_buffer[(samples_per_page*i)+j]=
	page_ptr[(num_channels*j)+grp_for_display.get_channel_id(i)]*global_gain;
    

#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::fill_show_buffer()\n";
#endif

}

int oscilloscope::show_data(int page)
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::show_data()\n";
#endif
// to get an idea of drawing time
  struct timespec beginning_drawing,end_drawing,drawing_duration,data_crunch_end,data_crunch_duration,  rec; 
  int i,j,k;
  // do all the drawing here
  cairo_t * cr;
  cairo_t * buffer_cr;
  cairo_surface_t *buffer_surface;
  cairo_surface_t *drawable_surface;
  int width_start, height_start;
  int vertical_channel_space;
  int horizontal_channel_space;
  int start_index,end_index; // to get the min and max y value for each x coordinate from 0 to horizontal_channel_space
  double data_points_per_x_pixel;
  double pixels_per_data_point;
  int x_pixels_to_draw;
  clock_gettime(CLOCK_REALTIME, &beginning_drawing);


  if(grp_for_display.get_num_channels()<1)
    {
#ifdef DEBUG_OSC
      cerr << "oscilloscope::show_data(), no channel to draw\n";
#endif
      return 0;
    }

  fill_show_buffer(page);
      
  







#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::show_data()\n";
#endif

}
int oscilloscope::show_new_data()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::show_new_data()\n";
#endif
  // only show when there is a new full page
  if (new_samples_buffer!=samples_per_page)
    {
      return 0;
    }
  
  
  // show the current page
  show_data(current_page);

  // adjust some variable
  num_samples_displayed+=samples_per_page;
  new_samples_buffer=0;
  if(current_page<num_pages_buffer-1)
    current_page++;
  else
    current_page=0;
  if(pages_in_memory<num_pages_buffer-1)
    pages_in_memory++;
  
#ifdef DEBUG_OSC
  cerr << "current_page: " << current_page << "\n";
  cerr << "num_samples_displayed: " << num_samples_displayed << '\n';
#endif

  
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::show_new_data()\n";
#endif
  return 0;

}


int oscilloscope::get_data()
 {
   /************************************************
     buffer is divided in pages and we only read
     a page at a time and avoid wrap around buffer
   ************************************************/
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::get_data()\n";
#endif

  // 1. exit here if no new data available
  long int new_samples_available=db->get_number_samples_read()-(num_samples_displayed+new_samples_buffer);
  if(new_samples_available<=0)
    {
      // no new data available
      return 0;
    }
  // 2. check how many samples we would need to complete the page
  int samples_to_complete_page=samples_per_page-new_samples_buffer;
  unsigned long int first_sample = num_samples_displayed+new_samples_buffer;

#ifdef DEBUG_OSC
  cerr << "num_samples_displayed: " << num_samples_displayed << '\n';
  cerr << "db->get_number_samples_read(): " << db->get_number_samples_read() << '\n';
  cerr << "new_samples_buffer: " << new_samples_buffer << '\n';
  cerr << "new samples available: " << new_samples_available << '\n';
  cerr << "samples_to_complete_page: " << samples_to_complete_page << '\n';
  cerr << "first_sample: " << first_sample << '\n';

  
#endif


  // 3. read the new data, maximum up to complete the current page, all channels
  buffer_ptr=buffer+(current_page*samples_per_page*num_channels)+(new_samples_buffer*num_channels);
  int samples_returned=db->getNewData(first_sample,buffer_ptr,samples_to_complete_page,num_channels,all_channels_list);
  if(samples_returned<0)
    {
      cerr << "oscilloscope::get_data(), problem getting new data\n";
      return -1;
    }
  
  new_samples_buffer=new_samples_buffer+samples_returned;


#ifdef DEBUG_OSC
  cerr << "samples_returned: " << samples_returned << '\n';
  cerr << "new_samples_buffer: " << new_samples_buffer << '\n';
  cerr << "leaving oscilloscope::get_data()\n";
#endif
  return 0;
}



void oscilloscope::reset()
{
  #ifdef DEBUG_OSC
  cerr << "entering oscilloscope::reset()\n";
#endif

  samples_per_page=sampling_rate*seconds_per_page;
  num_channels=db->getNumChannels();
  page_size=samples_per_page*num_channels;
  num_pages_buffer=buffer_size/(num_channels*samples_per_page);
  max_samples_buffer=buffer_size/num_channels;
  current_page=0;
  new_samples_buffer=0; // undisplayed data
  num_samples_displayed=db->get_number_samples_read();
  pages_in_memory=0;
  displayed_pages=0;

#ifdef DEBUG_OSC
  cerr << "num_samples_displayed: " << num_samples_displayed << '\n';
  cerr << "leaving oscilloscope::reset()\n";
#endif
}

bool oscilloscope::start_oscilloscope()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::start_oscilloscope()\n";
#endif
  reset();
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
	{
	  grp[i].set_channel_id(j,i*DEFAULT_CHANNELS_PER_GROUP+j);
	  grp[i].set_colours(j,1.0,0,0);
	}
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
  
  if(is_displaying==false)
    {
      refresh();
    }


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
