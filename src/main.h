/****************************************************************
Copyright (C) 2010 Kevin Allen

This file is part of kacq.

kacq is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
kacq is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with kacq.  If not, see <http://www.gnu.org/licenses/>.

File with declarations of the main structures and functions used in kacq


****************************************************************/
#include <comedi.h> // for the driver
#include <comedilib.h> // for the driver API
#include <stdio.h>
#include <fcntl.h> // for file operations
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h> // for the nanosleep
#include <gtk/gtk.h> // for the widgets
#include <gdk/gdk.h>
#include <pthread.h> // to be able to create threads
#include <glib/gprintf.h>
#include <math.h>
#include <pwd.h> // to get the home directory as default directory
#include <stdlib.h>
#include <getopt.h>
#include <cairo.h>
#include "../config.h"


#define _FILE_OFFSET_BITS 64 // to have files larger than 2GB
#define NSEC_PER_SEC (1000000000) // The number of nsecs per sec
#define COMEDI_INTERFACE_MAX_DEVICES 2
#define MAX_BUFFER_LENGTH 100000 // buffer length for each comedi_dev
#define DEFAULT_SAMPLING_RATE 20000
#define MAX_SAMPLING_RATE 48000
#define COMEDI_DEVICE_MAX_CHANNELS 32
#define COMEDI_INTERFACE_TO_DEVICE_BUFFER_SIZE_RATIO 8 // size of comedi interface buffer, set according to device buffer size
#define COMEDI_INTERFACE_ACQUISITION_SLEEP_TIME_MS 1 // if too long could lead to buffer overflow, we make it short to be up to data often

#define FACTOR_RECORDING_BUFFER 6 // factor by which the recording_interface buffer > comedi_interface buffer */
#define RECORDING_PROPORTION_BUFFER_FILLED_BEFORE_SAVE 0.5 // the recording buffer will fill to that proportion before emptying it into the recording file
#define RECORDING_MAXIMUM_CHANNELS 64

#define OSCILLOSCOPE_GROUPS 10 // number of groups of channels that can be displayed
#define MAX_CHANNELS_PER_GROUP 16 // if too large, the oscilloscope can keep up with the information to display
#define DEFAULT_CHANNELS_PER_GROUP 8 // has to not larger than MAX_CHANNELS_PER_GROUP, 8 is reasonable as of 2012
#define MAX_TIME_SEC_IN_OSCILLOSCOPE_PAGE 2
#define MIN_TIME_SEC_IN_OSCILLOSCOPE_PAGE 0.03125 // for brain activity, that is about the fastest that makes any sense
#define OSCILLOSCOPE_DEFAULT_TIME_SEC_IN_PAGE 1 // will start at that value, refresh osciloscope every 1 sec
#define OSCILLOSCOPE_DEFAULT_GAIN .10
#define OSCILLOSCOPE_MAX_GLOBAL_GAIN 1
#define OSCILLOSCOPE_MIN_GLOBAL_GAIN .001
#define OSCILLOSCOPE_GLOBAL_GAIN_CHANGE_FACTOR 1.25
#define OSCILLOSCOPE_MAXIMUM_X_PIXEL_FOR_DRAWING_AREA 5000 // the x resolution of the scree should be lower than this number
#define TIME_SEC_IN_OSCILLOSCOPE_PAGE_CHANGE_FACTOR 2 // change of time resolution on oscilloscope is by factor of 2
#define FACTOR_OSCILLATORY_BUFFER 60 // that buffer is quite large to be able to go back in time and look back at signal
#define OSCILLOSCOPE_PIXELS_PER_DATA_POINT_TO_DRAW 1 // integer ranging from 1 to 10, to reduce drawing time
//#define DEBUG_REC // to turn on debugging output for recording
//#define DEBUG_OSC // to turn on debugging output for oscilloscope
//#define DEBUG_ACQ // to turn on debugging output for oscilloscope



