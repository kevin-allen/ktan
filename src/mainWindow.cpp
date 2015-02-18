#include "mainWindow.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include <iostream>


mainWindow::mainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
  Gtk::Window(cobject), builder(refGlade) // call Gtk::Window and builder
{
  cerr << "entering mainWindow::mainWindow()\n";
  
  // Default amplifier bandwidth settings
  desiredLowerBandwidth = 0.1;
  desiredUpperBandwidth = 7500.0;
  desiredDspCutoffFreq = 1.0;
  dspEnabled = true;
  
  // Default electrode impedance measurement frequency
  desiredImpedanceFreq = 1000.0;
  actualImpedanceFreq = 0.0;
  impedanceFreqValid = false;
  
  
  // Set up array for 8 DACs on USB interface board
  dacNumber = 8;
  dacEnabled = new bool[dacNumber];
  for(int i = 0; i < dacNumber; i++)
    dacEnabled[i]=false;


  chipId = new int[MAX_NUM_DATA_STREAMS];
  for(int i = 0; i < MAX_NUM_DATA_STREAMS; i++)
    chipId[i]=-1;

  evalBoardMode=0;  
  openInterfaceBoard();// opel kelly 
  scanPorts(); // intan boards

  cerr << "leaving mainWindow::mainWindow()\n";
}

mainWindow::~mainWindow()
{
  delete[] dacEnabled;
  delete[] chipId;
}

void mainWindow::openInterfaceBoard()
{

  cerr << "entering mainWindow::openInterfaceBoard()\n";
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
  
  
  // Initialize interface board.
  evalBoard->initialize();
  cerr << "board sampling rate: " << evalBoard->getSampleRate() << " Hz\n";

     
    // Set sample rate and upload all auxiliary SPI command sequences.
    // changeSampleRate(sampleRateComboBox->currentIndex());

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
      //  qApp->processEvents();
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
   
  cerr << "leaving mainWindow::openInterfaceBoard()\n";
}



// Scan SPI Ports A-D to identify all connected RHD2000 amplifier chips.
void mainWindow::scanPorts()
{
  cerr << "entering mainWindow::scanPorts()\n";
  // Scan SPI Ports.
  findConnectedAmplifiers();

    /*
    // Configure SignalProcessor object for the required number of data streams.
    if (!synthMode) {
        signalProcessor->allocateMemory(evalBoard->getNumEnabledDataStreams());
        setWindowTitle(tr("Intan Technologies RHD2000 Interface"));
    } else {
        signalProcessor->allocateMemory(1);
        setWindowTitle(tr("Intan Technologies RHD2000 Interface "
                          "(Demonstration Mode with Synthesized Biopotentials)"));
    }

    // Turn on appropriate (optional) LEDs for Ports A-D
    if (!synthMode) {
        ttlOut[11] = 0;
        ttlOut[12] = 0;
        ttlOut[13] = 0;
        ttlOut[14] = 0;
        if (signalSources->signalPort[0].enabled) {
            ttlOut[11] = 1;
        }
        if (signalSources->signalPort[1].enabled) {
            ttlOut[12] = 1;
        }
        if (signalSources->signalPort[2].enabled) {
            ttlOut[13] = 1;
        }
        if (signalSources->signalPort[3].enabled) {
            ttlOut[14] = 1;
        }
        evalBoard->setTtlOut(ttlOut);
    }

    // Switch display to the first port that has an amplifier connected.
    if (signalSources->signalPort[0].enabled) {
        wavePlot->initialize(0);
    } else if (signalSources->signalPort[1].enabled) {
        wavePlot->initialize(1);
    } else if (signalSources->signalPort[2].enabled) {
        wavePlot->initialize(2);
    } else if (signalSources->signalPort[3].enabled) {
        wavePlot->initialize(3);
    } else {
        wavePlot->initialize(4);
        QMessageBox::information(this, tr("No RHD2000 Amplifiers Detected"),
                tr("No RHD2000 amplifiers are connected to the interface board."
                   "<p>Connect amplifier modules and click 'Rescan Ports A-D' under "
                   "the Configure tab."
                   "<p>You may record from analog and digital inputs on the evaluation "
                   "board in the absence of amplifier modules."));
    }

    wavePlot->setSampleRate(boardSampleRate);
    changeTScale(tScaleComboBox->currentIndex());
    changeYScale(yScaleComboBox->currentIndex());

    statusBar()->clearMessage();
  */
  cerr << "leaving mainWindow::scanPorts()\n";
}

