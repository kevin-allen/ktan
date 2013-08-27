#include "mainWindow.h"
#include <iostream>


mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{

  
}

mainWindow::~mainWindow()
{
}
