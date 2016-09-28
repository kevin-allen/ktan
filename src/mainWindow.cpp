//#define DEBUG_WIN
#include "mainWindow.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include "recording.h"
#include "acquisition.h"
#include "oscilloscope.h"
#include "dataBuffer.h"
#include "channelGroup.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>
#include <stdio.h>
#include <glibmm.h>
#include <pwd.h>
#include <mcheck.h>
#include <fstream>

#define RECORDING_CHANNELS_ON 37
#define MAX_RECORDING_TIME_MIN 20

mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{

#ifdef DEBUG_WIN
  cerr << "entering mainWindow::mainWindow()\n";
#endif

  // get the widget from builder
  builder->get_widget("play_toolbutton",play_toolbutton);
  builder->get_widget("record_toolbutton",record_toolbutton);
  builder->get_widget("rewind_toolbutton",rewind_toolbutton);
  builder->get_widget("forward_toolbutton",forward_toolbutton);
  builder->get_widget("gain_increase_toolbutton",gain_increase_toolbutton);
  builder->get_widget("gain_decrease_toolbutton",gain_decrease_toolbutton);
  builder->get_widget("time_increase_toolbutton",time_increase_toolbutton);
  builder->get_widget("time_decrease_toolbutton",time_decrease_toolbutton);
  builder->get_widget("add_toolbutton",add_toolbutton);
  builder->get_widget("remove_toolbutton",remove_toolbutton);
  builder->get_widget("add_channel_button",add_channel_button);
  builder->get_widget("remove_channel_button",remove_channel_button);
  
  builder->get_widget("about_menuitem",about_menuitem);
  builder->get_widget("about_dialog",about_dialog);
  builder->get_widget("recording_dialog",recording_dialog);
  builder->get_widget("oscilloscope_dialog",oscilloscope_dialog);
  builder->get_widget("quit_menuitem",quit_menuitem);
  builder->get_widget("oscilloscope_menuitem",oscilloscope_menuitem);
  builder->get_widget("recording_menuitem",recording_menuitem);
  builder->get_widget("recording_treeview",recording_treeview);
  builder->get_widget("statusbar",statusbar);
  builder->get_widget("trial_spinbutton",trial_spinbutton);
  builder->get_widget("file_base_entry",file_base_entry);
  builder->get_widget("max_recording_time_spinbutton",max_recording_time_spinbutton);
  builder->get_widget("group_spinbutton",group_spinbutton);
  builder->get_widget("osc_group_preference_spinbutton",osc_group_preference_spinbutton);
  builder->get_widget("osc_group_treeview",osc_group_treeview);
  builder->get_widget("osc_all_channels_treeview",osc_all_channels_treeview);
  builder->get_widget("drawing_area",drawing_area);
  builder->get_widget("window",window);

  string icon_file_name;
  string dir = DATADIR;
  icon_file_name=dir+"/ktan_icon.png";
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(icon_file_name);
  window->set_icon(pixbuf);


  
  // connect signals to functions
  play_toolbutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow::on_play_toolbutton_toggled));
  record_toolbutton_connection = record_toolbutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow::on_record_toolbutton_toggled));
  rewind_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_rewind_toolbutton_clicked));
  forward_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_forward_toolbutton_clicked));
  gain_increase_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_gain_increase_toolbutton_clicked));
  gain_decrease_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_gain_decrease_toolbutton_clicked));
  time_increase_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_time_increase_toolbutton_clicked));
  time_decrease_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_time_decrease_toolbutton_clicked));
  add_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_add_toolbutton_clicked));
  remove_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_remove_toolbutton_clicked));
  add_channel_button->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_add_channel_button_clicked));
  remove_channel_button->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_remove_channel_button_clicked));

  about_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_about_menuitem_activate));
  quit_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_quit_menuitem_activate));
  oscilloscope_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_oscilloscope_menuitem_activate));
  recording_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_recording_menuitem_activate));
  group_spinbutton->signal_value_changed().connect(sigc::mem_fun(*this, &mainWindow::on_group_spinbutton_value_changed));
  osc_group_preference_spinbutton->signal_value_changed().connect(sigc::mem_fun(*this, &mainWindow::on_osc_group_preference_spinbutton_value_changed));
  window->signal_delete_event().connect(sigc::mem_fun(*this, &mainWindow::on_window_delete_event));

  // get the home directory
  struct passwd *p;
  char *username=getenv("USER");
  p=getpwnam(username);
  strcpy(home_directory,p->pw_dir);
  strcat(home_directory,"/");
  //cerr << "home directory: " << home_directory << '\n';
  
  db=NULL;
  acq=NULL;
  rec=NULL;
  osc=NULL;
  sm=NULL;