void mainWindow::findConnectedAmplifiers()
{


  cerr << "entering mainWindow::findConnectedAmplifiers()\n";
  
  int delay, stream, id, i, channel, port, auxName, vddName;
  int register59Value;
  int numChannelsOnPort[4] = {0, 0, 0, 0};
  int* portIndex;
  int* portIndexOld;
  int* chipIdOld;

  portIndex = new int[MAX_NUM_DATA_STREAMS];
  portIndexOld = new int[MAX_NUM_DATA_STREAMS];
  chipIdOld = new int [MAX_NUM_DATA_STREAMS];

  for (int i = 0; i < MAX_NUM_DATA_STREAMS; i ++)
    {
      chipId[i]=-1;
      chipIdOld[i]=-1;
      portIndexOld[i]=-1;
      portIndex[i]=-1;
    }
    
  Rhd2000EvalBoard::BoardDataSource initStreamPorts[8] = {
    Rhd2000EvalBoard::PortA1,
    Rhd2000EvalBoard::PortA2,
    Rhd2000EvalBoard::PortB1,
    Rhd2000EvalBoard::PortB2,
    Rhd2000EvalBoard::PortC1,
    Rhd2000EvalBoard::PortC2,
    Rhd2000EvalBoard::PortD1,
    Rhd2000EvalBoard::PortD2 };
  
  Rhd2000EvalBoard::BoardDataSource initStreamDdrPorts[8] = {
    Rhd2000EvalBoard::PortA1Ddr,
    Rhd2000EvalBoard::PortA2Ddr,
    Rhd2000EvalBoard::PortB1Ddr,
    Rhd2000EvalBoard::PortB2Ddr,
    Rhd2000EvalBoard::PortC1Ddr,
    Rhd2000EvalBoard::PortC2Ddr,
    Rhd2000EvalBoard::PortD1Ddr,
    Rhd2000EvalBoard::PortD2Ddr };

  
  // Set sampling rate to highest value for maximum temporal resolution.
  changeSampleRate(Rhd2000EvalBoard::SampleRate30000Hz);


  /*

        // Enable all data streams, and set sources to cover one or two chips
        // on Ports A-D.
        evalBoard->setDataSource(0, initStreamPorts[0]);
        evalBoard->setDataSource(1, initStreamPorts[1]);
        evalBoard->setDataSource(2, initStreamPorts[2]);
        evalBoard->setDataSource(3, initStreamPorts[3]);
        evalBoard->setDataSource(4, initStreamPorts[4]);
        evalBoard->setDataSource(5, initStreamPorts[5]);
        evalBoard->setDataSource(6, initStreamPorts[6]);
        evalBoard->setDataSource(7, initStreamPorts[7]);

        portIndexOld[0] = 0;
        portIndexOld[1] = 0;
        portIndexOld[2] = 1;
        portIndexOld[3] = 1;
        portIndexOld[4] = 2;
        portIndexOld[5] = 2;
        portIndexOld[6] = 3;
        portIndexOld[7] = 3;

        evalBoard->enableDataStream(0, true);
        evalBoard->enableDataStream(1, true);
        evalBoard->enableDataStream(2, true);
        evalBoard->enableDataStream(3, true);
        evalBoard->enableDataStream(4, true);
        evalBoard->enableDataStream(5, true);
        evalBoard->enableDataStream(6, true);
        evalBoard->enableDataStream(7, true);

        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA,
                                        Rhd2000EvalBoard::AuxCmd3, 0);
        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB,
                                        Rhd2000EvalBoard::AuxCmd3, 0);
        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC,
                                        Rhd2000EvalBoard::AuxCmd3, 0);
        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD,
                                        Rhd2000EvalBoard::AuxCmd3, 0);

        // Since our longest command sequence is 60 commands, we run the SPI
        // interface for 60 samples.
        evalBoard->setMaxTimeStep(60);
        evalBoard->setContinuousRunMode(false);

        Rhd2000DataBlock *dataBlock =
                new Rhd2000DataBlock(evalBoard->getNumEnabledDataStreams());
        QVector<int> sumGoodDelays(MAX_NUM_DATA_STREAMS, 0);
        QVector<int> indexFirstGoodDelay(MAX_NUM_DATA_STREAMS, -1);
        QVector<int> indexSecondGoodDelay(MAX_NUM_DATA_STREAMS, -1);

        // Run SPI command sequence at all 16 possible FPGA MISO delay settings
        // to find optimum delay for each SPI interface cable.
        for (delay = 0; delay < 16; ++delay) {
            evalBoard->setCableDelay(Rhd2000EvalBoard::PortA, delay);
            evalBoard->setCableDelay(Rhd2000EvalBoard::PortB, delay);
            evalBoard->setCableDelay(Rhd2000EvalBoard::PortC, delay);
            evalBoard->setCableDelay(Rhd2000EvalBoard::PortD, delay);

            // Start SPI interface.
            evalBoard->run();

            // Wait for the 60-sample run to complete.
            while (evalBoard->isRunning()) {
                qApp->processEvents();
            }

            // Read the resulting single data block from the USB interface.
            evalBoard->readDataBlock(dataBlock);

            // Read the Intan chip ID number from each RHD2000 chip found.
            // Record delay settings that yield good communication with the chip.
            for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
                id = deviceId(dataBlock, stream, register59Value);

                if (id == CHIP_ID_RHD2132 || id == CHIP_ID_RHD2216 ||
                        (id == CHIP_ID_RHD2164 && register59Value == REGISTER_59_MISO_A)) {
                    // cout << "Delay: " << delay << " on stream " << stream << " is good." << endl;
                    sumGoodDelays[stream] = sumGoodDelays[stream] + 1;
                    if (indexFirstGoodDelay[stream] == -1) {
                        indexFirstGoodDelay[stream] = delay;
                        chipIdOld[stream] = id;
                    } else if (indexSecondGoodDelay[stream] == -1) {
                        indexSecondGoodDelay[stream] = delay;
                        chipIdOld[stream] = id;
                    }
                }
            }
        }

        // Set cable delay settings that yield good communication with each
        // RHD2000 chip.
        QVector<int> optimumDelay(MAX_NUM_DATA_STREAMS, 0);
        for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
            if (sumGoodDelays[stream] == 1 || sumGoodDelays[stream] == 2) {
                optimumDelay[stream] = indexFirstGoodDelay[stream];
            } else if (sumGoodDelays[stream] > 2) {
                optimumDelay[stream] = indexSecondGoodDelay[stream];
            }
        }

        evalBoard->setCableDelay(Rhd2000EvalBoard::PortA,
                                 qMax(optimumDelay[0], optimumDelay[1]));
        evalBoard->setCableDelay(Rhd2000EvalBoard::PortB,
                                 qMax(optimumDelay[2], optimumDelay[3]));
        evalBoard->setCableDelay(Rhd2000EvalBoard::PortC,
                                 qMax(optimumDelay[4], optimumDelay[5]));
        evalBoard->setCableDelay(Rhd2000EvalBoard::PortD,
                                 qMax(optimumDelay[6], optimumDelay[7]));

//        cout << "Port A cable delay: " << qMax(optimumDelay[0], optimumDelay[1]) << endl;
//        cout << "Port B cable delay: " << qMax(optimumDelay[2], optimumDelay[3]) << endl;
//        cout << "Port C cable delay: " << qMax(optimumDelay[4], optimumDelay[5]) << endl;
//        cout << "Port D cable delay: " << qMax(optimumDelay[6], optimumDelay[7]) << endl;


        cableLengthPortA =
                evalBoard->estimateCableLengthMeters(qMax(optimumDelay[0], optimumDelay[1]));
        cableLengthPortB =
                evalBoard->estimateCableLengthMeters(qMax(optimumDelay[2], optimumDelay[3]));
        cableLengthPortC =
                evalBoard->estimateCableLengthMeters(qMax(optimumDelay[4], optimumDelay[5]));
        cableLengthPortD =
                evalBoard->estimateCableLengthMeters(qMax(optimumDelay[6], optimumDelay[7]));

        delete dataBlock;

    } else {
        // If we are running with synthetic data (i.e., no interface board), just assume
        // that one RHD2132 is plugged into Port A.
        chipIdOld[0] = CHIP_ID_RHD2132;
        portIndexOld[0] = 0;
    }

    // Now that we know which RHD2000 amplifier chips are plugged into each SPI port,
    // add up the total number of amplifier channels on each port and calcualate the number
    // of data streams necessary to convey this data over the USB interface.
    int numStreamsRequired = 0;
    bool rhd2216ChipPresent = false;
    for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
        if (chipIdOld[stream] == CHIP_ID_RHD2216) {
            numStreamsRequired++;
            if (numStreamsRequired <= MAX_NUM_DATA_STREAMS) {
                numChannelsOnPort[portIndexOld[stream]] += 16;
            }
            rhd2216ChipPresent = true;
        }
        if (chipIdOld[stream] == CHIP_ID_RHD2132) {
            numStreamsRequired++;
            if (numStreamsRequired <= MAX_NUM_DATA_STREAMS) {
                numChannelsOnPort[portIndexOld[stream]] += 32;
            }
        }
        if (chipIdOld[stream] == CHIP_ID_RHD2164) {
            numStreamsRequired += 2;
            if (numStreamsRequired <= MAX_NUM_DATA_STREAMS) {
                numChannelsOnPort[portIndexOld[stream]] += 64;
            }
        }
    }

    // If the user plugs in more chips than the USB interface can support, throw
    // up a warning that not all channels will be displayed.
    if (numStreamsRequired > 8) {
        if (rhd2216ChipPresent) {
            QMessageBox::warning(this, tr("Capacity of USB Interface Exceeded"),
                    tr("This RHD2000 USB interface board can support 256 only amplifier channels."
                       "<p>More than 256 total amplifier channels are currently connected.  (Each RHD2216 "
                       "chip counts as 32 channels for USB interface purposes.)"
                       "<p>Amplifier chips exceeding this limit will not appear in the GUI."));
        } else {
            QMessageBox::warning(this, tr("Capacity of USB Interface Exceeded"),
                    tr("This RHD2000 USB interface board can support 256 only amplifier channels."
                       "<p>More than 256 total amplifier channels are currently connected."
                       "<p>Amplifier chips exceeding this limit will not appear in the GUI."));
        }
    }

    // Reconfigure USB data streams in consecutive order to accommodate all connected chips.
    stream = 0;
    for (int oldStream = 0; oldStream < MAX_NUM_DATA_STREAMS; ++oldStream) {
        if ((chipIdOld[oldStream] == CHIP_ID_RHD2216) && (stream < MAX_NUM_DATA_STREAMS)) {
            chipId[stream] = CHIP_ID_RHD2216;
            portIndex[stream] = portIndexOld[oldStream];
            if (!synthMode) {
                evalBoard->enableDataStream(stream, true);
                evalBoard->setDataSource(stream, initStreamPorts[oldStream]);
            }
            stream++;
        } else if ((chipIdOld[oldStream] == CHIP_ID_RHD2132) && (stream < MAX_NUM_DATA_STREAMS)) {
            chipId[stream] = CHIP_ID_RHD2132;
            portIndex[stream] = portIndexOld[oldStream];
            if (!synthMode) {
                evalBoard->enableDataStream(stream, true);
                evalBoard->setDataSource(stream, initStreamPorts[oldStream]);
            }
            stream++ ;
        } else if ((chipIdOld[oldStream] == CHIP_ID_RHD2164) && (stream < MAX_NUM_DATA_STREAMS - 1)) {
            chipId[stream] = CHIP_ID_RHD2164;
            chipId[stream + 1] =  CHIP_ID_RHD2164_B;
            portIndex[stream] = portIndexOld[oldStream];
            portIndex[stream + 1] = portIndexOld[oldStream];
            if (!synthMode) {
                evalBoard->enableDataStream(stream, true);
                evalBoard->enableDataStream(stream + 1, true);
                evalBoard->setDataSource(stream, initStreamPorts[oldStream]);
                evalBoard->setDataSource(stream + 1, initStreamDdrPorts[oldStream]);
            }
            stream += 2;
        }
    }

    // Disable unused data streams.
    for (; stream < MAX_NUM_DATA_STREAMS; ++stream) {
        if (!synthMode) {
            evalBoard->enableDataStream(stream, false);
        }
    }

    // Add channel descriptions to the SignalSources object to create a list of all waveforms.
    for (port = 0; port < 4; ++port) {
        if (numChannelsOnPort[port] == 0) {
            signalSources->signalPort[port].channel.clear();
            signalSources->signalPort[port].enabled = false;
        } else if (signalSources->signalPort[port].numAmplifierChannels() !=
                   numChannelsOnPort[port]) {  // if number of channels on port has changed...
            signalSources->signalPort[port].channel.clear();  // ...clear existing channels...
            // ...and create new ones.
            channel = 0;
            // Create amplifier channels for each chip.
            for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
                if (portIndex[stream] == port) {
                    if (chipId[stream] == CHIP_ID_RHD2216) {
                        for (i = 0; i < 16; ++i) {
                            signalSources->signalPort[port].addAmplifierChannel(channel++, i, stream);
                        }
                    } else if (chipId[stream] == CHIP_ID_RHD2132) {
                        for (i = 0; i < 32; ++i) {
                            signalSources->signalPort[port].addAmplifierChannel(channel++, i, stream);
                        }
                    } else if (chipId[stream] == CHIP_ID_RHD2164) {
                        for (i = 0; i < 32; ++i) {  // 32 channels on MISO A; another 32 on MISO B
                            signalSources->signalPort[port].addAmplifierChannel(channel++, i, stream);
                        }
                    } else if (chipId[stream] == CHIP_ID_RHD2164_B) {
                        for (i = 0; i < 32; ++i) {  // 32 channels on MISO A; another 32 on MISO B
                            signalSources->signalPort[port].addAmplifierChannel(channel++, i, stream);
                        }
                    }
                }
            }
            // Now create auxiliary input channels and supply voltage channels for each chip.
            auxName = 1;
            vddName = 1;
            for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
                if (portIndex[stream] == port) {
                    if (chipId[stream] == CHIP_ID_RHD2216 ||
                            chipId[stream] == CHIP_ID_RHD2132 ||
                            chipId[stream] == CHIP_ID_RHD2164) {
                        signalSources->signalPort[port].addAuxInputChannel(channel++, 0, auxName++, stream);
                        signalSources->signalPort[port].addAuxInputChannel(channel++, 1, auxName++, stream);
                        signalSources->signalPort[port].addAuxInputChannel(channel++, 2, auxName++, stream);
                        signalSources->signalPort[port].addSupplyVoltageChannel(channel++, 0, vddName++, stream);
                    }
                }
            }
        } else {    // If number of channels on port has not changed, don't create new channels (since this
                    // would clear all user-defined channel names.  But we must update the data stream indices
                    // on the port.
            channel = 0;
            // Update stream indices for amplifier channels.
            for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
                if (portIndex[stream] == port) {
                    if (chipId[stream] == CHIP_ID_RHD2216) {
                        for (i = channel; i < channel + 16; ++i) {
                            signalSources->signalPort[port].channel[i].boardStream = stream;
                        }
                        channel += 16;
                    } else if (chipId[stream] == CHIP_ID_RHD2132) {
                        for (i = channel; i < channel + 32; ++i) {
                            signalSources->signalPort[port].channel[i].boardStream = stream;
                        }
                        channel += 32;
                    } else if (chipId[stream] == CHIP_ID_RHD2164) {
                        for (i = channel; i < channel + 32; ++i) {  // 32 channels on MISO A; another 32 on MISO B
                            signalSources->signalPort[port].channel[i].boardStream = stream;
                        }
                        channel += 32;
                    } else if (chipId[stream] == CHIP_ID_RHD2164_B) {
                        for (i = channel; i < channel + 32; ++i) {  // 32 channels on MISO A; another 32 on MISO B
                            signalSources->signalPort[port].channel[i].boardStream = stream;
                        }
                        channel += 32;
                    }
                }
            }
            // Update stream indices for auxiliary channels and supply voltage channels.
            for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) {
                if (portIndex[stream] == port) {
                    if (chipId[stream] == CHIP_ID_RHD2216 ||
                            chipId[stream] == CHIP_ID_RHD2132 ||
                            chipId[stream] == CHIP_ID_RHD2164) {
                        signalSources->signalPort[port].channel[channel++].boardStream = stream;
                        signalSources->signalPort[port].channel[channel++].boardStream = stream;
                        signalSources->signalPort[port].channel[channel++].boardStream = stream;
                        signalSources->signalPort[port].channel[channel++].boardStream = stream;
                    }
                }
            }
        }
    }

    // Update Port A-D radio buttons in GUI

    if (signalSources->signalPort[0].numAmplifierChannels() == 0) {
        signalSources->signalPort[0].enabled = false;
        displayPortAButton->setEnabled(false);
        displayPortAButton->setText(signalSources->signalPort[0].name);
    } else {
        signalSources->signalPort[0].enabled = true;
        displayPortAButton->setEnabled(true);
        displayPortAButton->setText(signalSources->signalPort[0].name +
          " (" + QString::number(signalSources->signalPort[0].numAmplifierChannels()) +
          " channels)");
    }

    if (signalSources->signalPort[1].numAmplifierChannels() == 0) {
        signalSources->signalPort[1].enabled = false;
        displayPortBButton->setEnabled(false);
        displayPortBButton->setText(signalSources->signalPort[1].name);
    } else {
        signalSources->signalPort[1].enabled = true;
        displayPortBButton->setEnabled(true);
        displayPortBButton->setText(signalSources->signalPort[1].name +
          " (" + QString::number(signalSources->signalPort[1].numAmplifierChannels()) +
          " channels)");
    }

    if (signalSources->signalPort[2].numAmplifierChannels() == 0) {
        signalSources->signalPort[2].enabled = false;
        displayPortCButton->setEnabled(false);
        displayPortCButton->setText(signalSources->signalPort[2].name);
    } else {
        signalSources->signalPort[2].enabled = true;
        displayPortCButton->setEnabled(true);
        displayPortCButton->setText(signalSources->signalPort[2].name +
          " (" + QString::number(signalSources->signalPort[2].numAmplifierChannels()) +
          " channels)");
    }

    if (signalSources->signalPort[3].numAmplifierChannels() == 0) {
        signalSources->signalPort[3].enabled = false;
        displayPortDButton->setEnabled(false);
        displayPortDButton->setText(signalSources->signalPort[3].name);
    } else {
        signalSources->signalPort[3].enabled = true;
        displayPortDButton->setEnabled(true);
        displayPortDButton->setText(signalSources->signalPort[3].name +
          " (" + QString::number(signalSources->signalPort[3].numAmplifierChannels()) +
          " channels)");
    }

    if (signalSources->signalPort[0].numAmplifierChannels() > 0) {
        displayPortAButton->setChecked(true);
    } else if (signalSources->signalPort[1].numAmplifierChannels() > 0) {
        displayPortBButton->setChecked(true);
    } else if (signalSources->signalPort[2].numAmplifierChannels() > 0) {
        displayPortCButton->setChecked(true);
    } else if (signalSources->signalPort[3].numAmplifierChannels() > 0) {
        displayPortDButton->setChecked(true);
    } else {
        displayAdcButton->setChecked(true);
    }

    // Return sample rate to original user-selected value.
    changeSampleRate(sampleRateComboBox->currentIndex());

//    signalSources->signalPort[0].print();
//    signalSources->signalPort[1].print();
//    signalSources->signalPort[2].print();
//    signalSources->signalPort[3].print();
*/
  changeSampleRate(Rhd2000EvalBoard::SampleRate20000Hz);
  delete[] portIndex;
  delete[] portIndexOld;
  delete[] chipIdOld;
  
  cerr << "leaving mainWindow::findConnectedAmplifiers()\n";
}


