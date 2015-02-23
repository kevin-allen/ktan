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
  

  // start data acquisition on the board
  acq->start_acquisition();

  //start a thread that will get the data comming from usb and put them into db
  pthread_create(&acquisition_thread, NULL, &acquisition::acquisition_thread_helper, acq);
  sleep(1);

  // stop acquisition, the acquisition thread will die
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


