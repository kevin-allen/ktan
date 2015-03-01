//#define DEBUG_ACQ
#include "acquisition.h"
#include "rhd2000evalboard.h"
#include "rhd2000datablock.h"
#include "rhd2000registers.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>
#include <unistd.h>

acquisition::acquisition(dataBuffer* dbuffer)
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::acquisition()\n";
#endif
  set_successfully=false;

  db=dbuffer; // get the address of the data buffer
  localBuffer=NULL;

  is_acquiring = false;

  // Default amplifier bandwidth settings
  desiredLowerBandwidth = 0.1;
  desiredUpperBandwidth = 7500.0;
  desiredDspCutoffFreq = 1.0;
  dspEnabled = true;
  desiredImpedanceFreq = 1000.0;
  actualImpedanceFreq = 0.0;
  impedanceFreqValid = false;

  // Set up array for 8 DACs on USB interface board
  numDacs = 8;
  dacEnabled = new bool[numDacs];
  for(int i = 0; i < numDacs; i++)
    dacEnabled[i]=false;
  
  // Set up array for the 4 ports
  numPorts=4; 
  portEnabled = new bool[numPorts];
  for(int i = 0; i < numPorts; i++)
    portEnabled[i]=false;
  
  chipId = new int[MAX_NUM_DATA_STREAMS];
  for(int i = 0; i < MAX_NUM_DATA_STREAMS; i++)
    chipId[i]=-1;

  for (int i = 0; i < 16; ++i)
    ttlOut[i] = 0;

  manualDelayEnabled = new bool[4];
  for(int i; i < 4; i++)
    manualDelayEnabled[i]=false;
  manualDelay = new int[4];
  for(int i; i < 4; i++)
    manualDelay[i]=0;


  auxDigOutEnabled = new bool[4];
  auxDigOutChannel = new int[4];
  for(int i = 0;i<4;i++)
    {
      auxDigOutEnabled[i]=false;
      auxDigOutChannel[i]=0;
    }



  evalBoard = new Rhd2000EvalBoard;

  if(openBoardBit()==false)
    {
      cerr << "acquisition::acquisition(), problem opening the board\n";
      return ;
    }
  
  /*************************************
   code to be replaced by minimal code 
  ************************************/
  evalBoardMode=0;  
  openInterfaceBoard();// opel kelly 
  findConnectedAmplifiers(); // intan boards

  //  evalBoard->enableDacHighpassFilter(false);
  //  evalBoard->setDacHighpassFilter(250.0);
  //  updateAuxDigOut();
  /************************************
  end of replacement
  **************************************/


  // small buffer where we put the data comming from usb buffer before sending into the mainWindow dataBuffer
  // contains only one usb block read.
  totalNumChannels=numAmplifierChannels;
  localBuffer = new short int [numStreams*SAMPLES_PER_DATA_BLOCK*numUsbBlocksToRead*totalNumChannels];
  db->setNumChannels(totalNumChannels);

  settingAmp();

  is_acquiring=false;
  inter_acquisition_sleep_ms=ACQUISITION_SLEEP_TIME_MS;
  inter_acquisition_sleep_timespec=tk.set_timespec_from_ms(inter_acquisition_sleep_ms);
  pause_restart_acquisition_thread_ms=100;
  timespec_pause_restat_acquisition_thread=tk.set_timespec_from_ms(pause_restart_acquisition_thread_ms);
    
  
  
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::acquisition()\n";
#endif



}

acquisition::~acquisition()
{

#ifdef DEBUG_ACQ
  cerr << "entering acquisition::~acquisition()\n";
#endif


  delete[] dacEnabled;
  delete[] portEnabled;
  delete[] chipId;
  delete[] manualDelayEnabled;
  delete[] manualDelay;
  delete[] auxDigOutEnabled;
  delete[] auxDigOutChannel;
  if(localBuffer!=NULL)
    delete[] localBuffer;
  delete evalBoard;
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::~acquisition()\n";
#endif
}