#ifdef DEBUG_WIN
  cerr << "calling dataBuffer constructor\n";
#endif
  db = new dataBuffer; // buffer that holds the latest data acquired by acquisition object
#ifdef DEBUG_WIN
  cerr << "calling acquisition constructor\n";
#endif
  acq = new acquisition(db); // pass a dataBuffer as a pointer to the acquisition object
  
  board_is_there=false;
  if(acq->get_set_successfully()==false)
    {
      cerr << "Problem setting the acquisition board\n";
      delete db;
      delete acq;
      db=NULL;
      acq=NULL;
      return;
    }
  board_is_there=true;

#ifdef DEBUG_WIN
  cerr << "calling recording constructor\n";
#endif
  rec = new recording(db); // pass a dataBuffer as a pointer to the recording object

#ifdef DEBUG_WIN
  cerr << "calling oscilloscope constructor\n";
#endif
  osc = new oscilloscope(db,drawing_area);
  num_channels=db->getNumChannels();

  file_base_entry->set_text(rec->get_file_base());
  trial_spinbutton->set_value(rec->get_file_index());
  
  // for timer
  tslot = sigc::mem_fun(*this, &mainWindow::on_statusbar_timeout);
  tslot_sm=sigc::mem_fun(*this, &mainWindow::on_sm_timeout);
  // create the shared memory, for triggering start and stop of recording
  sm= new shared_memory;


  // set group spinbutton, what is show is index of group
  Glib::RefPtr<Gtk::Adjustment>  osc_group_adjustment= group_spinbutton->get_adjustment();
  osc_group_adjustment->set_lower(0);
  osc_group_adjustment->set_upper(osc->get_num_groups()-1); 
  group_spinbutton->set_value(0);
  Glib::RefPtr<Gtk::Adjustment>  osc_group_preference_adjustment= group_spinbutton->get_adjustment();
  osc_group_preference_adjustment=osc_group_preference_spinbutton->get_adjustment();
  osc_group_preference_adjustment->set_lower(0);
  osc_group_preference_adjustment->set_upper(osc->get_num_groups()-1); 
  osc_group_preference_spinbutton->set_value(0);

  // set max recording time
  max_recording_time_spinbutton->set_value(MAX_RECORDING_TIME_MIN);

  
  build_model_recording_treeview();
  build_model_oscilloscope_all_treeview();
  build_model_oscilloscope_group_treeview();

  
  sm_timeout_connection = Glib::signal_timeout().connect(tslot_sm,500); 
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::mainWindow()\n";
#endif
}

mainWindow::~mainWindow()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::~mainWindow()\n";
#endif

  sm_timeout_connection.disconnect();
  
  if(acq!=NULL)
    delete acq;
  if(rec!=NULL)
    delete rec;
  if(osc!=NULL)
    delete osc;
  if(db!=NULL)
    delete db;
  if(sm!=NULL)
    delete sm;
  
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::~mainWindow()\n";
#endif

}

bool mainWindow::get_board_is_there()
{
  return board_is_there;
}

