#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <gtkmm.h>
#include "rhd2000evalboard.h"

class mainWindow: public Gtk::Window
{
 private:
  Rhd2000EvalBoard *evalBoard;
  int errorCode;
  void openInterfaceBoard();

 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();
  
 protected:
  Glib::RefPtr<Gtk::Builder> builder;
    
};

#endif // MAINWINDOW_H