bool acquisition::openBoardBit()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::openBoardBit()\n";
#endif

  // Open Opal Kelly XEM6010 board.
  errorCode = evalBoard->open();
  if (errorCode < 1) 
    {
      if (errorCode == -1) 
  	{
  	  cerr << "Cannot load Opal Kelly FrontPanel DLL\n";
  	  cerr << "Opal Kelly USB drivers not installed.\n ";
  	} 
      else 
  	{
  	  cerr << "Intan RHD2000 USB Interface Board Not Found\n";
  	}
      evalBoard = 0;
      return false;
    }
  // Load Rhythm FPGA configuration bitfile (provided by Intan Technologies).
  string bitfilename ="main.bit";
  if (!evalBoard->uploadFpgaBitfile(bitfilename)) {
    cerr << "FPGA Configuration File Upload Error\n";
    cerr << "Cannot upload configuration file to FPGA.  Make sure file main.bit is in the same directory as the executable file.\n";
    return false;
  }
  
  // Initialize interface board.
  evalBoard->initialize();

  

#ifdef DEBUG_ACQ
    cerr << "board sampling rate: " << evalBoard->getSampleRate() << " Hz\n";
  cerr << "leaving acquisition::openBoardBit()\n";
#endif

  return true;

}
int acquisition::get_number_channels()
{
  return totalNumChannels;
}
bool acquisition::get_set_successfully()
{
  return set_successfully;
}


void acquisition::settingAmp()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::settingAmp()\n";
#endif


  // Set up an RHD2000 register object using this sample rate.
  Rhd2000Registers *chipRegisters;
  chipRegisters = new Rhd2000Registers(evalBoard->getSampleRate());
  // Create command lists to be uploaded to auxiliary command slots.
  int commandSequenceLength;
  vector<int> commandList;
  // First, let's create a command list for the AuxCmd1 slot. This command
  // sequence will create a 1 kHz, full-scale sine wave for impedance testing.
  commandSequenceLength = chipRegisters->createCommandListZcheckDac(commandList,1000.0, 128.0);
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd1, 0);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd1, 0,commandSequenceLength - 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd1, 0);
  
  // evalBoard->printCommandList(commandList); // optionally, print command list
  // Next, we'll create a command list for the AuxCmd2 slot. This command sequence
  // will sample the temperature sensor and other auxiliary ADC inputs.
  commandSequenceLength = chipRegisters->createCommandListTempSensor(commandList);
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd2, 0);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd2, 0, commandSequenceLength - 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd2, 0);
  // evalBoard->printCommandList(commandList); // optionally, print command list
  
  // For the AuxCmd3 slot, we will create two command sequences. Both sequences
  // will configure and read back the RHD2000 chip registers, but one sequence will
  // also run ADC calibration.
  // Before generating register configuration command sequences, set amplifier
  // bandwidth paramters.
  double dspCutoffFreq;
  dspCutoffFreq = chipRegisters->setDspCutoffFreq(10.0); // 10 Hz DSP cutoff
  cout << "Actual DSP cutoff frequency: " << dspCutoffFreq << " Hz" << endl;
  chipRegisters->setLowerBandwidth(1.0);
  chipRegisters->setUpperBandwidth(7500.0);
  // 1.0 Hz lower bandwidth
  // 7.5 kHz upper bandwidth
  commandSequenceLength = chipRegisters->createCommandListRegisterConfig(commandList, false);
  // Upload version with no ADC calibration to AuxCmd3 RAM Bank 0.
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 0);
  chipRegisters->createCommandListRegisterConfig(commandList, true);
  // Upload version with ADC calibration to AuxCmd3 RAM Bank 1.
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 1);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0, commandSequenceLength - 1);
  // Select RAM Bank 1 for AuxCmd3 initially, so the ADC is calibrated.
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA,Rhd2000EvalBoard::AuxCmd3, 1);


  // evalBoard->printCommandList(commandList); // optionally, print command list
  // Since our longest command sequence is 60 commands, let’s just run the SPI
  // interface for 60 samples.
  evalBoard->setMaxTimeStep(60);
  evalBoard->setContinuousRunMode(false);
  // Start SPI interface.
  evalBoard->run();
  // Wait for the 60-sample run to complete.
  while (evalBoard->isRunning()) { }
  // Read the resulting single data block from the USB interface.
  Rhd2000DataBlock *dataBlock =
    new Rhd2000DataBlock(evalBoard->getNumEnabledDataStreams());
  evalBoard->readDataBlock(dataBlock);
  // Display register contents from data stream 0.
  //  dataBlock->print(0);

  // Now that ADC calibration has been performed, we switch to the command sequence
  // that does not execute ADC calibration.
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA,Rhd2000EvalBoard::AuxCmd3, 0);
  delete dataBlock;
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::settingAmp()\n";
#endif
  

}

