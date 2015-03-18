//#define DEBUG_OSC
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


oscilloscope::oscilloscope(dataBuffer* datab,Gtk::DrawingArea* da)
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::oscilloscope()\n";
#endif

  db=datab;
  drawing_area=da;
  // Allocate memory for an internal buffer that can be used by other objects 
  buffer_size= OSC_BUFFER_LENGTH_SAMPLES*OSC_MAXIMUM_CHANNELS; // to rewind
  buffer = new double [buffer_size];
  show_buffer_size =  OSC_SHOW_BUFFER_LENGTH_SAMPLES*OSC_MAXIMUM_CHANNELS;
  show_buffer = new double [show_buffer_size];

  blue = new double[OSC_MAXIMUM_CHANNELS];
  red = new double[OSC_MAXIMUM_CHANNELS];
  green = new double[OSC_MAXIMUM_CHANNELS];


  is_drawing=false;
  is_displaying=false;
  draw_only_mean=false;
  
  num_channels=db->getNumChannels();
  cerr << "oscilloscope number of channels: " << num_channels << '\n';
  all_channels_list = new unsigned int [num_channels];
  for(int i = 0; i < num_channels;i++)
    all_channels_list[i]=i;

  num_groups= OSC_GROUPS;
  max_channels_per_group = MAX_CHANNELS_PER_GROUP;
  grp = new channelGroup[num_groups];
  current_group=0;
  set_channel_group_default();
  set_default_colours();
  sampling_rate=db->get_sampling_rate();
  seconds_per_page=OSCILLOSCOPE_DEFAULT_TIME_SEC_IN_PAGE;
  gui_seconds_per_page=seconds_per_page;
  samples_per_page=sampling_rate*seconds_per_page;
  page_size=samples_per_page*num_channels;
  num_pages_buffer=buffer_size/page_size;
  current_page=0;
  pages_in_memory=0;
  displayed_page=0;
  pixels_per_data_point_to_draw=OSCILLOSCOPE_PIXELS_PER_DATA_POINT_TO_DRAW;
  x_margin_left=OSCILLOSCOPE_X_MARGIN_LEFT;
  x_margin_right=OSCILLOSCOPE_X_MARGIN_RIGHT;
  y_margin_top=OSCILLOSCOPE_Y_MARGIN_TOP;
  y_margin_bottom=OSCILLOSCOPE_Y_MARGIN_BOTTOM;

  x_axis_data= new double[OSCILLOSCOPE_MAX_SAMPLING_RATE*MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE];
  for (int i=0; i<OSCILLOSCOPE_MAX_SAMPLING_RATE*MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE;i++)
    x_axis_data[i]=i;


  y_min_for_pixel_x = new double[OSCILLOSCOPE_MAXIMUM_X_PIXEL_FOR_DRAWING_AREA];
  y_max_for_pixel_x = new double[OSCILLOSCOPE_MAXIMUM_X_PIXEL_FOR_DRAWING_AREA];
  mean_for_pixel_x = new double[OSCILLOSCOPE_MAXIMUM_X_PIXEL_FOR_DRAWING_AREA];
  
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
  
  factor_microvolt=OSC_FACTOR_MICROVOLT;
  
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
  

  stop_oscilloscope();
  
  delete[] buffer;
  delete[] show_buffer;
  delete[] grp;
  delete[] all_channels_list;
  delete[] y_min_for_pixel_x;
  delete[] y_max_for_pixel_x;
  delete[] mean_for_pixel_x;
  delete[] blue;
  delete[] red;
  delete[] green;
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
  
  
  // gui can affect time and gain only when this function is called
  // prevent gui changes from affecting drawing until it is completed
  update_time_gain();


  // get a copy of the display group so that any change during 
  // the displaying process does not cause segmentation faults
  // so for displaying use grp_for_display
  grp[current_group].copy_channelGroup(grp_for_display);


  
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

  if(gui_global_gain*global_gain_factor<=max_global_gain)
    {
      gui_global_gain=gui_global_gain*global_gain_factor;
    }
