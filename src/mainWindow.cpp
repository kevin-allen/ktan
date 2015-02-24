#include "mainWindow.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include "recording.h"
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
  rec = new recording(db); // pass a dataBuffer as a pointer to the recording object

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

  cerr << "leaving mainWindow::mainWindow()\n";
}

mainWindow::~mainWindow()
{
  cerr << "entering mainWindow::~mainWindow()\n";
  delete acq;
  delete db;
  delete rec;
  cerr << "leaving mainWindow::~mainWindow()\n";

}