bool acquisition::get_is_acquiring()
{
  return is_acquiring;
}



void acquisition::openInterfaceBoard()
{
 
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::openInterfaceBoard()\n";
#endif


  // function called from the gui to set up the board
  // so that it is ready to record.


  
  evalBoardMode = evalBoard->getBoardMode();
  cerr << "evaluation board mode: " << evalBoardMode << '\n';

  changeSampleRate(14);

  // upload all auxiliary SPI command sequences.
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
  while (evalBoard->isRunning()) {}
  
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
  evalBoard->selectDacDataStream(0, 8);   // Initially point DACs to DacManual1 input
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
  evalBoard->setDacManual(32768);
  evalBoard->setDacGain(0);
  evalBoard->setAudioNoiseSuppress(0);
  
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortA, 0.0);
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortB, 0.0);
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortC, 0.0);
  evalBoard->setCableLengthMeters(Rhd2000EvalBoard::PortD, 0.0);

#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::openInterfaceBoard()\n";
#endif

 }

void acquisition::findConnectedAmplifiers()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::findConnectedAmplifiers()\n";
#endif
  
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
  //  changeSampleRate(Rhd2000EvalBoard::SampleRate30000Hz);


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
  
  
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA,Rhd2000EvalBoard::AuxCmd3, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB,Rhd2000EvalBoard::AuxCmd3, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC,Rhd2000EvalBoard::AuxCmd3, 0);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD,Rhd2000EvalBoard::AuxCmd3, 0);
  
  // Since our longest command sequence is 60 commands, we run the SPI
  // interface for 60 samples.
  evalBoard->setMaxTimeStep(60);
  evalBoard->setContinuousRunMode(false);

  
  Rhd2000DataBlock *dataBlock =
    new Rhd2000DataBlock(evalBoard->getNumEnabledDataStreams());
  int* sumGoodDelays;
  int* indexFirstGoodDelay;
  int* indexSecondGoodDelay;
  sumGoodDelays = new int[MAX_NUM_DATA_STREAMS];
  indexFirstGoodDelay = new int[MAX_NUM_DATA_STREAMS];
  indexSecondGoodDelay = new int[MAX_NUM_DATA_STREAMS];
  for(int i = 0; i < MAX_NUM_DATA_STREAMS; i++)
    {
      sumGoodDelays[i]=0;
      indexFirstGoodDelay[i]=-1;
      indexSecondGoodDelay[i]=-1;
    }

  
  // Run SPI command sequence at all 16 possible FPGA MISO delay settings
  // to find optimum delay for each SPI interface cable.
  for (delay = 0; delay < 16; ++delay) 
    {
      evalBoard->setCableDelay(Rhd2000EvalBoard::PortA, delay);
      evalBoard->setCableDelay(Rhd2000EvalBoard::PortB, delay);
      evalBoard->setCableDelay(Rhd2000EvalBoard::PortC, delay);
      evalBoard->setCableDelay(Rhd2000EvalBoard::PortD, delay);
      
      // Start SPI interface.
      evalBoard->run();
      
      // Wait for the 60-sample run to complete.
      while (evalBoard->isRunning())
	{
	//qApp->processEvents();
	}
      
      // Read the resulting single data block from the USB interface.
      evalBoard->readDataBlock(dataBlock);
      
      // Read the Intan chip ID number from each RHD2000 chip found.
      // Record delay settings that yield good communication with the chip.
      for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
	{
	  id = deviceId(dataBlock, stream, register59Value);
	  if (id == CHIP_ID_RHD2132 || id == CHIP_ID_RHD2216 || (id == CHIP_ID_RHD2164 && register59Value == REGISTER_59_MISO_A)) 
	    {
	      cerr << "Delay: " << delay << " on stream " << stream << " is good." << endl;
	      numChips++;
	      sumGoodDelays[stream] = sumGoodDelays[stream] + 1;
	      if (indexFirstGoodDelay[stream] == -1) 
		{
		  indexFirstGoodDelay[stream] = delay;
		  chipIdOld[stream] = id;
		} 
	      else if (indexSecondGoodDelay[stream] == -1) {
		indexSecondGoodDelay[stream] = delay;
		chipIdOld[stream] = id;
	      }
	    }
	}
      
    }

  // count the number of Intan chips 
  numChips=0;
  for(stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
    {
      if(sumGoodDelays[stream]>0)
	numChips++;
    
    }
  cerr << "Number of Intan chips: " << numChips << '\n';


  // Set cable delay settings that yield good communication with each
  // RHD2000 chip.
  int* optimumDelay;
  optimumDelay = new int [MAX_NUM_DATA_STREAMS];
  for(int i = 0; i < MAX_NUM_DATA_STREAMS; i++)
    optimumDelay[i]=0;

  for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
    {
      if (sumGoodDelays[stream] == 1 || sumGoodDelays[stream] == 2) 
	{
	  optimumDelay[stream] = indexFirstGoodDelay[stream];
	} 
      else if (sumGoodDelays[stream] > 2) 
	{
	  optimumDelay[stream] = indexSecondGoodDelay[stream];
	}
    }

  if(optimumDelay[0]>optimumDelay[1])
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortA,optimumDelay[0]);
  else
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortA,optimumDelay[1]);
  if(optimumDelay[2]>optimumDelay[3])
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortB,optimumDelay[2]);
  else
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortB,optimumDelay[3]);
  if(optimumDelay[4]>optimumDelay[5])
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortC,optimumDelay[4]);
  else
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortC,optimumDelay[5]);
  if(optimumDelay[6]>optimumDelay[7])
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortD,optimumDelay[6]);
  else
    evalBoard->setCableDelay(Rhd2000EvalBoard::PortD,optimumDelay[7]);

  if(optimumDelay[0]>optimumDelay[1])
    cableLengthPortA = evalBoard->estimateCableLengthMeters(optimumDelay[0]);
  else
    cableLengthPortA = evalBoard->estimateCableLengthMeters(optimumDelay[1]);
							    
  if(optimumDelay[2]>optimumDelay[3])
    cableLengthPortB = evalBoard->estimateCableLengthMeters(optimumDelay[2]);
  else
    cableLengthPortB = evalBoard->estimateCableLengthMeters(optimumDelay[3]);

  if(optimumDelay[4]>optimumDelay[5])
    cableLengthPortC = evalBoard->estimateCableLengthMeters(optimumDelay[4]);
  else
    cableLengthPortC = evalBoard->estimateCableLengthMeters(optimumDelay[5]);

  if(optimumDelay[6]>optimumDelay[7])
    cableLengthPortD = evalBoard->estimateCableLengthMeters(optimumDelay[6]);
  else
    cableLengthPortD = evalBoard->estimateCableLengthMeters(optimumDelay[7]);

  // dataBlock no longer needed
  delete dataBlock;

  
  // Now that we know which RHD2000 amplifier chips are plugged into each SPI port,
  // add up the total number of amplifier channels on each port and calcualate the number
  // of data streams necessary to convey this data over the USB interface.
  int numStreamsRequired = 0;
  bool rhd2216ChipPresent = false;
  for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
    {
      if (chipIdOld[stream] == CHIP_ID_RHD2216) 
	{
	  numStreamsRequired++;
	  if (numStreamsRequired <= MAX_NUM_DATA_STREAMS) 
	    {
	      numChannelsOnPort[portIndexOld[stream]] += 16;
	    }
	  rhd2216ChipPresent = true;
	}
      if (chipIdOld[stream] == CHIP_ID_RHD2132) 
	{
	  numStreamsRequired++;
	  if (numStreamsRequired <= MAX_NUM_DATA_STREAMS) 
	    {
	      numChannelsOnPort[portIndexOld[stream]] += 32;
	    }
	}
      if (chipIdOld[stream] == CHIP_ID_RHD2164) 
	{
	  numStreamsRequired += 2;
	  if (numStreamsRequired <= MAX_NUM_DATA_STREAMS) 
	    {
	      numChannelsOnPort[portIndexOld[stream]] += 64;
	    }
	}
    }
  
  cerr << "number of streams required: " << numStreamsRequired << '\n';
  
  numAmplifierChannels=0;
  cerr << "number of amplifier channels on ports: \n";
  for (int i = 0; i < 4; i++)
    {
      cerr << "port " << i << " : " << numChannelsOnPort[i] << " channels\n";
      numAmplifierChannels+=numChannelsOnPort[i];
    }
  cerr << "total number of amplifier channels: " << numAmplifierChannels << '\n';


    // If the user plugs in more chips than the USB interface can support, throw
    // up a warning that not all channels will be displayed.
  if (numStreamsRequired > 8) 
    cerr << "Capacity of USB Interface Exceeded\n"
	 << "This RHD2000 USB interface board can support only 256  amplifier channels.\n";
      

  
  // Reconfigure USB data streams in consecutive order to accommodate all connected chips.
  stream = 0;
  for (int oldStream = 0; oldStream < MAX_NUM_DATA_STREAMS; ++oldStream) 
    {
      if ((chipIdOld[oldStream] == CHIP_ID_RHD2216) && (stream < MAX_NUM_DATA_STREAMS)) 
	{
	  chipId[stream] = CHIP_ID_RHD2216;
	  portIndex[stream] = portIndexOld[oldStream];
	  evalBoard->enableDataStream(stream, true);
	  evalBoard->setDataSource(stream, initStreamPorts[oldStream]);
	  stream++;
	} 
      else if ((chipIdOld[oldStream] == CHIP_ID_RHD2132) && (stream < MAX_NUM_DATA_STREAMS)) 
	{
	  chipId[stream] = CHIP_ID_RHD2132;
	  portIndex[stream] = portIndexOld[oldStream];
	  evalBoard->enableDataStream(stream, true);
	  evalBoard->setDataSource(stream, initStreamPorts[oldStream]);
	  stream++ ;
	} 
      else if ((chipIdOld[oldStream] == CHIP_ID_RHD2164) && (stream < MAX_NUM_DATA_STREAMS - 1)) 
	{
	  chipId[stream] = CHIP_ID_RHD2164;
	  chipId[stream + 1] =  CHIP_ID_RHD2164_B;
	  portIndex[stream] = portIndexOld[oldStream];
	  portIndex[stream + 1] = portIndexOld[oldStream];
	  evalBoard->enableDataStream(stream, true);
	  evalBoard->enableDataStream(stream + 1, true);
	  evalBoard->setDataSource(stream, initStreamPorts[oldStream]);
	  evalBoard->setDataSource(stream + 1, initStreamDdrPorts[oldStream]);
	  stream += 2;
	}
    }

  cerr << "number of usb stream enabled: " << stream << "\n";
  numStreams=stream;

  // Disable unused data streams.
  for (; stream < MAX_NUM_DATA_STREAMS; ++stream)
    evalBoard->enableDataStream(stream, false);
  
  
  // enable the ports as needed
  for (port = 0; port < 4; ++port) 
    {
      if (numChannelsOnPort[port] == 0) 
	portEnabled[port]=false;
      else
	portEnabled[port]=true;
    }

  changeSampleRate(Rhd2000EvalBoard::SampleRate20000Hz);
  
  

  delete[] portIndex;
  delete[] portIndexOld;
  delete[] chipIdOld;
  delete[] sumGoodDelays;
  delete[] indexFirstGoodDelay;
  delete[] indexSecondGoodDelay;
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::findConnectedAmplifiers()\n";
#endif
}