#ifdef DEBUG_OSC
  cerr << "gui global gain: " << gui_global_gain << '\n';
#endif 
}
void oscilloscope::decrease_gain()
{
 if(gui_global_gain/global_gain_factor>=min_global_gain)
    {
      gui_global_gain=gui_global_gain/global_gain_factor;
    }
#ifdef DEBUG_OSC
  cerr << "gui global gain: " << gui_global_gain << '\n';
#endif

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
  
  grp[current_group].copy_channelGroup(grp_for_display);
  show_data(displayed_page); 

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
  Cairo::RefPtr<Cairo::Context> cr;
  Gtk::Allocation allocation;
  Cairo::RefPtr<Cairo::Context> buffer_cr;
  Cairo::RefPtr<Cairo::Surface> drawable_surface;
  Cairo::RefPtr<Cairo::Surface> buffer_surface;
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
  
  allocation = drawing_area->get_allocation();
  cr = drawing_area->get_window()->create_cairo_context();
  width_start = allocation.get_width();
  height_start = allocation.get_height();

  if(width_start>OSCILLOSCOPE_MAXIMUM_X_PIXEL_FOR_DRAWING_AREA)
    {
      cerr << "oscilloscope::show_data: width of drawing_area is larger than OSCILLOSCOPE_MAXIMUM_X_PIXEL_FOR_DRAWING_AREA\n";
      return -1;
    }

  drawable_surface = cr->get_target();

  // we will draw in a buffer surface and then copy to drawing_area, for smoother oscilloscope
  buffer_surface= drawable_surface->create(drawable_surface,
  					   Cairo::CONTENT_COLOR_ALPHA,
					   width_start,
					   height_start);
  buffer_cr=Cairo::Context::create(buffer_surface);


  // get the vertical space allocated for each channel in the current group
  vertical_channel_space=(height_start - y_margin_top - y_margin_bottom)/grp_for_display.get_num_channels();
  horizontal_channel_space=width_start-x_margin_left-x_margin_right;
  x_pixels_to_draw=horizontal_channel_space/pixels_per_data_point_to_draw; // to reduce drawing time, and x resolution.
  data_points_per_x_pixel=(double)samples_per_page/x_pixels_to_draw;
  pixels_per_data_point=(double)x_pixels_to_draw/samples_per_page;


  buffer_cr->set_source_rgb(0.9, 0.9, 0.9);
  buffer_cr->paint();


 // for each channel
  buffer_cr->set_line_width(1.0);
  if(data_points_per_x_pixel>=1) // more than one data point per pixel in the screen
    {
      for (i=0;i<grp_for_display.get_num_channels();i++)
	{
	  buffer_cr->set_source_rgb(red[grp_for_display.get_channel_id(i)],
			     green[grp_for_display.get_channel_id(i)],
			     blue[grp_for_display.get_channel_id(i)]);
	  
	  // get the minimum y and maximum y and mean y for each x coordinate on the screen
	  for (j=0;j<x_pixels_to_draw;j++)
	    {
	      start_index=(int)round(j*data_points_per_x_pixel);
	      end_index=(int)round(start_index+data_points_per_x_pixel);
	      y_min_for_pixel_x[j]=show_buffer[(samples_per_page*i)+start_index];
	      y_max_for_pixel_x[j]=show_buffer[(samples_per_page*i)+start_index];
	      mean_for_pixel_x[j]=0;
	      for (k=start_index;k<end_index;k++)
		{
		  if(y_max_for_pixel_x[j]<show_buffer[(samples_per_page*i)+k])
		    {
		      y_max_for_pixel_x[j]=show_buffer[(samples_per_page*i)+k];
		    }
		  if(y_min_for_pixel_x[j]>show_buffer[(samples_per_page*i)+k])
		    {
		      y_min_for_pixel_x[j]=show_buffer[(samples_per_page*i)+k];
		    }
		  mean_for_pixel_x[j]+=show_buffer[(samples_per_page*i)+k];
		}
	      mean_for_pixel_x[j]=mean_for_pixel_x[j]/(end_index-start_index);
	    }
	  /*to draw the mean */
	  for (j=1;j<x_pixels_to_draw;j++)
	    {
	      if (j==0) 
	      	{

		  buffer_cr->move_to(x_margin_left+j*pixels_per_data_point_to_draw,
			      (int)((vertical_channel_space*i+vertical_channel_space/2+y_margin_top)+(mean_for_pixel_x[j])));
	      	}
	      else
	      	{
		  buffer_cr->line_to(x_margin_left+j*pixels_per_data_point_to_draw,
			      (int)((vertical_channel_space*i+vertical_channel_space/2+y_margin_top)+(mean_for_pixel_x[j])));
	      	}
	    }
	  if(draw_only_mean==0)
	    {
	      /* draw from max to min */
	      for (j=1;j<x_pixels_to_draw;j++)
		{
		  buffer_cr->move_to(x_margin_left+j*pixels_per_data_point_to_draw,
			      (int)((vertical_channel_space*i+vertical_channel_space/2+y_margin_top)+(y_max_for_pixel_x[j])));
		  buffer_cr->line_to(x_margin_left+j*pixels_per_data_point_to_draw,
			      (int)((vertical_channel_space*i+vertical_channel_space/2+y_margin_top)+(y_min_for_pixel_x[j])));
		}
	    }
	  buffer_cr->stroke();
	}
    }
  else // less than a data point per pixel in the screen
    {
      
      for (i=0;i<grp_for_display.get_num_channels();i++)
	{
	
	  buffer_cr->set_source_rgb(red[grp_for_display.get_channel_id(i)],
	  		     green[grp_for_display.get_channel_id(i)],
			     blue[grp_for_display.get_channel_id(i)]);
	  
	  // get the minimum y and maximum y and mean y for each x coordinate on the screen
	  for (j=0;j<samples_per_page;j++)
	    {
	      if (j==0) 
	      	{
		  buffer_cr->move_to(x_margin_left+j*pixels_per_data_point,
			      (int)((vertical_channel_space*i+vertical_channel_space/2+y_margin_top)+show_buffer[(samples_per_page*i)+j]));
	      	}
	      else
	      	{
		  buffer_cr->line_to(x_margin_left+j*pixels_per_data_point,
			      (int)((vertical_channel_space*i+vertical_channel_space/2+y_margin_top)+show_buffer[(samples_per_page*i)+j]));
	      	}
	    }
	  buffer_cr->stroke();
	}
    }
  

  draw_grid(buffer_cr);

  cr->set_source(buffer_surface,0,0);  
  cr->paint();



  clock_gettime(CLOCK_REALTIME, &end_drawing);
  drawing_duration=tk.diff(&beginning_drawing,&end_drawing);
  displayed_page=page;
  
#ifdef DEBUG_OSC
  cerr << "Drawing time : " << drawing_duration.tv_sec*1000+drawing_duration.tv_nsec/1000000.0 << "ms\n";
  cerr << "leaving oscilloscope::show_data()\n";
#endif

  return 0;


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
  displayed_page=current_page;
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


  // 3. read the new data, maximum up to complete the current page, all channels, in microvolts
  buffer_ptr=buffer+(current_page*samples_per_page*num_channels)+(new_samples_buffer*num_channels);
  int samples_returned=db->getNewData(first_sample,buffer_ptr,samples_to_complete_page,num_channels,all_channels_list,factor_microvolt);
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
  displayed_page=0;

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
  usleep(50000); // allow thread to die

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
  int chan;
  // set default values in channel groups
  for(int i = 0; i < num_groups; i++)
    {
      grp[i].set_num_channels(DEFAULT_CHANNELS_PER_GROUP);
      for(int j = 0; j < DEFAULT_CHANNELS_PER_GROUP; j++)
	{
	  chan=i*DEFAULT_CHANNELS_PER_GROUP+j;
	  while(chan>=num_channels)
	    {chan=chan-num_channels;}
	  grp[i].set_channel_id(j,chan);
	}
    }
  
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::set_channel_group_default()\n";
#endif
}



