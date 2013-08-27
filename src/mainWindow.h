#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//#include <gtkmm/window.h>
//#include <gtkmm/button.h>
#include <gtkmm.h>

class mainWindow: public Gtk::Window
{
 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();

 protected:
  Glib::RefPtr<Gtk::Builder> builder;
};

#endif // MAINWINDOW_H