// Change RHD2000 interface board amplifier sample rate.
// This function also updates the Aux1, Aux2, and Aux3 SPI command
// sequences that are used to set RAM registers on the RHD2000 chips.
void acquisition::changeSampleRate(int sampleRateIndex)
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::changeSampleRate()\n";
#endif
  
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
  
  cerr << "New sampling rate " << boardSampleRate << " Hz\n";

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
  commandSequenceLength = chipRegisters.createCommandListZcheckDac(commandList, 250.0, 0.0);
  
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
  actualDspCutoffFreq = chipRegisters.setDspCutoffFreq(desiredDspCutoffFreq);
  actualLowerBandwidth = chipRegisters.setLowerBandwidth(desiredLowerBandwidth);
  actualUpperBandwidth = chipRegisters.setUpperBandwidth(desiredUpperBandwidth);
  chipRegisters.enableDsp(dspEnabled);

  cerr << "actual Dsp cutoff frequency : " << actualDspCutoffFreq << '\n';
  cerr << "actual lower bandwidth: " << actualLowerBandwidth << '\n';
  cerr << "actual upper bandwidth: " << actualUpperBandwidth << '\n';

  chipRegisters.createCommandListRegisterConfig(commandList, true);
  // Upload version with ADC calibration to AuxCmd3 RAM Bank 0.
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 0);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0, commandSequenceLength - 1);

  commandSequenceLength = chipRegisters.createCommandListRegisterConfig(commandList, false);
  // Upload version with no ADC calibration to AuxCmd3 RAM Bank 1.
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 1);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0, commandSequenceLength - 1);

  chipRegisters.setFastSettle(true);
  commandSequenceLength = chipRegisters.createCommandListRegisterConfig(commandList, false);
  // Upload version with fast settle enabled to AuxCmd3 RAM Bank 2.
  evalBoard->uploadCommandList(commandList, Rhd2000EvalBoard::AuxCmd3, 2);
  evalBoard->selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0,commandSequenceLength - 1);
  chipRegisters.setFastSettle(false);
  
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3,
				  fastSettleEnabled ? 2 : 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3,
				  fastSettleEnabled ? 2 : 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3,
				  fastSettleEnabled ? 2 : 1);
  evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3,
				  fastSettleEnabled ? 2 : 1);
  
  //  evalBoard->setDacHighpassFilter(250); // what is that

  // there was possibility to set up highpassFilter here
  // signalProcessor->setNotchFilter(notchFilterFrequency, notchFilterBandwidth, boardSampleRate);
  // signalProcessor->setHighpassFilter(highpassFilterFrequency, boardSampleRate);
  // evalBoard->setDacHighpassFilter(highpassFilterFrequency);
  
  // some impedance stuff
  //impedanceFreqValid = false;
  //updateImpedanceFrequency();


  db->set_sampling_rate(evalBoard->getSampleRate());
  