void oscilloscope::set_default_colours()
{
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::set_default_colours()\n";
#endif
  // set default values in channel groups
  int grouping=4;
  int tetrode,remaining;
  double num_tetrodes=OSC_MAXIMUM_CHANNELS/grouping;
  for(int i = 0; i < OSC_MAXIMUM_CHANNELS; i++)
    {
      tetrode=i/grouping;
      remaining=tetrode%8;
      switch(remaining)
	{
	case 0:
	  red[i]=0.6-((double)tetrode/num_tetrodes)*0.6;
	  green[i]=0.0;
	  blue[i]=0.6;
	  break;
	case 1:
	  red[i]=0+((double)tetrode/num_tetrodes)*0.6;
	  green[i]=0.5;
	  blue[i]=0.6;
	  break;
	case 2:
	  red[i]=0.6-((double)tetrode/num_tetrodes)*0.6;
	  green[i]=0.6;
	  blue[i]=0.0;
	  break;
	case 3:
	  red[i]=0+((double)tetrode/num_tetrodes)*0.6;
	  green[i]=0.6;
	  blue[i]=0.0;
	  break;
	case 4:
	  red[i]=0.0;
	  green[i]=0.0;
	  blue[i]=0.0;
	  break;
	case 5:
	  red[i]=0.5;
	  green[i]=0+((double)tetrode/num_tetrodes)*0.6;
	  blue[i]=0.6;
	  break;
	case 6:
	  red[i]=0.3;
	  green[i]=0.6-((double)tetrode/num_tetrodes)*0.6;
	  blue[i]=0.3;
	  break;
	case 7:
	  red[i]=0.2;
	  green[i]=0+((double)tetrode/num_tetrodes)*0.6;
	  blue[i]=0.5;
	  break;
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
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::set_current_group()\n";
#endif
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
  
  refresh();
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::set_current_group(), group set to " << current_group << "\n";
#endif
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
void oscilloscope::draw_grid(Cairo::RefPtr<Cairo::Context> cr)
{
  
  int width, height,i;
  double vertical_channel_space,horizontal_channel_space;
  int writing_y=30;
  Gtk::Allocation allocation;
  // get the size of the drawing area
  
  allocation = drawing_area->get_allocation();
  width = allocation.get_width();
  height = allocation.get_height();
  
  // get the vertical space allocated for each channel in the current group
  vertical_channel_space=(height - y_margin_top - y_margin_bottom)/grp_for_display.get_num_channels();
  horizontal_channel_space=width-x_margin_left-x_margin_right;



  // write the channel number on the left side of the drawing area
  cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL,Cairo::FONT_WEIGHT_NORMAL);
  cr->set_font_size (14.0);
  cr->set_source_rgb (0, 0, 0);
  for (i=0;i<grp_for_display.get_num_channels();i++)
    {
      cr->move_to(5,
		  (i*vertical_channel_space)+(vertical_channel_space/2) + y_margin_top);
      cr->show_text (g_strdup_printf("%d",grp_for_display.get_channel_id(i)));
    }
	
        
  // // write the range in V for the channels
  // // code is ugly because need access to comedi interface to know the range
  // // now just give one number for all channels because user not allowed to change it for each channel individually
  // double range_v=comedi_inter.dev[0].range_input_array[comedi_inter.dev[0].range_set_input]->max - comedi_inter.dev[0].range_input_array[comedi_inter.dev[0].range_set_input]->min;
  // double visual_range_one_channel=(double)range_v/65535.00*vertical_channel_space/osc->global_gain/osc->gain[osc->current_group][0];// should be in V
  // cairo_move_to(cr, 5,writing_y);
  // cairo_show_text (cr, g_strdup_printf("%.2fV",visual_range_one_channel));
  
  // //  printf("global_gain: %f, gui_global_gain: %f, gain: %f, visual_range_one_channel: %f, vertical_channel_space: %d \n",osc->global_gain, osc->gui_global_gain, osc->gain[osc->current_group][0],visual_range_one_channel,vertical_channel_space); 

  // // for each channel individually it would be something like this
  // /* for (i=0;i<osc->number_channels_per_group[osc->current_group];i++) */
  // /*   {  */
  // /*     visual_range_one_channel=(double)range_v/65535.00*vertical_channel_space*osc->global_gain*osc->gain[osc->current_group][i]*1000; */
  // /*     cairo_move_to(cr, 5, (i*vertical_channel_space)+(vertical_channel_space/2)+15); */
  // /*     cairo_show_text (cr, g_strdup_printf("%f mV",visual_range_one_channel)); */

  // /*   } */


  // write the number of sec per page
  cr->move_to(x_margin_left+(horizontal_channel_space/10*9.2),writing_y);
  cr->show_text (g_strdup_printf("%.1lf ms",seconds_per_page*1000));

  // write the global gain
  cr->move_to(x_margin_left+(horizontal_channel_space/10*5),writing_y);
  cr->show_text (g_strdup_printf("gain: %.4lf",global_gain));
  

     
  // draw an horizontal tic between each channel at the right and left of the oscilloscope
  cr->set_source_rgb(0.5, 0.5, 0.5);
  cr->set_line_width(2);
  for (i=0;i<grp_for_display.get_num_channels()+1;i++)
     {
       // left side
       cr->move_to(0, (i*vertical_channel_space)+y_margin_top);
       cr->rel_line_to (10, 0);
     // right side
       cr->move_to(width-10, (i*vertical_channel_space)+y_margin_top);
       cr->rel_line_to(10, 0);
   }
  cr->stroke();
      
  // draw 9 vertical tics that will separate the oscilloscope region in 10 equal parts, at top and bottom
  for (i=0; i < 11; i++)
    {
      cr->move_to(x_margin_left+(i*horizontal_channel_space/10), 0);
      cr->rel_line_to(0, 10);
      cr->move_to(x_margin_left+(i*horizontal_channel_space/10), height);
      cr->rel_line_to(0, -10);
    }
  
  cr->stroke();
  cr->set_line_width(1.0);

}
 void oscilloscope::show_previous_page()
 {
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::show_previous_page()\n";
#endif
  int page;
  // by default, show the page currently on displayed
  page=displayed_page;
  
  if(displayed_page<current_page)
    {
      if(displayed_page>0&&current_page-displayed_page<pages_in_memory )
	{
	  page=displayed_page-1;
	}
      if(displayed_page==0)
	{
	  if(num_pages_buffer-1>current_page && pages_in_memory>current_page)
	    {
	      page=num_pages_buffer-1;
	    }
	}
    }                                                     
  if(displayed_page-1>current_page && current_page+(num_pages_buffer-displayed_page)<pages_in_memory)
    {
      page=displayed_page-1;
    }

#ifdef DEBUG_OSC
  fprintf(stderr,"show page %d\n", page);
#endif
  show_data(page);
  

#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::show_previous_page()\n";
#endif
}
 void oscilloscope::show_next_page()
 {
#ifdef DEBUG_OSC
  cerr << "entering oscilloscope::show_next_page()\n";
#endif
  // by default, show the page currently on displayed
  int page=displayed_page;
  if(displayed_page+1<current_page)
    {
      page=displayed_page+1;
    }
  if(displayed_page>current_page)
    {
      if (displayed_page+2<num_pages_buffer)
	page=displayed_page+1;
      else
	page=0;
    }

#ifdef DEBUG_OSC
  fprintf(stderr,"show page %d\n", page);
#endif

  show_data(page);
  
#ifdef DEBUG_OSC
  cerr << "leaving oscilloscope::show_next_page()\n";
#endif
}