void mainWindow::start_oscilloscope(){
  if(acq->get_is_acquiring()==false) // no acquisition running, so start it
    {
#ifdef DEBUG_WIN
      cerr << "starting acquisition\n";
#endif
      db->resetData();
      acq->start_acquisition();
      pthread_create(&acquisition_thread, NULL, &acquisition::acquisition_thread_helper, acq);
    }
  // start the osc
  osc->start_oscilloscope();
}
void mainWindow::stop_oscilloscope(){
  // stop the osc
  osc->stop_oscilloscope();
  
  // if not recording, stop acquisition
  if(rec->get_is_recording()==false)
    { 
#ifdef DEBUG_WIN
      cerr << "stop acquisition\n";
#endif
      acq->stop_acquisition();
    }
  
}
void mainWindow::start_recording(){
  // recording not under way, start it
  bool osc_flag=osc->get_is_displaying();
  // if oscilloscope running, stop it for a brief moment
  if(osc_flag==true)
    osc->stop_oscilloscope();
  
  
  if(acq->get_is_acquiring()==true)
    acq->stop_acquisition();
  
  // recording not running, start it
  db->resetData();
#ifdef DEBUG_WIN
  cerr << "recording not running, start it\n";
#endif
  
  rec->set_max_recording_time(max_recording_time_spinbutton->get_value());
  rec->set_file_base(file_base_entry->get_text());
  rec->set_file_index(trial_spinbutton->get_value());
  
  if(check_file_overwrite()==false)
    {
      cerr << "check file overwrite returned false, abort recording\n";
      record_toolbutton_connection.disconnect();
      record_toolbutton->set_active(false);
      record_toolbutton_connection = record_toolbutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow::on_record_toolbutton_toggled));
      return;
    }
  
  acq->start_acquisition();
  pthread_create(&acquisition_thread, NULL, &acquisition::acquisition_thread_helper, acq);
  
#ifdef DEBUG_WIN
  cerr << "start recording\n";
#endif
  
  if(rec->start_recording()==false)
    {
      cerr << "rec->start_recording returned false, recording aborted\n";
      record_toolbutton_connection.disconnect();
      record_toolbutton->set_active(false);
      record_toolbutton_connection = record_toolbutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow::on_record_toolbutton_toggled));
      return;
    }
  pthread_create(&recording_thread, NULL, &recording::recording_thread_helper, rec);
  statusbar_timeout_connection = Glib::signal_timeout().connect(tslot,1000); 
  
  if(osc_flag==true)
    osc->start_oscilloscope();
}
void mainWindow::stop_recording(){
  // recording is running, stop it
#ifdef DEBUG_WIN
  cerr << "recording is running, stop it\n";
#endif
  rec->stop_recording();
  file_base_entry->set_text(rec->get_file_base());
  trial_spinbutton->set_value(rec->get_file_index());
  unsigned int m_context_id;
  m_context_id = statusbar->get_context_id("Statusbar example");
  statusbar->pop(m_context_id);
  statusbar_timeout_connection.disconnect();
  if(osc->get_is_displaying()==false)
    {
      acq->stop_acquisition();
    }
}

void mainWindow::on_play_toolbutton_toggled()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_play_toolbutton_toggled()\n";
#endif
  if(osc->get_is_displaying()==true)
    {
      stop_oscilloscope();
    }
  else // not displaying
    {
      start_oscilloscope();
    }

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_play_toolbutton_toggled()\n";
#endif
}

void mainWindow::on_record_toolbutton_toggled()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_record_toolbutton_toggled()\n";
#endif
  if(rec->get_is_recording()==true)
    {
      stop_recording();
    }
  else 
    {
      start_recording();
    }
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_record_toolbutton_toggled()\n";
#endif
}
bool mainWindow::on_sm_timeout()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_sm_timeout()\n";
#endif
    
  if(sm->get_start_osc()==1){
    sm->set_start_osc(0);
    if(osc->get_is_displaying()==false)
      start_oscilloscope();
  }
  if(sm->get_stop_osc()==1){
    sm->set_stop_osc(0);
    if(osc->get_is_displaying()==true)
      stop_oscilloscope();
  }
  if(sm->get_start_rec()==1){
    sm->set_start_rec(0);
    if(rec->get_is_recording()==false)
      start_recording();
  }
  if(sm->get_stop_rec()==1){
    sm->set_stop_rec(0);
    if(rec->get_is_recording()==true)
      stop_recording();
  }
  return true;
}
bool mainWindow::on_statusbar_timeout()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_statusbar_timeout()\n";
#endif
  unsigned int m_context_id;
  m_context_id = statusbar->get_context_id("Statusbar example");
  std::stringstream ss;
  ss << "recording " << rec->get_number_channels_save() << " channels to " << rec->get_file_name() << " time: " << rec->get_recording_duration_sec() << " sec";
  statusbar->pop(m_context_id);
  statusbar->push(ss.str(),m_context_id);
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_statusbar_timeout()\n";
#endif
  return true;
}