#ifdef DEBUG_ACQ
  cerr << "leave acquisition::changeSampleRate()\n";
#endif

}


int acquisition::deviceId(Rhd2000DataBlock *dataBlock, int stream, int &register59Value)
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::deviceId()\n";
#endif

    bool intanChipPresent;
    // First, check ROM registers 32-36 to verify that they hold 'INTAN', and
    // the initial chip name ROM registers 24-26 that hold 'RHD'.
    // This is just used to verify that we are getting good data over the SPI
    // communication channel.
    intanChipPresent = ((char) dataBlock->auxiliaryData[stream][2][32] == 'I' &&
                        (char) dataBlock->auxiliaryData[stream][2][33] == 'N' &&
                        (char) dataBlock->auxiliaryData[stream][2][34] == 'T' &&
                        (char) dataBlock->auxiliaryData[stream][2][35] == 'A' &&
                        (char) dataBlock->auxiliaryData[stream][2][36] == 'N' &&
                        (char) dataBlock->auxiliaryData[stream][2][24] == 'R' &&
                        (char) dataBlock->auxiliaryData[stream][2][25] == 'H' &&
                        (char) dataBlock->auxiliaryData[stream][2][26] == 'D');

    // If the SPI communication is bad, return -1.  Otherwise, return the Intan
    // chip ID number stored in ROM regstier 63.

    if (!intanChipPresent) {
      register59Value = -1;
      return -1;
    } else {

      register59Value = dataBlock->auxiliaryData[stream][2][23]; // Register 59
      return dataBlock->auxiliaryData[stream][2][19]; // chip ID (Register 63)
    }
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::deviceId()\n";
#endif

}

