#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <gtkmm.h>
#include "rhd2000evalboard.h"

class mainWindow: public Gtk::Window
{
 private:
  Rhd2000EvalBoard *evalBoard;
  int errorCode;
  bool fastSettleEnabled;

  // amplifier settings 
  double desiredDspCutoffFreq;
  double actualDspCutoffFreq;
  double desiredUpperBandwidth;
  double actualUpperBandwidth;
  double desiredLowerBandwidth;
  double actualLowerBandwidth;
  bool dspEnabled;

  // impedance settings
  double desiredImpedanceFreq;
  double actualImpedanceFreq;
  bool impedanceFreqValid;

  // digital to analog converters
  int dacNumber;
  bool* dacEnabled;
  int* chipId;

  int evalBoardMode;

  // variable changed with sampling rate
  int sampleRate;
  double boardSampleRate;
  int numUsbBlocksToRead;

  // cable length for the different ports
  double cableLengthPortA;  // in meters
  double cableLengthPortB;  // in meters
  double cableLengthPortC;  // in meters
  double cableLengthPortD;  // in meters



  void openInterfaceBoard();
  void scanPorts();
  void findConnectedAmplifiers();
  void changeSampleRate(int sampleRateIndex);

 
 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();
  
 protected:
  Glib::RefPtr<Gtk::Builder> builder;
  
};

#endif // MAINWINDOW_H