/***************************************************
The following variables are for the program threads
Acquisition thread is stopped by setting comedi_interface.is_acquiring to 0
Recording thread is stopped by setting the recording_interface.is_recording to 0
****************************************************/
pthread_t recording_thread;
int recording_thread_id;
pthread_t oscilloscope_thread;
int oscilloscope_thread_id;
char* t;
/* mutex to prevent thread to read from a buffer that is being modified
 all parts of code reading or writing to comedi_inter.buffer_data should
 be surrounded by 
 pthread_mutex_lock( &mutex1 ); and   pthread_mutex_unlock( &mutex1 );
*/
pthread_mutex_t mutex_comedi_interface_buffer;





/***********************************************************************************
 structure that holds all the gui widgets that we need to control during execution
***********************************************************************************/
struct all_widget
{ // see kacq.glade file to know where these appear
  GtkWidget *window;  // main window
  GtkWidget *vbox1; // main vbox in the main window, to put the gtkdatabox
  GtkWidget *test_label;
  GtkWidget *drawing_area;
  GdkColor color;
  GtkAdjustment *sampling_rate_adjustment;
  GtkAdjustment *osc_group_adjustment;
  GtkAdjustment *osc_group_preferences_adjustment;
  GtkAdjustment *trial_no_adjustment;
  GtkWidget *about_dlg; // about dialog
  GtkWidget *acquisition_dlg;
  GtkWidget *oscilloscope_dlg;
  GtkWidget *recording_dlg;
  GtkWidget *toolbar;
  GtkWidget *dev1_name_label; // to show user the name of device
  GtkWidget *dev2_name_label; // to show user the driver
  GtkWidget *dev1_driver_label;
  GtkWidget *dev2_driver_label;
  GtkWidget *num_devices_detected_label;
  GtkWidget *sampling_rate_value_label;
  GtkWidget *num_available_channels_label;
  GtkWidget *num_channels_device_1_label;
  GtkWidget *num_channels_device_2_label;
  GtkWidget *range_label;
  GtkWidget *current_saving_directory_label2;
  GtkWidget *group_preferences_spinbutton;
  GtkWidget *preferences_channel_vbox; // to display the channel information
  GtkWidget *file_name_entry; // filebase of the file name
  GtkWidget *trial_spinbutton; // index following filebase for file name
  GtkWidget *statusbar; // index following filebase for file name   
  GtkWidget *group_spinbutton; // for oscilloscope display group
  GtkWidget *recording_channel_view; // treeview to select the channels to record
  GtkWidget *time_decrease_image;
  GtkWidget *time_increase_image;
  GtkWidget *gain_decrease_image;
  GtkWidget *gain_increase_image;
  GtkWidget *time_decrease_toolbutton;
  GtkWidget *time_increase_toolbutton;
  GtkWidget *gain_decrease_toolbutton;
  GtkWidget *gain_increase_toolbutton;

  GtkListStore  *recording_channel_store; //to fill up the treeview to select the recording channels
  GtkWidget *oscilloscope_all_channels_view; // treeview to select the channels to record
  GtkListStore  *oscilloscope_all_channels_store; // to fill the treeview to select the channels for oscilloscope
  GtkWidget *oscilloscope_selected_channels_view; // selected channels for a page of the oscilloscope
  GtkListStore *oscilloscope_selected_channels_store; // to fill the treeview for a group of the oscilloscope
  int is_filling_selected_channel_liststore;
};
struct all_widget widgets;

// enumeration to identify the columns in the model to select the channels to record
enum
{
  REC_COL_NO = 0,
  REC_COL_DEVICE,
  REC_COL_SELECT,
  REC_NUM_COLS
} ;
enum
{
  OSC_ALL_COL_NO = 0,
  OSC_ALL_COL_DEVICE,
  OSC_ALL_NUM_COLS
} ;