bool acquisition::start_acquisition()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::start_acquisition()\n";
#endif
  if(is_acquiring==true)
    return true;
 
  // Turn LEDs on to indicate that data acquisition is running.
  ttlOut[15] = 1;
  for (int i = 0; i < 8; i++)
    ledArray[i]=0;
  ledArray[0]=1;
  evalBoard->setLedDisplay(ledArray);
  evalBoard->setTtlOut(ttlOut);
  // set some variables for acquisition
  dataBlockSize = Rhd2000DataBlock::calculateDataBlockSizeInWords(evalBoard->getNumEnabledDataStreams());
  numBlocksLoaded=0;
  samplePeriod = 1.0 / boardSampleRate;
  fifoCapacity = Rhd2000EvalBoard::fifoCapacityInWords();
  // start the board
  evalBoard->setContinuousRunMode(true);
  evalBoard->run();
  is_acquiring = true;

#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::start_acquisition()\n";
#endif

  return true;
}




bool acquisition::stop_acquisition()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::stop_acquisition()\n";
#endif
 
  
  if(is_acquiring==false)
    return true;
  
  is_acquiring = false;
  
  usleep(50000); // allow thread to die

#ifdef DEBUG_ACQ
  cerr << "stop_acquisition: setContinuousRunMode(false)\n";