bool mainWindow::check_file_overwrite() // abort when returned false
{
  string fd =rec->get_file_name();
  struct stat st;
  if(stat(fd.c_str(),&st)== 0)
    {

      Gtk::MessageDialog dialog(*this, fd,
				false /* use_markup */, Gtk::MESSAGE_QUESTION,
				Gtk::BUTTONS_YES_NO);


      dialog.set_secondary_text("This file already exists. Do you want to overwrite it?");
      
      int result = dialog.run();

      switch(result)
	{
	case(Gtk::RESPONSE_YES):
	  {
	    return true;
	  }
	case(Gtk::RESPONSE_NO):
	  {
	    return false;
	    break;
	  }
	default:
	  {
	    return false;
	    break;
	  }
	}
  
    }
  return true;
}


void mainWindow::on_rewind_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_rewind_toolbutton_clicked()\n";
#endif
  osc->show_previous_page();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_rewind_toolbutton_clicked()\n";
#endif
  
}
void mainWindow::on_forward_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_forward_toolbutton_clicked()\n";
#endif
  osc->show_next_page();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_forward_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_gain_increase_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_gain_increase_toolbutton_clicked()\n";
#endif
  osc->increase_gain();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_gain_increase_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_gain_decrease_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_gain_decrease_toolbutton_clicked()\n";
#endif
  osc->decrease_gain();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_gain_decrease_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_time_increase_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_time_increase_toolbutton_clicked()\n";
#endif
  osc->increase_time_shown();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_time_increase_toolbutton_clicked()\n";
#endif  
}
void mainWindow::on_time_decrease_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_time_decrease_toolbutton_clicked()\n";
#endif
  osc->decrease_time_shown();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_time_decrease_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_about_menuitem_activate()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_about_menuitem_activate()\n";
#endif
  int response;
  response=about_dialog->run();
  switch(response)
    {
    case (GTK_RESPONSE_CANCEL):
      {
	about_dialog->hide();
	break;
      }
    default:
      {
	about_dialog->hide();
	break;
      }
    }
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_about_menu_item_activate()\n";
#endif
}


void mainWindow::on_oscilloscope_menuitem_activate()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::oscilloscope_menuitem_activate()\n";
#endif
  int response;
  response=oscilloscope_dialog->run();
  switch(response)
    {
    case (GTK_RESPONSE_CANCEL):
      {
	oscilloscope_dialog->hide();
	break;
      }
    default:
      {
	oscilloscope_dialog->hide();
	break;
      }
    }
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::oscilloscope_menuitem_activate()\n";
#endif
}

void mainWindow::on_recording_menuitem_activate()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::recording_menuitem_activate()\n";
#endif
  int response;
  response=recording_dialog->run();
  switch(response)
    {
    case (GTK_RESPONSE_CANCEL):
      {
	recording_dialog->hide();
	break;
      }
    default:
      {
	recording_dialog->hide();
	break;
      }
    }

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::recording_menuitem_activate()\n";
#endif
}


void mainWindow::build_model_oscilloscope_group_treeview()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::build_model_oscilloscope_group_treeview()\n";
#endif
  osc_group_treeview->append_column("ID", m_OscilloscopeColumns.m_col_id);
  osc_group_treeview->append_column("Name", m_OscilloscopeColumns.m_col_name);

 // allow multiple selection
  Glib::RefPtr<Gtk::TreeSelection> ts = osc_all_channels_treeview->get_selection();
  ts->set_mode(Gtk::SELECTION_MULTIPLE);


  // set the model for the treeview
  m_refOscGrpTreeModel = Gtk::ListStore::create(m_OscilloscopeColumns);
  osc_group_treeview->set_model(m_refOscGrpTreeModel);

  // get value in spinbutton
  int index_group = osc_group_preference_spinbutton->get_value();
  channelGroup* cg= osc->get_one_channel_group(index_group);

  // add the channels that are in the channel group of the oscilloscope
  
  Gtk::TreeModel::Row row;
  for(int i = 0; i < cg->get_num_channels(); i++)
    { 
      row = *(m_refOscGrpTreeModel->append());
      std::stringstream ss;
      ss << cg->get_channel_id(i);
      row[m_RecordingColumns.m_col_id] = cg->get_channel_id(i);
      row[m_RecordingColumns.m_col_name] = ss.str();
    }


