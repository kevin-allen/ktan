#define DEBUG_WIN
#include "mainWindow.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include "recording.h"
#include "acquisition.h"
#include "dataBuffer.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>
#include <stdio.h>

mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{

#ifdef DEBUG_WIN
  cerr << "entering mainWindow::mainWindow()\n";
#endif

  cerr << "entering mainWindow::mainWindow()\n";
  db = new dataBuffer; // buffer that holds the latest data acquired by acquisition object
  //  acq = new acquisition(db); // pass a dataBuffer as a pointer to the acquisition object
  //rec = new recording(db); // pass a dataBuffer as a pointer to the recording object

  // get the widget from builder
  builder->get_widget("play_toolbutton",play_toolbutton);
  builder->get_widget("record_toolbutton",record_toolbutton);
  builder->get_widget("rewind_toolbutton",rewind_toolbutton);
  builder->get_widget("forward_toolbutton",forward_toolbutton);
  builder->get_widget("gain_increase_toolbutton",gain_increase_toolbutton);
  builder->get_widget("gain_decrease_toolbutton",gain_decrease_toolbutton);
  builder->get_widget("time_increase_toolbutton",time_increase_toolbutton);
  builder->get_widget("time_decrease_toolbutton",time_decrease_toolbutton);
  builder->get_widget("about_menuitem",about_menuitem);
  builder->get_widget("about_dialog",about_dialog);
  builder->get_widget("recording_dialog",recording_dialog);
  builder->get_widget("quit_menuitem",quit_menuitem);
  builder->get_widget("oscilloscope_menuitem",oscilloscope_menuitem);
  builder->get_widget("recording_menuitem",recording_menuitem);
  builder->get_widget("recording_treeview",recording_treeview);
  
  // connect signals to functions
  play_toolbutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow::on_play_toolbutton_toggled));
  record_toolbutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow::on_record_toolbutton_toggled));
  rewind_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_rewind_toolbutton_clicked));
  forward_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_forward_toolbutton_clicked));
  gain_increase_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_gain_increase_toolbutton_clicked));
  gain_decrease_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_gain_decrease_toolbutton_clicked));
  time_increase_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_time_increase_toolbutton_clicked));
  time_decrease_toolbutton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow::on_time_decrease_toolbutton_clicked));
  about_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_about_menuitem_activate));
  quit_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_quit_menuitem_activate));
  oscilloscope_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_oscilloscope_menuitem_activate));
  recording_menuitem->signal_activate().connect(sigc::mem_fun(*this, &mainWindow::on_recording_menuitem_activate));


  // // start data acquisition on the board
  // acq->start_acquisition();
  // //start a thread that will get the data comming from usb and put them into db
  // pthread_create(&acquisition_thread, NULL, &acquisition::acquisition_thread_helper, acq);

  // rec->start_recording();
  // pthread_create(&recording_thread, NULL, &recording::recording_thread_helper, rec);
  // sleep(5);
  // // stop acquisition, the acquisition thread will die
  // acq->stop_acquisition();
  // rec->stop_recording();

  build_model_recording_treeview();
  
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::mainWindow()\n";
#endif


}

mainWindow::~mainWindow()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::~mainWindow()\n";
#endif

  // delete acq;
  // delete rec;
  delete db;

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::~mainWindow()\n";
#endif

}


void mainWindow::on_play_toolbutton_toggled()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_play_toolbutton_toggled()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_play_toolbutton_toggled()\n";
#endif
}
void mainWindow::on_record_toolbutton_toggled()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_record_toolbutton_toggled()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_record_toolbutton_toggled()\n";
#endif
}
void mainWindow::on_rewind_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_rewind_toolbutton_clicked()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_rewind_toolbutton_clicked()\n";
#endif
  
}
void mainWindow::on_forward_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_forward_toolbutton_clicked()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_forward_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_gain_increase_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_gain_increase_toolbutton_clicked()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_gain_increase_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_gain_decrease_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_gain_decrease_toolbutton_clicked()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_gain_decrease_toolbutton_clicked()\n";
#endif
}
void mainWindow::on_time_increase_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_time_increase_toolbutton_clicked()\n";
#endif

#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::on_time_increase_toolbutton_clicked()\n";
#endif  
}
void mainWindow::on_time_decrease_toolbutton_clicked()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::on_time_decrease_toolbutton_clicked()\n";
#endif

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


void mainWindow::on_quit_menuitem_activate()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::quit_menuitem_activate()\n";
#endif
  hide();
#ifdef DEBUG_WIN
  cerr << "leaving mainWindow::quit_menuitem_activate()\n";
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
void mainWindow::build_model_recording_treeview()
{
#ifdef DEBUG_WIN
  cerr << "entering mainWindow::build_model_recording_treeview()\n";
#endif
  //Add the TreeView's view columns:
  //This number will be shown with the default numeric formatting.
  recording_treeview->append_column("ID", m_RecordingColumns.m_col_id);
  recording_treeview->append_column("Name", m_RecordingColumns.m_col_name);
  recording_treeview->append_column("Select", m_RecordingColumns.m_col_selected);
  
  Gtk::TreeView::Column* pColumn = recording_treeview->get_column(2);
  pColumn->set_clickable(true);


  // http://www.pygtk.org/pygtk2tutorial/sec-TreeSelections.html
  Glib::RefPtr<Gtk::TreeSelection> ts = recording_treeview->get_selection();
  ts->set_mode(Gtk::SELECTION_MULTIPLE);
  
  m_refRecTreeModel = Gtk::ListStore::create(m_RecordingColumns);
  recording_treeview->set_model(m_refRecTreeModel);

  Gtk::TreeModel::Row row;
  for(int i = 0; i < 32; i++)
    { 
      row = *(m_refRecTreeModel->append());
      std::stringstream ss;
      ss << i;
      row[m_RecordingColumns.m_col_id] = i;
      row[m_RecordingColumns.m_col_name] = ss.str();
      row[m_RecordingColumns.m_col_selected] = true;
    }
  
#ifdef DEBUG_WIN
  cerr << "leave mainWindow::build_model_recording_treeview()\n";
#endif
}