// Change RHD2000 interface board amplifier sample rate.
// This function also updates the Aux1, Aux2, and Aux3 SPI command
// sequences that are used to set RAM registers on the RHD2000 chips.
void mainWindow::changeSampleRate(int sampleRateIndex)
{

  cerr << "Entering MainWindow::changeSampleRate(" << sampleRateIndex << ")\n";

  
  Rhd2000EvalBoard::AmplifierSampleRate sampleRate =
    Rhd2000EvalBoard::SampleRate1000Hz;
  
  // Note: numUsbBlocksToRead is set to give an approximate frame rate of
  // 30 Hz for most sampling rates.
  
  switch (sampleRateIndex) {
  case 0:
    sampleRate = Rhd2000EvalBoard::SampleRate1000Hz;
    boardSampleRate = 1000.0;
    numUsbBlocksToRead = 1;
    break;
  case 1:
    sampleRate = Rhd2000EvalBoard::SampleRate1250Hz;
    boardSampleRate = 1250.0;
    numUsbBlocksToRead = 1;
    break;
  case 2:
    sampleRate = Rhd2000EvalBoard::SampleRate1500Hz;
    boardSampleRate = 1500.0;
    numUsbBlocksToRead = 1;
    break;
  case 3:
    sampleRate = Rhd2000EvalBoard::SampleRate2000Hz;
    boardSampleRate = 2000.0;
    numUsbBlocksToRead = 1;
    break;
  case 4:
    sampleRate = Rhd2000EvalBoard::SampleRate2500Hz;
    boardSampleRate = 2500.0;
    numUsbBlocksToRead = 1;
    break;
  case 5:
    sampleRate = Rhd2000EvalBoard::SampleRate3000Hz;
    boardSampleRate = 3000.0;
    numUsbBlocksToRead = 2;
    break;
  case 6:
    sampleRate = Rhd2000EvalBoard::SampleRate3333Hz;
    boardSampleRate = 10000.0 / 3.0;
    numUsbBlocksToRead = 2;
    break;
  case 7:
    sampleRate = Rhd2000EvalBoard::SampleRate4000Hz;
    boardSampleRate = 4000.0;
    numUsbBlocksToRead = 2;
    break;
  case 8:
    sampleRate = Rhd2000EvalBoard::SampleRate5000Hz;
    boardSampleRate = 5000.0;
    numUsbBlocksToRead = 3;
    break;
  case 9:
    sampleRate = Rhd2000EvalBoard::SampleRate6250Hz;
    boardSampleRate = 6250.0;
    numUsbBlocksToRead = 3;
    break;
  case 10:
    sampleRate = Rhd2000EvalBoard::SampleRate8000Hz;
    boardSampleRate = 8000.0;
    numUsbBlocksToRead = 4;
    break;
  case 11:
    sampleRate = Rhd2000EvalBoard::SampleRate10000Hz;
    boardSampleRate = 10000.0;
    numUsbBlocksToRead = 6;
    break;
  case 12:
    sampleRate = Rhd2000EvalBoard::SampleRate12500Hz;
    boardSampleRate = 12500.0;
    numUsbBlocksToRead = 7;
    break;
  case 13:
    sampleRate = Rhd2000EvalBoard::SampleRate15000Hz;
    boardSampleRate = 15000.0;
    numUsbBlocksToRead = 8;
    break;
  case 14:
    sampleRate = Rhd2000EvalBoard::SampleRate20000Hz;
    boardSampleRate = 20000.0;
    numUsbBlocksToRead = 12;
    break;
  case 15:
    sampleRate = Rhd2000EvalBoard::SampleRate25000Hz;
    boardSampleRate = 25000.0;
    numUsbBlocksToRead = 14;
    break;
  case 16:
    sampleRate = Rhd2000EvalBoard::SampleRate30000Hz;
    boardSampleRate = 30000.0;
    numUsbBlocksToRead = 16;
    break;
  }
  
  cout << "New sampling rate " << boardSampleRate << " Hz\n";

  // Set up an RHD2000 register object using this sample rate to
  // optimize MUX-related register settings.
  Rhd2000Registers chipRegisters(boardSampleRate);
  
  
  int commandSequenceLength;
  vector<int> commandList;

  evalBoard->setSampleRate(sampleRate);
  
  
  // Now that we have set our sampling rate, we can set the MISO sampling delay
  // which is dependent on the sample rate.

  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortA, cableLengthPortA);
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortB, cableLengthPortB);
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortC, cableLengthPortC);
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortD, cableLengthPortD);
  
  
  // Create command lists to be uploaded to auxiliary command slots.
  
  // Create a command list for the AuxCmd1 slot.  This command sequence will create a 250 Hz,
  // zero-amplitude sine wave (i.e., a flatline).  We will change this when we want to perform
  // impedance testing.
  // commandSequenceLength = chipRegisters.createCommandListZcheckDac(commandList, 250.0, 0.0);
  
  // Create a command list for the AuxCmd1 slot.  This command sequence will continuously
  // update Register 3, which controls the auxiliary digital output pin on each RHD2000 chip.
  // In concert with the v1.4 Rhythm FPGA code, this permits real-time control of the digital
  // output pin on chips on each SPI port.
  chipRegisters.setDigOutLow();   // Take auxiliary output out of HiZ mode.
  
  commandSequenceLength = chipRegisters.createCommandListUpdateDigOut(commandList);
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd1, 0);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd1, 0, commandSequenceLength - 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd1, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd1, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd1, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd1, 0);

  // Next, we'll create a command list for the AuxCmd2 slot.  This command sequence
  // will sample the temperature sensor and other auxiliary ADC inputs.
  
  commandSequenceLength = chipRegisters.createCommandListTempSensor(commandList);
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd2, 0);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd2, 0, commandSequenceLength - 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd2, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd2, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd2, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd2, 0);
  
  // For the AuxCmd3 slot, we will create three command sequences.  All sequences
  // will configure and read back the RHD2000 chip registers, but one sequence will
  // also run ADC calibration.  Another sequence will enable amplifier 'fast settle'.
  
  // Before generating register configuration command sequences, set amplifier
  // bandwidth paramters.


  /*
    actualDspCutoffFreq = chipRegisters.setDspCutoffFreq(desiredDspCutoffFreq);
    actualLowerBandwidth = chipRegisters.setLowerBandwidth(desiredLowerBandwidth);
    actualUpperBandwidth = chipRegisters.setUpperBandwidth(desiredUpperBandwidth);
    chipRegisters.enableDsp(dspEnabled);

    if (dspEnabled) {
        dspCutoffFreqLabel->setText("Desired/Actual DSP Cutoff: " +
                                    QString::number(desiredDspCutoffFreq, 'f', 2) + " Hz / " +
                                    QString::number(actualDspCutoffFreq, 'f', 2) + " Hz");
    } else {
        dspCutoffFreqLabel->setText("Desired/Actual DSP Cutoff: DSP disabled");
    }

    if (!synthMode) {
        chipRegisters.createCommandListRegisterConfig(commandList, true);
        // Upload version with ADC calibration to AuxCmd3 RAM Bank 0.
        evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 0);
        evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0,
                                          commandSequenceLength - 1);

        commandSequenceLength = chipRegisters.createCommandListRegisterConfig(commandList, false);
        // Upload version with no ADC calibration to AuxCmd3 RAM Bank 1.
        evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 1);
        evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0,
                                          commandSequenceLength - 1);

        chipRegisters.setFastSettle(true);
        commandSequenceLength = chipRegisters.createCommandListRegisterConfig(commandList, false);
        // Upload version with fast settle enabled to AuxCmd3 RAM Bank 2.
        evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 2);
        evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0,
                                          commandSequenceLength - 1);
        chipRegisters.setFastSettle(false);

        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3,
                                        fastSettleEnabled ? 2 : 1);
        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3,
                                        fastSettleEnabled ? 2 : 1);
        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3,
                                        fastSettleEnabled ? 2 : 1);
        evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3,
                                        fastSettleEnabled ? 2 : 1);
    }

  
    signalProcessor->setNotchFilter(notchFilterFrequency, notchFilterBandwidth, boardSampleRate);
    signalProcessor->setHighpassFilter(highpassFilterFrequency, boardSampleRate);

    evalBoard->setDacHighpassFilter(highpassFilterFrequency);
  
  

    impedanceFreqValid = false;
    updateImpedanceFrequency();
  
    */


  cerr << "Leaving MainWindow::changeSampleRate(int sampleRateIndex)\n";
}