#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::build_model_oscilloscope_group_treeview()\n";
#endif

}
void mainWindow::build_model_oscilloscope_all_treeview()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::build_model_oscilloscope_all_treeview()\n";
#endif
  //Add the TreeView's view columns:
  osc_all_channels_treeview->append_column("ID", m_OscilloscopeColumns.m_col_id);
  osc_all_channels_treeview->append_column("Name", m_OscilloscopeColumns.m_col_name);

  // allow selection of rows
  //Gtk::TreeView::Column* pColumn = osc_all_channels_treeview->get_column(2);
  //pColumn->set_clickable(true);

   // allow multiple selection
  Glib::RefPtr<Gtk::TreeSelection> ts = osc_all_channels_treeview->get_selection();
  ts->set_mode(Gtk::SELECTION_MULTIPLE);
  
 // set the model for the treeview
  m_refOscAllTreeModel = Gtk::ListStore::create(m_OscilloscopeColumns);
  osc_all_channels_treeview->set_model(m_refOscAllTreeModel);

  // fill up the model
  Gtk::TreeModel::Row row;
  for(unsigned int i = 0; i < num_channels; i++)
    { 
      row = *(m_refOscAllTreeModel->append());
      std::stringstream ss;
      ss << i;
      row[m_RecordingColumns.m_col_id] = i;
      row[m_RecordingColumns.m_col_name] = ss.str();
    }
  

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::build_model_oscilloscope_all_treeview()\n";
#endif

}


void mainWindow::build_model_recording_treeview()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::build_model_recording_treeview()\n";
#endif

  //Add the TreeView's view columns:
  recording_treeview->append_column("ID", m_RecordingColumns.m_col_id);
  recording_treeview->append_column("Name", m_RecordingColumns.m_col_name);
  recording_treeview->append_column("Select", m_RecordingColumns.m_col_selected);

  // allow selection of rows
  Gtk::TreeView::Column* pColumn = recording_treeview->get_column(2);
  pColumn->set_clickable(true);

  // http://www.pygtk.org/pygtk2tutorial/sec-TreeSelections.html

  // allow multiple selection
  Glib::RefPtr<Gtk::TreeSelection> ts = recording_treeview->get_selection();
  ts->set_mode(Gtk::SELECTION_MULTIPLE);
  
  // set the model for the treeview
  m_refRecTreeModel = Gtk::ListStore::create(m_RecordingColumns);
  recording_treeview->set_model(m_refRecTreeModel);


   // fill up the model
  Gtk::TreeModel::Row row;
  char conf_file_name[255];
  strcpy(conf_file_name,home_directory);
  strcat(conf_file_name,"ktan.recording.channels");
    
  int rec_channels[256];
  int num_rec_channels=0;
  int in_list = 0;

  
  //cout << "check if " << conf_file_name << " is present in " << home_directory << "\n";
  ifstream file(conf_file_name);
  if(file.is_open()==TRUE)
    {
      //cout << "reading recording channels from " << conf_file_name << '\n';
      while (file >> rec_channels[num_rec_channels]&&num_rec_channels<256)
	{
	  num_rec_channels++;
	}

      for(unsigned int i = 0; i < num_channels; i++)
	{ 
	  row = *(m_refRecTreeModel->append());
	  std::stringstream ss;
	  ss << i;
	  row[m_RecordingColumns.m_col_id] = i;
	  row[m_RecordingColumns.m_col_name] = ss.str();
	  in_list=0;
	  for(int j = 0; j < num_rec_channels;j++){
	    if(rec_channels[j]==i)
	      in_list=1;
	  }
	  if(in_list==1) // to work with 48 drives on 64 channels amplifier
	    row[m_RecordingColumns.m_col_selected] = true;
	  else
	    row[m_RecordingColumns.m_col_selected] = false;
	}
    }
  else
    {
      for(unsigned int i = 0; i < num_channels; i++)
	{ 
	  row = *(m_refRecTreeModel->append());
	  std::stringstream ss;
	  ss << i;
	  row[m_RecordingColumns.m_col_id] = i;
	  row[m_RecordingColumns.m_col_name] = ss.str();
	  if(i < RECORDING_CHANNELS_ON && (i<40||i>55)) // to work with 48 drives on 64 channels amplifier
	    row[m_RecordingColumns.m_col_selected] = true;
	  else
	    row[m_RecordingColumns.m_col_selected] = false;
	}
    }
  file.close();
  update_recording_channels();  
