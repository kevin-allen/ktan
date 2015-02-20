#include "mainWindow.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include "acquisition.h"
#include "dataBuffer.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>

mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{

  cerr << "entering mainWindow::mainWindow()\n";
  db = new dataBuffer; // buffer that holds the latest data acquired by acquisition object
  acq = new acquisition(db); // pass a dataBuffer as a pointer to the acquisition object
  

  // start and stop the data acquisition 
  acq->start_acquisition();
  pthread_create(&acquisition_thread, NULL, &acquisition::acquisition_thread_helper, acq);
  sleep(2);
  acq->stop_acquisition();
  

  cerr << "leaving mainWindow::mainWindow()\n";

}

mainWindow::~mainWindow()
{
  cerr << "entering mainWindow::~mainWindow()\n";
  delete acq;
  delete db;
  cerr << "leaving mainWindow::~mainWindow()\n";

}


