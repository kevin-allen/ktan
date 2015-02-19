#include "mainWindow.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>

mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{
  cerr << "entering mainWindow::mainWindow()\n";
  acq = new acquisition;
  cerr << "leaving mainWindow::mainWindow()\n";




  // start and stop the data acquisition 
  acq->start_acquisition();
  pthread_create(&acquisition_thread, NULL, &acquisition::acquisition_thread_helper, &acq);
  sleep(2);
  acq->stop_acquisition();
  


}

mainWindow::~mainWindow()
{
  cerr << "entering mainWindow::~mainWindow()\n";
  delete acq;
  cerr << "leaving mainWindow::~mainWindow()\n";

}