#ifdef DEBUG_WIN
  cerr << "leave mainWindow::build_model_recording_treeview()\n";
#endif
}

void mainWindow::on_add_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_add_toolbutton_clicked()\n";
#endif
    change_recording_treeview_selection(true);
    update_recording_channels();
#ifdef DEBUG_WIN
  cerr << "leave mainWindow::on_add_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_remove_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_remove_toolbutton_clicked()\n";
#endif
  change_recording_treeview_selection(false);
  update_recording_channels();
#ifdef DEBUG_WIN
  cerr << "leave mainWindow::on_remove_toolbutton_clicked()\n";
#endif
}
void mainWindow::update_recording_channels()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::update_recording_channels()\n";
#endif
  int numRecChannels=0;
  unsigned int* Chan;
  Chan = new unsigned int [num_channels];
  typedef Gtk::TreeModel::Children type_children; //minimise code length.
  type_children children = m_refRecTreeModel->children();
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter)
    {
       Gtk::TreeModel::Row row = *iter;
       if(row[m_RecordingColumns.m_col_selected] == true)
	 {
	   Chan[numRecChannels]=row[m_RecordingColumns.m_col_id];
	   numRecChannels++;
         }
    } 
#ifdef DEBUG_WIN
  cerr << numRecChannels << " channels to record\n";
#endif
  if(rec->set_recording_channels(numRecChannels, Chan)==false)
    {
      cerr << "mainWindow::update_recording_channels(), problem settig recording channels\n";
    }
  delete[] Chan;
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::update_recording_channels()\n";
#endif
  
}

void mainWindow::change_recording_treeview_selection(bool sel)
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::change_recording_treeview_selection()\n";
#endif
  // get selected row in treeview model
  Glib::RefPtr<Gtk::TreeSelection> ts = recording_treeview->get_selection();
  std::vector<Gtk::TreeModel::Path> pathlist;
  pathlist = ts->get_selected_rows(); 

  // get change selected rows
  for(int i = 0; i < pathlist.size(); i++)
    {
      Gtk::TreeModel::iterator iter =  m_refRecTreeModel->get_iter(pathlist[i]);
      Gtk::TreeModel::Row row = *iter;
      // now do what you need to do with the data in your TreeModel
      row[m_RecordingColumns.m_col_selected] = sel;

    } 
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::change_recording_treeview_selection()\n";
#endif

}

