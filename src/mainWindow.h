#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <gtkmm.h>
#include "rhd2000evalboard.h"
#include "acquisition.h"
#include "recording.h"
#include "oscilloscope.h"
#include "timeKeeper.h"
#include "dataBuffer.h"
#include "shared_memory.h"




class mainWindow: public Gtk::Window
{
 private:
  acquisition* acq;
  dataBuffer* db; // data buffer not deutsche bahn
  recording* rec;
  oscilloscope* osc;
  shared_memory* sm;
  pthread_t acquisition_thread;
  int acquisition_thread_id;
  pthread_t recording_thread;
  int recording_thread_id;
  int num_channels;
  bool board_is_there;
  char home_directory[255];
  
 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();
  bool get_board_is_there();
 protected:
  Glib::RefPtr<Gtk::Builder> builder;

  // widgets
  Gtk::ToggleToolButton* play_toolbutton;
  Gtk::ToggleToolButton* record_toolbutton;
  sigc::connection record_toolbutton_connection;
  Gtk::ToolButton*  rewind_toolbutton;
  Gtk::ToolButton*  forward_toolbutton;
  Gtk::ToolButton*  gain_increase_toolbutton;
  Gtk::ToolButton*  gain_decrease_toolbutton;
  Gtk::ToolButton*  time_increase_toolbutton;
  Gtk::ToolButton*  time_decrease_toolbutton;
  Gtk::ToolButton*  add_toolbutton;
  Gtk::ToolButton*  remove_toolbutton;
  
  Gtk::Button*  add_channel_button;
  Gtk::Button*  remove_channel_button;

  Gtk::Window* window;

  Gtk::MenuItem* about_menuitem;
  Gtk::MenuItem* quit_menuitem;
  Gtk::MenuItem* oscilloscope_menuitem;
  Gtk::MenuItem* recording_menuitem;

  Gtk::AboutDialog* about_dialog;
  Gtk::Dialog* recording_dialog;
  Gtk::Dialog* oscilloscope_dialog;

  Gtk::TreeView* osc_group_treeview;
  Gtk::TreeView* osc_all_channels_treeview;

  Gtk::TreeView* recording_treeview;
  Gtk::SpinButton* trial_spinbutton;
  Gtk::Entry* file_base_entry;
  Gtk::Statusbar* statusbar;

  Gtk::SpinButton* max_recording_time_spinbutton;

  Gtk::SpinButton* group_spinbutton;
  Gtk::SpinButton* osc_group_preference_spinbutton;
  sigc::slot<bool> tslot;
  sigc::slot<bool> tslot_sm;
  sigc::connection statusbar_timeout_connection; // for timeout
  sigc::connection sm_timeout_connection; // for timeout
   
  Gtk::DrawingArea* drawing_area;


  string date_string;


  //Tree model columns:
  class ModelRecordingColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:
    ModelRecordingColumns()
    { add(m_col_id); add(m_col_name); add(m_col_selected);}
    Gtk::TreeModelColumn<unsigned int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    Gtk::TreeModelColumn<bool> m_col_selected;
  };
  ModelRecordingColumns m_RecordingColumns;

  Glib::RefPtr<Gtk::ListStore> m_refRecTreeModel;
  
  class ModelOscilloscopeColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:
    ModelOscilloscopeColumns()
      { add(m_col_id); add(m_col_name);}
    Gtk::TreeModelColumn<unsigned int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_name;
  };
  ModelRecordingColumns m_OscilloscopeColumns;
  Glib::RefPtr<Gtk::ListStore> m_refOscGrpTreeModel;
  Glib::RefPtr<Gtk::ListStore> m_refOscAllTreeModel;
  

  void set_max_recording_time();
  // callback functions
  void on_play_toolbutton_toggled();
  void on_record_toolbutton_toggled();
  void on_rewind_toolbutton_clicked();
  void on_forward_toolbutton_clicked();
  void on_gain_increase_toolbutton_clicked();
  void on_gain_decrease_toolbutton_clicked();
  void on_time_increase_toolbutton_clicked();
  void on_time_decrease_toolbutton_clicked();
  void on_add_toolbutton_clicked();
  void on_remove_toolbutton_clicked();
  void on_about_menuitem_activate();
  void on_quit_menuitem_activate();
  void on_oscilloscope_menuitem_activate();
  void on_recording_menuitem_activate();
  void change_recording_treeview_selection(bool sel);
  void update_recording_channels();
  void build_model_recording_treeview();
  void build_model_oscilloscope_all_treeview();
  void build_model_oscilloscope_group_treeview();
  bool check_file_overwrite();
  bool on_statusbar_timeout();
  bool on_sm_timeout();
  void on_group_spinbutton_value_changed();
  void on_osc_group_preference_spinbutton_value_changed();
  void on_add_channel_button_clicked();
  void on_remove_channel_button_clicked();
  void copy_osc_group_tree_model_into_channel_group();
  bool on_window_delete_event(GdkEventAny* event);


  void start_oscilloscope();
  void stop_oscilloscope();
  void start_recording();
  void stop_recording();
};

#endif // MAINWINDOW_H
