#ifndef ACQUISITION_H
#define ACQUISITION_H

#define CHIP_ID_RHD2132  1
#define CHIP_ID_RHD2216  2
#define CHIP_ID_RHD2164  4
#define CHIP_ID_RHD2164_B  1000
#define REGISTER_59_MISO_A  53
#define REGISTER_59_MISO_B  58
#define SAMPLES_PER_DATA_BLOCK  60
#define MAX_BUFFER_LENGTH 2000000 // data_buffer 
#define ACQUISITION_SLEEP_TIME_MS 1 // if too long could lead to buffer overflow, we make it short to be up to data often

#include <string>
#include <queue>
#include "timeKeeper.h"
#include <pthread.h> // to be able to create threads

using namespace std;

class Rhd2000EvalBoard;
class Rhd2000DataBlock;
class acquisition
{
 public:
  acquisition();
  ~acquisition();

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
  queue<Rhd2000DataBlock> dataQueue;
  queue<Rhd2000DataBlock> bufferQueue;

  

  // variables for acquisition
  bool newDataReady;
  unsigned int dataBlockSize;
  unsigned int wordsInFifo;
  double fifoPercentageFull;
  double fifoCapacity;
  double samplePeriod;
  double latency;
 

  // variables to operate the acquisition buffer
  int numChips;
  int numAmplifierChannels;

   // variables inherited from kacq
  bool is_acquiring; // to send signal to the comedi thread
  int acquisition_thread_running; // set by the comedi thread when enter and exit
  pthread_t acquisition_thread;
  int acquisition_thread_id;
  int max_number_samples_in_buffer;
  unsigned long int number_samples_read; // total samples read
  int sample_no_to_add; // sample no of the newest sample in the buffer
  int samples_copied_at_end_of_buffer;
  int data_offset_to_dat; // to correct the data so that 0 is midpoint
  unsigned int* channel_list;
  int buffer_size;
  short int* buffer_data;// buffer to get data from comedi devices
  int data_points_move_back;
  int offset_move_back;
  struct timespec time_last_sample_acquired_timespec; 
  struct timespec inter_acquisition_sleep_timespec; // between read operations to driver
  double inter_acquisition_sleep_ms;
  struct timespec timespec_pause_restat_acquisition_thread; // allow acquisition to complete
  double pause_restart_acquisition_thread_ms;
  timeKeeper tk;


  // function to operate the intan evaluation board
  void openInterfaceBoard();
  void scanPorts();
  void findConnectedAmplifiers();
  void changeSampleRate(int sampleRateIndex);
  int deviceId(Rhd2000DataBlock *dataBlock, int stream, int &register59Value);
  bool start_acquisition();
  bool stop_acquisition();

  // thread functions
  void *acquisition_thread_function(void);
  static void *acquisition_thread_helper(void *context)
  {
    ((acquisition *)context)->acquisition_thread_function();
  }
  
  
  int loadAmplifierData(queue<Rhd2000DataBlock> &dataQueue,int numBlocks,queue<Rhd2000DataBlock> &bufferQueue);
  void runInterfaceBoard();
  void stopInterfaceBoard();


 private:
  

};



#endif // ACQUISITION_H
