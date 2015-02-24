#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <gtkmm.h>
#include "rhd2000evalboard.h"
#include "acquisition.h"
#include "recording.h"
#include "timeKeeper.h"
#include "dataBuffer.h"


class mainWindow: public Gtk::Window
{
 private:
  acquisition* acq;
  dataBuffer* db; // data buffer not deutsche bahn
  recording* rec;
  pthread_t acquisition_thread;
  int acquisition_thread_id;
  pthread_t recording_thread;
  int recording_thread_id;
  
 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();
  
 protected:
  Glib::RefPtr<Gtk::Builder> builder;

  // widgets
  Gtk::ToggleToolButton* play_toolbutton;
  Gtk::ToggleToolButton* record_toolbutton;
  Gtk::ToolButton*  rewind_toolbutton;
  Gtk::ToolButton*  forward_toolbutton;
  Gtk::ToolButton*  gain_increase_toolbutton;
  Gtk::ToolButton*  gain_decrease_toolbutton;
  Gtk::ToolButton*  time_increase_toolbutton;
  Gtk::ToolButton*  time_decrease_toolbutton;

  Gtk::MenuItem*  about_menuitem;
  Gtk::AboutDialog* about_dialog;



  /* GtkWidget *window;  // main window */
  /* GtkWidget *vbox1; // main vbox in the main window, to put the gtkdatabox */
  /* GtkWidget *test_label; */
  /* GtkWidget *drawing_area; */
  /* GdkColor color; */
  /* GtkAdjustment *sampling_rate_adjustment; */
  /* GtkAdjustment *osc_group_adjustment; */
  /* GtkAdjustment *osc_group_preferences_adjustment; */
  /* GtkAdjustment *trial_no_adjustment; */
  /* GtkWidget *about_dlg; // about dialog */
  /* GtkWidget *acquisition_dlg; */
  /* GtkWidget *oscilloscope_dlg; */
  /* GtkWidget *recording_dlg; */
  /* GtkWidget *toolbar; */
  /* GtkWidget *dev1_name_label; // to show user the name of device */
  /* GtkWidget *dev2_name_label; // to show user the driver */
  /* GtkWidget *dev1_driver_label; */
  /* GtkWidget *dev2_driver_label; */
  /* GtkWidget *num_devices_detected_label; */
  /* GtkWidget *sampling_rate_value_label; */
  /* GtkWidget *num_available_channels_label; */
  /* GtkWidget *num_channels_device_1_label; */
  /* GtkWidget *num_channels_device_2_label; */
  /* GtkWidget *range_label; */
  /* GtkWidget *current_saving_directory_label2; */
  /* GtkWidget *group_preferences_spinbutton; */
  /* GtkWidget *preferences_channel_vbox; // to display the channel information */
  /* GtkWidget *file_name_entry; // filebase of the file name */
  /* GtkWidget *trial_spinbutton; // index following filebase for file name */
  /* GtkWidget *statusbar; // index following filebase for file name    */
  /* GtkWidget *group_spinbutton; // for oscilloscope display group */
  /* GtkWidget *recording_channel_view; // treeview to select the channels to record */
  /* GtkWidget *time_decrease_image; */
  /* GtkWidget *time_increase_image; */
  /* GtkWidget *gain_decrease_image; */
  /* GtkWidget *gain_increase_image; */
  /* GtkWidget *time_decrease_toolbutton; */
  /* GtkWidget *time_increase_toolbutton; */
  /* GtkWidget *gain_decrease_toolbutton; */
  /* GtkWidget *gain_increase_toolbutton; */

  /* GtkListStore  *recording_channel_store; //to fill up the treeview to select the recording channels */
  /* GtkWidget *oscilloscope_all_channels_view; // treeview to select the channels to record */
  /* GtkListStore  *oscilloscope_all_channels_store; // to fill the treeview to select the channels for oscilloscope */
  /* GtkWidget *oscilloscope_selected_channels_view; // selected channels for a page of the oscilloscope */
  /* GtkListStore *oscilloscope_selected_channels_store; // to fill the treeview for a group of the oscilloscope */
  /* int is_filling_selected_channel_liststore; */





  // callback functions
  void on_play_toolbutton_toggled();
  void on_record_toolbutton_toggled();
  void on_rewind_toolbutton_clicked();
  void on_forward_toolbutton_clicked();
  void on_gain_increase_toolbutton_clicked();
  void on_gain_decrease_toolbutton_clicked();
  void on_time_increase_toolbutton_clicked();
  void on_time_decrease_toolbutton_clicked();
  void on_about_menuitem_activate();


};

#endif // MAINWINDOW_H