/******************************************************
 structure to hold the information regarding an AD card
Notes:
Acquisition always reads from all available channels

The selection of which channels are displayed or
recorded will not affect comedi_dev code

Each device has its own buffer to read data, but 
the data should always be accessed via the 
comedi_interface buffer
*******************************************************/
struct comedi_dev
{
  comedi_t *comedi_dev; // device itself
  char *file_name; // file name for access to device
  const char *name; // name of the card
  const char *driver; // name of the comedi driver
  int number_of_subdevices;
  int subdevice_analog_input; // id of analog input subdev
  int subdevice_analog_output; // id of analog output subdev
  int number_channels_analog_input;
  int number_channels_analog_output;
  int maxdata_input;
  int maxdata_output;
  int range_set_input; // index of the selected range
  int number_ranges_input;
  comedi_range ** range_input_array; // pointer to all the possible ranges on the card
  int range_set_output;
  int number_ranges_output;
  comedi_range ** range_output_array;
  double voltage_max_input;
  double voltage_max_output;
  int aref;
  int buffer_size;
  sampl_t buffer_data[MAX_BUFFER_LENGTH];
  sampl_t* pointer_buffer_data; // to accomodate for incomplete read sample
  int read_bytes;
  int samples_read;
  int data_point_out_of_samples; // because read operation returns incomplete samples
  long int cumulative_samples_read;
  comedi_cmd command; 
  unsigned int channel_list[COMEDI_DEVICE_MAX_CHANNELS]; // channel number for the comedi side
  int number_sampled_channels; // variable to be able to sample twice same channel on each sampling
  int is_acquiring;
};


/*********************************************************
structure to hold the information about all cards together

comed_interface has a buffer with the data from all
devices in it. The recording and gui part of the program
should only interface with comedi_interface and never directly 
with the comedi_dev.

*********************************************************/
struct comedi_interface
{
  int is_acquiring; // to send signal to the comedi thread
  int acquisition_thread_running; // set by the comedi thread when enter and exit
  int command_running;
  pthread_t acquisition_thread;
  int acquisition_thread_id;
  int number_devices; // counter of valid devices
  int sampling_rate; // sampling rate of recording
  int number_channels; // aggregate of two cards, calculate from dev[]->number_sampled_channels
  int max_number_samples_in_buffer;
  int min_samples_read_from_devices; // if compare the different devices
  unsigned long int number_samples_read; // total samples read
  int sample_no_to_add; // sample no of the newest sample in the buffer
  int samples_copied_at_end_of_buffer;
  int data_offset_to_dat; // to correct the data so that 0 is midpoint
  struct comedi_dev dev[COMEDI_INTERFACE_MAX_DEVICES]; // hold the info about each device
  unsigned int channel_list[COMEDI_DEVICE_MAX_CHANNELS*COMEDI_INTERFACE_MAX_DEVICES]; // hold the channel number user side
  int buffer_size;
  short int* buffer_data;// buffer to get data from comedi devices
  int data_points_move_back;
  int offset_move_back;
  struct timespec time_last_sample_acquired_timespec; 
  struct timespec inter_acquisition_sleep_timespec; // between read operations to driver
  double inter_acquisition_sleep_ms;
  struct timespec timespec_pause_restat_acquisition_thread; // allow acquisition to complete
  double pause_restart_acquisition_thread_ms;
  struct timespec req;
  };


/*********************************************************
Structure holding variables for the recording process
********************************************************/
struct recording_interface
{
  int is_recording; // to send signal to the recording thread
  int recording_thread_running; // set by the recording thread when enter and exit
  FILE *file;
  gchar* file_name; // directory + file name from gui
  gchar* directory; // 
  off_t file_size; // to check if the file size makes sense
  off_t predicted_file_size; // to check the size of file at the end
  int number_of_channels_to_save; // number of channels that will be saved
  unsigned int channel_list[RECORDING_MAXIMUM_CHANNELS]; // list of channels that will be saved
  int rec_buffer_size;
  int max_samples_in_buffer;
  double proportion_buffer_filled_before_save;
  short int* rec_buffer_data;
  struct timespec inter_recording_sleep_timespec; // sleeping time between attemping to save
  struct timespec start_recording_time_timespec;
  struct timespec now_timespec;
  struct timespec duration_recording_timespec;
  struct timespec req;
  double inter_recording_sleep_ms;
  // following variables that changes during recording process
  int new_samples_in_buffer;
  unsigned long int number_samples_saved; // this is the 0-based index of first sample in the recording buffer
  // context for statusbar, the status bar will show details on the recording currently in process
  guint statusbar_context_id;
  guint statusbar_message_id;
  double recording_time_sec;
};