void mainWindow::on_group_spinbutton_value_changed()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_group_spinbutton_value_changed()\n";
#endif
  osc->set_current_group(group_spinbutton->get_value());
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_group_spinbutton_value_changed()\n";
#endif
  
}
void mainWindow::on_osc_group_preference_spinbutton_value_changed()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_osc_group_preference_spinbutton_value_changed()\n";
#endif
  // empty the group tree view
  m_refOscGrpTreeModel->clear();

  // get value in spinbutton
  int index_group = osc_group_preference_spinbutton->get_value();
  channelGroup* cg= osc->get_one_channel_group(index_group);

  // add the channels that are in the channel group of the oscilloscope
  Gtk::TreeModel::Row row;
  for(int i = 0; i < cg->get_num_channels(); i++)
    { 
      row = *(m_refOscGrpTreeModel->append());
      std::stringstream ss;
      ss << cg->get_channel_id(i);
      row[m_RecordingColumns.m_col_id] = cg->get_channel_id(i);
      row[m_RecordingColumns.m_col_name] = ss.str();
    }

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_osc_group_preference_spinbutton_value_changed()\n";
#endif

}
 void mainWindow::on_add_channel_button_clicked()
 {
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_add_channel_button_clicked()\n";
#endif
  
  // check how many channels we can add
  int maxAdd=0;
  int index_group = osc_group_preference_spinbutton->get_value();
  channelGroup* cg= osc->get_one_channel_group(index_group);
  maxAdd=cg->get_max_num_channels()-cg->get_num_channels();

  Glib::RefPtr<Gtk::TreeSelection> ts = osc_all_channels_treeview->get_selection();
  std::vector<Gtk::TreeModel::Path> pathlist;
  pathlist = ts->get_selected_rows(); 

  if(pathlist.size()>maxAdd)
    {
      cerr << "mainWindow::on_add_channel_button_clicked, too many channels to add\n";
      return;
    }
    
  Glib::ustring name;
  int id;
  // add the selected rows to the group tree model
  for(int i = 0; i < pathlist.size(); i++)
    {
      Gtk::TreeModel::iterator iter =  m_refOscAllTreeModel->get_iter(pathlist[i]);
      Gtk::TreeModel::Row row = *iter;
      Gtk::TreeModel::Row row1 =  *(m_refOscGrpTreeModel->append());
      // now do what you need to do with the data in your TreeModel
      id = row[m_OscilloscopeColumns.m_col_id];
      name = row[m_OscilloscopeColumns.m_col_name];
      row1[m_OscilloscopeColumns.m_col_id] = id;
      row1[m_OscilloscopeColumns.m_col_name] = name;
    }

  copy_osc_group_tree_model_into_channel_group();

  
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_add_channel_button_clicked()\n";
#endif

}
   
 void mainWindow::on_remove_channel_button_clicked()
 {
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_remove_channel_button_clicked()\n";
#endif

    // get selected row in treeview model
  Glib::RefPtr<Gtk::TreeSelection> ts = osc_group_treeview->get_selection();
  std::vector<Gtk::TreeModel::Path> pathlist;
  pathlist = ts->get_selected_rows(); 
  if (pathlist.size()>0)
    {
      Gtk::TreeModel::iterator iter =  m_refOscGrpTreeModel->get_iter(pathlist[0]);
      m_refOscGrpTreeModel->erase(iter);
    }
  
  // update the channel group
  copy_osc_group_tree_model_into_channel_group();
  
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_remove_channel_button_clicked()\n";
#endif
 }

void mainWindow::copy_osc_group_tree_model_into_channel_group()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::copy_osc_group_tree_model_into_channel_group()\n";
#endif

  int index_group = osc_group_preference_spinbutton->get_value();
  channelGroup* cg= osc->get_one_channel_group(index_group);
  // copy the tree model into the group
  typedef Gtk::TreeModel::Children type_children; //minimise code length.
  type_children children = m_refOscGrpTreeModel->children();
  int num_channels=0;
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter)
    num_channels++;

  if(num_channels>cg->get_max_num_channels())
    {
      cerr << "mainWindow::copy_osc_group_tree_model_into_channel_group, num_channels is larger than cg can take\n";
      return;
    }

  cg->set_num_channels(num_channels);
  int index=0;
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter)
    {
      Gtk::TreeModel::Row row = *iter;
      cg->set_channel_id(index,row[m_OscilloscopeColumns.m_col_id]);
      index++;
    }

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::copy_osc_group_tree_model_into_channel_group()\n";
#endif

}


void mainWindow::on_quit_menuitem_activate()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::quit_menuitem_activate()\n";
#endif
  if(rec->get_is_recording()==true)
     rec->stop_recording();
  if(osc->get_is_displaying()==true)
    osc->stop_oscilloscope();
  if(acq->get_is_acquiring()==true)
    acq->stop_acquisition();
  hide();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::quit_menuitem_activate()\n";
#endif
}


bool mainWindow::on_window_delete_event(GdkEventAny* event)
{
#ifdef DEBUG_WIN
  cerr << " mainWindow::on_window_delete_event()\n";
#endif
  if(rec->get_is_recording()==true)
     rec->stop_recording();
  if(osc->get_is_displaying()==true)
    osc->stop_oscilloscope();
  if(acq->get_is_acquiring()==true)
    acq->stop_acquisition();
  hide();
  return true;
}