#endif
  evalBoard->setContinuousRunMode(false);

#ifdef DEBUG_ACQ
  cerr << "stop_acquisition: setMaxTimeStep(0)\n";
#endif

  evalBoard->setMaxTimeStep(0);

#ifdef DEBUG_ACQ
  cerr << "stop_acquisition: flush()\n";
#endif



  // Flush USB FIFO on XEM6010
  evalBoard->flush();
  turnOffLED();

  
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::stop_acquisition()\n";
#endif

  return true;
}

void acquisition::checkFifoOK()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::checkFifoOK()\n";
#endif

  // Check the number of words stored in the Opal Kelly USB interface FIFO.
  wordsInFifo = evalBoard->numWordsInFifo();
  latency = 1000.0 * Rhd2000DataBlock::getSamplesPerDataBlock() * (wordsInFifo / dataBlockSize) * samplePeriod;
  fifoPercentageFull = 100.0 * wordsInFifo / fifoCapacity;
  
  // Alert the user if the number of words in the FIFO is getting to be significant
  // or nearing FIFO capacity.
  if (latency > 25.0) 
    cerr << "fifo latency: " << latency << ", this is too long\n";
  if (fifoPercentageFull > 75.0) 
    cerr << "fifo percentage full: " << fifoPercentageFull << "%\n";
  
  // If the USB interface FIFO (on the FPGA board) exceeds 99% full, halt
  // data acquisition and display a warning message.
  if (fifoPercentageFull > 99.0) 
    {
      is_acquiring = false;
      // Stop data acquisition
      evalBoard->setContinuousRunMode(false);
      evalBoard->setMaxTimeStep(0);
      // Turn off LED.
      turnOffLED();
      // warn user
      cerr << "USB Buffer Overrun Error\n";
      cerr << "Acquisition was stop\n";
      cerr << "This happens when the host computer \n"
	   << "cannot keep up with the data streaming from the interface board.\n";
    }
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::checkFifoOK()\n";
#endif

}

void *acquisition::acquisition_thread_function(void)
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::acquisition_thread_function()\n";
#endif

  ledIndex=0;
  while(is_acquiring==true)
    {
      // this puts the new usb blocks into dataQueue 
      newDataReady = evalBoard->readDataBlocks(numUsbBlocksToRead, dataQueue);    // takes about 17 ms at 30 kS/s with 256 amplifiers
      // If new data is ready, then read it.
      if (newDataReady) 
	{
	  // Read waveform data from USB interface board.
	  move_to_dataBuffer();

	  // play with led to impress visitors
	  advanceLED();
	  // check if we are fast enough to prevent buffer overflow in opal kelly board
	  checkFifoOK();
	}
      // take a break here instead of looping 100% of PCU
      nanosleep(&inter_acquisition_sleep_timespec,&req);
    }
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::acquisition_thread_function()\n";
#endif

}

void acquisition::advanceLED()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::advanceLED()\n";
#endif

  // Advance LED display when we get new data
  ledArray[ledIndex] = 0;
  ledIndex++;
  if (ledIndex==8) 
    ledIndex = 0;
  ledArray[ledIndex] = 1;
  evalBoard->setLedDisplay(ledArray);
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::advanceLED()\n";
#endif

}
void acquisition::turnOffLED()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::turnOffLED()\n";
#endif

  // Turn off LED.
  for (int i = 0; i < 8; ++i) 
    ledArray[i] = 0;
  ttlOut[15] = 0;
  evalBoard->setLedDisplay(ledArray);
  evalBoard->setTtlOut(ttlOut);
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::turnOffLED()\n";
#endif

}





// Reads numBlocks blocks of raw USB data stored in a queue of Rhd2000DataBlock
// objects, loads this data into this SignalProcessor object, scaling the raw
// data to generate waveforms with units of volts or microvolts.
//
// Returns number of bytes written to binary datastream out if saveToDisk == true.