/*********************************************************
Structure holding variables for the oscilloscope
********************************************************/
struct oscilloscope_interface
{
  int is_displaying; // is on
  int is_drawing; // in the drawing process
  int draw_only_mean;
  // a page is the data required to fill one screen of data, it has all the channels in it
  int number_pages_in_buffer; // this is for the display buffer, to display old data
			      // will vary according to the time resolution of oscilloscope
                              // with slow resolution, a page contains more data, so fewer pages in total
  int number_groups; // channels are always displayed by group of x channels 
  int max_channels_per_group; // to avoid having to much data to display on the oscilloscope
  int number_channels;
  
  // the osc_buffer has a copy of all the acquired channels, that will make simpler to code
  // it is devided into a number of pages, each page contains the data to fill a display
  // the data should be organized differently from the other buffers.
  // all samples from one channels to fill a page are together, followed by other channels of same page
  // that way we can fill the graph without having to move data around
  gfloat* osc_buffer; // main buffer with data from all channels, with all pages
  int buffer_size;
  gfloat* show_buffer; // data with gain to be displayed on the oscilloscope
  int show_buffer_size;
  gfloat* y_min_for_pixel_x; // for drawing
  gfloat* y_max_for_pixel_x; // for drawing
  gfloat* mean_for_pixel_x; // for drawing
  int pixels_per_data_point_to_draw; // number of screen pixels per data point drawn on the screen
  gfloat* x_axis_data;
  gfloat** pointer_draw_data; 
  gfloat* page_pointer; // pointer to the right page of data within the osc_buffer
  

  double seconds_per_page;
  int samples_per_page;
  int page_size;
  int max_samples_in_buffer;
  
  // variables changing as we get data
  int new_samples_in_buffer; // undisplayed data
  unsigned long int number_samples_displayed; // displayed data
  int current_page;// page in which we are adding data
  int acq_last_sample_no_needed;

  // variables to set the display
  double global_gain;
  double min_global_gain;
  double max_global_gain;
  double global_gain_factor;

  int current_group;
  int* number_channels_per_group; // one dimentional array
  int** channel_list_in_group; // 2D array
  gfloat** gain;
  gfloat** offset; 
  float** channel_red; 
  float** channel_green; 
  float** channel_blue; 
  
  // variables setting sleeping time between attemps to get new data from acquisition buffer
  struct timespec inter_displaying_sleep_timespec; // sleeping time between attemping to save
  struct timespec req;
  double inter_displaying_sleep_ms;

  // variables directly modified by user at any time
  double gui_seconds_per_page;
  double gui_global_gain;
  int gui_current_group;

  int pages_in_memory; // to go back at previous data
  int displayed_page;

};



// 3 structures holding most of internal variables
struct comedi_interface comedi_inter;
struct recording_interface recording_inter;
struct oscilloscope_interface osc_inter;

GtkBuilder *builder; // to build the interface from glade


/***********************
 defined in callback.c
***********************/
void on_window_destroy (GtkObject *object, gpointer user_data);
// menuitems

void on_quit_menuitem_activate(GtkObject *object, gpointer user_data);
void on_about_menuitem_activate(GtkObject *object, gpointer user_data);
void on_preferences_menuitem_activate(GtkObject *object, gpointer user_data);
// about dialot
void on_about_dialog_delete_event(GtkObject *object, gpointer user_data);
// preferences dialog
void on_preferences_dialog_delete_event(GtkObject *object, gpointer user_data);
void on_ok_preferences_button_activate(GtkObject *object, gpointer user_data);
void cell_toggled_callback (GtkCellRendererToggle *cell,gchar *path_string, gpointer user_data);
// acquisition dialog
void on_acquisition_menuitem_activate(GtkObject *object, gpointer user_data);
void on_acquisition_dialog_delete_event(GtkObject *object, gpointer user_data);
void on_ok_acquisition_button_clicked(GtkObject *object, gpointer user_data);
// oscilloscope dialog
void on_oscilloscope_menuitem_activate(GtkObject *object, gpointer user_data);
void on_ok_oscilloscope_button_clicked(GtkObject *object, gpointer user_data);
void on_oscilloscope_dialog_delete_event(GtkObject *object, gpointer user_data);
// recording dialog
void on_recording_menuitem_activate(GtkObject *object, gpointer user_data);
void on_ok_recording_button_clicked(GtkObject *object, gpointer user_data);
void on_recording_dialog_delete_event(GtkObject *object, gpointer user_data);



