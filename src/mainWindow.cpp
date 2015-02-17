#include "mainWindow.h"
#include <iostream>


mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{
    openInterfaceBoard();
  
}

mainWindow::~mainWindow()
{
}

void mainWindow::openInterfaceBoard()
{
  // function called from the gui to set up the board
  // so that it is ready to record.
  errorCode=0;
  evalBoard = new Rhd2000EvalBoard;
  // Open Opal Kelly XEM6010 board.
  errorCode = evalBoard->open();
  if (errorCode < 1) 
    {
      if (errorCode == -1) 
  	{
  	  fprintf(stderr,"Cannot load Opal Kelly FrontPanel DLL\n");
  	  fprintf(stderr,"Opal Kelly USB drivers not installed.\n ");
  	} 
      else 
  	{
  	  fprintf(stderr,"Intan RHD2000 USB Interface Board Not Found\n");
  	}
      delete evalBoard;
      evalBoard = 0;
      return;
    }
  
  // Load Rhythm FPGA configuration bitfile (provided by Intan Technologies).
  string bitfilename ="main.bit";
  if (!evalBoard->uploadFpgaBitfile(bitfilename)) {
    fprintf(stderr,"FPGA Configuration File Upload Error\n");
    fprintf(stderr,"Cannot upload configuration file to FPGA.  Make sure file main.bit is in the same directory as the executable file.\n");
    return;
  }
  
  /*
    // Initialize interface board.
    evalBoard->initialize();
    
    // Set sample rate and upload all auxiliary SPI command sequences.
    changeSampleRate(sampleRateComboBox->currentIndex());

    // Select RAM Bank 0 for AuxCmd3 initially, so the ADC is calibrated.
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3, 0);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3, 0);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3, 0);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3, 0);

    // Since our longest command sequence is 60 commands, we run the SPI
    // interface for 60 samples.
    evalBoard->setMaxTimeStep(60);
    evalBoard->setContinuousRunMode(false);

    // Start SPI interface.
    evalBoard->run();

    // Wait for the 60-sample run to complete.
    while (evalBoard->isRunning()) {
        qApp->processEvents();
    }

    // Read the resulting single data block from the USB interface.
    Rhd2000DataBlock *dataBlock = new Rhd2000DataBlock(evalBoard->getNumEnabledDataStreams());
    evalBoard->readDataBlock(dataBlock);

    // We don't need to do anything with this data block; it was used to configure
    // the RHD2000 amplifier chips and to run ADC calibration.
    delete dataBlock;

    // Now that ADC calibration has been performed, we switch to the command sequence
    // that does not execute ADC calibration.
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);

    // Set default configuration for all eight DACs on interface board.
    evalBoard->enableDac(0, false);
    evalBoard->enableDac(1, false);
    evalBoard->enableDac(2, false);
    evalBoard->enableDac(3, false);
    evalBoard->enableDac(4, false);
    evalBoard->enableDac(5, false);
    evalBoard->enableDac(6, false);
    evalBoard->enableDac(7, false);
    evalBoard->selectDacDataStream(0, 0);
    evalBoard->selectDacDataStream(1, 0);
    evalBoard->selectDacDataStream(2, 0);
    evalBoard->selectDacDataStream(3, 0);
    evalBoard->selectDacDataStream(4, 0);
    evalBoard->selectDacDataStream(5, 0);
    evalBoard->selectDacDataStream(6, 0);
    evalBoard->selectDacDataStream(7, 0);
    evalBoard->selectDacDataChannel(0, 0);
    evalBoard->selectDacDataChannel(1, 1);
    evalBoard->selectDacDataChannel(2, 0);
    evalBoard->selectDacDataChannel(3, 0);
    evalBoard->selectDacDataChannel(4, 0);
    evalBoard->selectDacDataChannel(5, 0);
    evalBoard->selectDacDataChannel(6, 0);
    evalBoard->selectDacDataChannel(7, 0);
    evalBoard->setDacManual(Rhd2000EvalBoard::DacManual1, 32768);
    evalBoard->setDacManual(Rhd2000EvalBoard::DacManual2, 32768);
    evalBoard->setDacGain(0);
    evalBoard->setAudioNoiseSuppress(0);
  */
}