int acquisition::move_to_dataBuffer()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::move_to_dataBuffer()\n";
#endif

  int block, channel, stream, i, j, sample;
  int indexAmp = 0;

  // copy from usb buffer ot localBuffer
  for (block = 0; block < numUsbBlocksToRead; ++block) 
    {
      for (sample = 0; sample < SAMPLES_PER_DATA_BLOCK; ++sample) 
	{
	  for (channel = 0; channel < 32; ++channel) 
	    {
	      for (stream = 0; stream < numStreams; ++stream) 
		{ 
		  //          (        sample no                  )*   total number channels  + channel no
		  localBuffer[(block*SAMPLES_PER_DATA_BLOCK+sample)* (32*numStreams) +   (channel*stream+channel) ] = 0 - (dataQueue.front().amplifierData[stream][channel][sample] - 32767);
		  // multiply by 0.195 to get microvolt
		  // if(channel<1)
		  // cout << channel << " " << dataQueue.front().amplifierData[stream][channel][sample] - 32768 << '\n';
		}
	    }
	  ++indexAmp;
        }
      // We are done with this Rhd2000DataBlock object; remove it from dataQueue
      numBlocksLoaded++;
      dataQueue.pop();
    }

  // move data to the mainWindow data buffer
  db->addNewData(numUsbBlocksToRead*SAMPLES_PER_DATA_BLOCK,localBuffer);

#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::move_to_dataBuffer()\n";
#endif

}



int acquisition::loadAmplifierData(queue<Rhd2000DataBlock> &dataQueue,
				  int numBlocks,
				  queue<Rhd2000DataBlock> &bufferQueue)
{

  // current data from usb are in dataQueue

  cerr << "entering acquisition::loadAmplifierData\n";

  int block, t, channel, stream, i, j;
  int indexAmp = 0;
  int indexDig = 0;
  int numWordsWritten = 0;
   
 
  for (block = 0; block < numBlocks; ++block) 
    {
      cerr << "block " << block << '\n';
      /*
      // Load and scale RHD2000 amplifier waveforms
      // (sampled at amplifier sampling rate)
      for (t = 0; t < SAMPLES_PER_DATA_BLOCK; ++t) 
	{
	  for (channel = 0; channel < 32; ++channel) 
	    {
	      for (stream = 0; stream < numDataStreams; ++stream) 
		{
		  // Amplifier waveform units = microvolts
		  amplifierPreFilter[stream][channel][indexAmp] = 0.195 *
		    (dataQueue.front().amplifierData[stream][channel][t] - 32768);
		}
	    }
	  ++indexAmp;
        }
      
      // Load and scale RHD2000 auxiliary input waveforms
      // (sampled at 1/4 amplifier sampling rate)
      for (t = 0; t < SAMPLES_PER_DATA_BLOCK; t += 4) 
	{
	  // Load USB interface board digital input and output waveforms
	  for (t = 0; t < SAMPLES_PER_DATA_BLOCK; ++t) 
	    {
	      for (channel = 0; channel < 16; ++channel) 
		{
		  boardDigIn[channel][indexDig] =
		    (dataQueue.front().ttlIn[t] & (1 << channel)) != 0;
		  boardDigOut[channel][indexDig] =
		    (dataQueue.front().ttlOut[t] & (1 << channel)) != 0;
		  
		}
	      ++indexDig;
	    }
	}
	  // We are done with this Rhd2000DataBlock object; remove it from dataQueue
        dataQueue.pop();
      */
    }
 
  cerr << "leaving acquisition::loadAmplifierData\n";
  return 0;
}


void acquisition::updateAuxDigOut()
{
#ifdef DEBUG_ACQ
  cerr << "entering acquisition::updateAuxDigOut()\n";
#endif

  evalBoard->enableExternalDigOut(Rhd2000EvalBoard::PortA, auxDigOutEnabled[0]);
  evalBoard->enableExternalDigOut(Rhd2000EvalBoard::PortB, auxDigOutEnabled[1]);
  evalBoard->enableExternalDigOut(Rhd2000EvalBoard::PortC, auxDigOutEnabled[2]);
  evalBoard->enableExternalDigOut(Rhd2000EvalBoard::PortD, auxDigOutEnabled[3]);
  evalBoard->setExternalDigOutChannel(Rhd2000EvalBoard::PortA, auxDigOutChannel[0]);
  evalBoard->setExternalDigOutChannel(Rhd2000EvalBoard::PortB, auxDigOutChannel[1]);
  evalBoard->setExternalDigOutChannel(Rhd2000EvalBoard::PortC, auxDigOutChannel[2]);
  evalBoard->setExternalDigOutChannel(Rhd2000EvalBoard::PortD, auxDigOutChannel[3]);
#ifdef DEBUG_ACQ
  cerr << "leaving acquisition::updateAuxDigOut()\n";
#endif

}