// toolbar buttons
void on_rewind_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_forward_toolbutton_clicked(GtkObject *object, gpointer user_data);

void on_play_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_stop_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_record_toolbutton_clicked(GtkObject *object, gpointer user_data);

void on_gain_increase_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_gain_decrease_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_time_increase_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_time_decrease_toolbutton_clicked(GtkObject *object, gpointer user_data);

// to redraw the drawing area (oscilloscope on expose events)
void on_drawing_area_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);

// to select the saving directory
void on_directory_button_clicked(GtkObject *object, gpointer user_data);
// to select thd group of channels to display on oscilloscope
void on_group_spinbutton_change_value(GtkObject *object, gpointer user_data);
void on_group_spinbutton_value_changed(GtkObject *object, gpointer user_data); // from the main window
void on_osc_group_preference_spinbutton_value_changed(GtkObject *object, gpointer user_data); // from the oscilloscope dialog
// to select the channels to record
void on_add_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_remove_toolbutton_clicked(GtkObject *object, gpointer user_data);

void on_osc_remove_toolbutton_clicked(GtkObject *object, gpointer user_data);
void on_osc_add_toolbutton_clicked(GtkObject *object, gpointer user_data);

void view_selected_add_foreach_func (GtkTreeModel  *model,GtkTreePath   *path, GtkTreeIter   *iter, gpointer       userdata);
void view_selected_remove_foreach_func (GtkTreeModel  *model,GtkTreePath   *path, GtkTreeIter   *iter, gpointer       userdata);
// functions called at startup, to initialize variables
int init_window(); // to complete building the interface
int set_acquisition_labels(); // to set the labels in the comedi preferences dialog
void set_recording_labels();

// model and tree view to select channels for recording
int build_recording_channel_tree_view();
void create_and_fill_recording_model();

// models and tree views used to group channels for the oscilloscope
int build_oscilloscope_all_channels_tree_view();
void create_and_fill_oscilloscope_all_channels_model();
int build_oscilloscope_selected_channels_tree_view();
void fill_oscilloscope_selected_channels_model();
void add_selected_channel_to_selection_foreach_func(GtkTreeModel *model,GtkTreePath *path, GtkTreeIter *iter, gpointer userdata);
void on_selected_channel_row_changed(GtkObject *object, gpointer user_data);


// function to run kacq in the terminal, defined in callback
int read_configuration_file(char * file_name,struct comedi_interface* com, struct recording_interface* rec);
int recording_terminal_mode(struct comedi_interface* com,struct recording_interface* rec);

// functions used to get the time of some operations
struct timespec set_timespec_from_ms(double milisec);
struct timespec diff(struct timespec* start, struct timespec* end);
int microsecond_from_timespec(struct timespec* duration);


/************************
defined in recording.c
************************/
int recording_interface_init(struct recording_interface* rec);
int recording_interface_free(struct recording_interface* rec);
int recording_interface_open_file(struct recording_interface* rec);
int recording_interface_get_data(struct recording_interface* rec, struct comedi_interface* com);
int recording_interface_save_data(struct recording_interface* rec);
int recording_interface_save_data_flush(struct recording_interface* rec);
int recording_interface_close_file(struct recording_interface* rec);
int recording_interface_check_file_size(struct recording_interface* rec);
int recording_interface_clear_current_recording_variables(struct recording_interface* rec);
int recording_interface_get_channels_from_recording_channel_store(struct recording_interface* rec,struct comedi_interface* com, GtkListStore  *channel_store);
int recording_interface_set_recording_channels(struct recording_interface* rec, struct comedi_interface* com,int number_channels, int* channel_list);
int recording_interface_set_recording_time(struct recording_interface* rec, double recording_time_sec);
int recording_interface_set_data_file_name(struct recording_interface* rec, char* data_file_name);
int recording_interface_start_recording(struct recording_interface* rec);
int recording_interface_stop_recording(struct recording_interface* rec);
void *recording(void* recording_inter); // thread that does the acquisition, the recording inerface needs to be passed to this function

