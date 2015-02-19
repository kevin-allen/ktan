#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <gtkmm.h>
#include "rhd2000evalboard.h"
#include "acquisition.h"
#include "timeKeeper.h"


class mainWindow: public Gtk::Window
{
 private:
  acquisition* acq;
  pthread_t acquisition_thread;
  int acquisition_thread_id;

 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();
  
 protected:
  Glib::RefPtr<Gtk::Builder> builder;
  
};

#endif // MAINWINDOW_H
