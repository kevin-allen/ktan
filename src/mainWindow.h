#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <gtkmm.h>
#include "rhd2000evalboard.h"

#define CHIP_ID_RHD2132  1
#define CHIP_ID_RHD2216  2
#define CHIP_ID_RHD2164  4
// Constant used in software to denote RHD2164 MISO B data source
#define CHIP_ID_RHD2164_B  1000
// RHD2164 MISO ID numbers from ROM register 59
#define REGISTER_59_MISO_A  53
#define REGISTER_59_MISO_B  58


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
  int numDacs;
  bool* dacEnabled;
  int* chipId;

  int evalBoardMode;
  
  // for leds
  int ttlOut[16];

  // variable changed with sampling rate
  int sampleRate;
  double boardSampleRate;
  int numUsbBlocksToRead;

  // cable length for the different ports
  double cableLengthPortA;  // in meters
  double cableLengthPortB;  // in meters
  double cableLengthPortC;  // in meters
  double cableLengthPortD;  // in meters

  int numPorts;
  bool* portEnabled;

  bool running;
  
  void openInterfaceBoard();
  void scanPorts();
  void findConnectedAmplifiers();
  void changeSampleRate(int sampleRateIndex);
  int deviceId(Rhd2000DataBlock *dataBlock, int stream, int &register59Value);
  void runInterfaceBoard();
  void stopInterfaceBoard();


 public:
  mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);//constructor
  virtual ~mainWindow();
  
 protected:
  Glib::RefPtr<Gtk::Builder> builder;
  
};

#endif // MAINWINDOW_H
