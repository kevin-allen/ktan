#ifndef CHANNELGROUP_H
#define CHANNELGROUP_H
#define MAX_CHANNELS_PER_GROUP 16
#include <string>
#include "timeKeeper.h"
#include "dataBuffer.h"

using namespace std;

/***************************************************************
Class to take care of the channelGroup in it own thread
***************************************************************/ 
class channelGroup
{
 public:
  channelGroup();
  ~channelGroup();
  void set_channel_list(int numChannels, int* channelList);
  void set_channel_id(int index, int id);
  void set_num_channels(int nc);
  int get_num_channels();
  int get_max_num_channels();
  int get_channel_id(int index);


 private:
  int num_channels;
  int max_num_channels;
  int * channel_list;    
};



#endif
