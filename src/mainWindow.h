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
  
};

#endif // MAINWINDOW_H