/*************************
 define in comedi_code.c
**************************/
int comedi_interface_init(); // get the information on the AD cards installed
int comedi_interface_free(struct comedi_interface* com); 
int comedi_interface_set_channel_list();
int comedi_interface_build_command();
int comedi_interface_run_command();
int comedi_interface_stop_command();
int comedi_interface_get_data_from_devices();
int comedi_interface_clear_current_acquisition_variables();
int comedi_interface_start_acquisition(struct comedi_interface* com);
int comedi_interface_stop_acquisition(struct comedi_interface* com);
int comedi_interface_set_sampling_rate (struct comedi_interface* com, int req_sampling_rate);
int comedi_interface_print_info(struct comedi_interface* com);
unsigned long int comedi_interface_get_data_one_channel(struct comedi_interface* com,int channel_no,int number_samples,double* data,struct timespec* time_acquisition);
int comedi_dev_init(struct comedi_dev *dev, char* file_name);
int comedi_dev_free(struct comedi_dev *dev);
int comedi_dev_read_data(struct comedi_dev *dev);
int comedi_dev_adjust_data_point_out_of_samples(struct comedi_dev *dev,int number_samples_transfered_to_inter_buffer);
int comedi_dev_clear_current_acquisition_variables(struct comedi_dev *dev);
int comedi_t_enable_master(comedi_t *dev);
int comedi_t_enable_slave(comedi_t *dev);
int comedi_device_print_info(struct comedi_dev* dev);
void * acquisition(void* comedi_inter); // thread that does the acquisition, the comedi interface needs to be passed to this function

/********************************
defined in oscilloscope.c
********************************/
int oscilloscope_interface_init(struct oscilloscope_interface* osc, struct comedi_interface* com);
int oscilloscope_interface_free(struct oscilloscope_interface* osc);
int oscilloscope_interface_set_default_group_values(struct oscilloscope_interface* osc);
int oscilloscope_interface_get_data(struct oscilloscope_interface* osc, struct comedi_interface* com);
int oscilloscope_interface_show_new_data(struct oscilloscope_interface* osc);
int oscilloscope_interface_refresh_display(struct oscilloscope_interface* osc);
int oscilloscope_interface_show_data(struct oscilloscope_interface* osc,int page);
int oscilloscope_interface_reset_current_variables(struct oscilloscope_interface* osc,struct comedi_interface* com);
int oscilloscope_interface_update_time_gain(struct oscilloscope_interface* osc,struct comedi_interface* com);
int oscilloscope_interface_update_group(struct oscilloscope_interface* osc,struct comedi_interface* com);
int oscilloscope_interface_draw_grid(struct oscilloscope_interface* osc, GtkWidget *widget, cairo_t *cr);
int oscilloscope_interface_start_oscilloscope(struct oscilloscope_interface* com);
int oscilloscope_interface_stop_oscilloscope(struct oscilloscope_interface* com);
int oscilloscope_interface_fill_show_buffer(struct oscilloscope_interface* osc,int page);
int oscilloscope_interface_update_channel_list_in_group_from_liststore(struct oscilloscope_interface* osc,GtkListStore *oscilloscope_selected_channels_store,int group);


int oscilloscope_interface_increase_time_resolution(struct oscilloscope_interface* osc, struct comedi_interface* com);
int oscilloscope_interface_decrease_time_resolution(struct oscilloscope_interface* osc, struct comedi_interface* com);
int oscilloscope_interface_increase_global_gain(struct oscilloscope_interface* osc);
int oscilloscope_interface_decrease_global_gain(struct oscilloscope_interface* osc);
int oscilloscope_interface_show_previous_page(struct oscilloscope_interface* osc);
int oscilloscope_interface_show_next_page(struct oscilloscope_interface* osc);
//void *oscilloscope(void* osc_inter);
static gboolean oscilloscope_interface_timer_update();


