#define DEBUG_GRP
#include "channelGroup.h"
#include <stdlib.h> 
#include <stdint.h>
#include <iostream>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <gtkmm.h>
channelGroup::channelGroup()
{
#ifdef DEBUG_GRP
  cerr << "entering channelGroup::channelGroup()\n";
#endif
  num_channels=0;
  max_num_channels=MAX_CHANNELS_PER_GROUP;
  channel_list = new int[max_num_channels];
  blue = new double[max_num_channels];
  red = new double[max_num_channels];
  green = new double[max_num_channels];
#ifdef DEBUG_GRP
  cerr << "leaving channelGroup::channelGroup()\n";
#endif
}
channelGroup::~channelGroup()
{
#ifdef DEBUG_GRP
  cerr << "entering channelGroup::~channelGroup()\n";
#endif
  delete[] channel_list;
  delete[] blue;
  delete[] red;
  delete[] green;
#ifdef DEBUG_GRP
  cerr << "entering channelGroup::~channelGroup()\n";
#endif
}
void channelGroup::set_channel_list(int numChannels, int* channelList)
{
  if(numChannels<0)
    {
      cerr << "channelGroup::set_channel_list(), numChannels < 0: " << numChannels << '\n';
      return;
    }
  if(numChannels>max_num_channels)
    {
      cerr << "channelGroup::set_channel_list(), numChannels > max_num_channels: " << numChannels << '\n';
      return;
    }
  num_channels=numChannels;
  for(int i = 0; i < num_channels;i++)
    channel_list[i]=channelList[i];
}
void channelGroup::set_num_channels(int nc)
{
  if(nc<0)
    {
      cerr << "channelGroup::set_num_channels(), nc < 0: " << nc << '\n';
      return;
    }
  if(nc>max_num_channels)
    {
      cerr << "channelGroup::set_num_channels(), nc > max_num_channels: " << nc << '\n';
      return;
    }
  num_channels=nc;
}
void channelGroup::set_channel_id(int index, int id)
{
  if(index<0)
    {
      cerr << "channelGroup::set_channel_id(), index < 0: " << index << '\n';
      return;
    }
  if(index>max_num_channels)
    {
      cerr << "channelGroup::set_channel_id(), index > max_num_channles: " << index << '\n';
      return;
    }
  channel_list[index]=id;
}


int channelGroup::get_num_channels()
{
  return num_channels;
}
int channelGroup::get_max_num_channels()
{
  return max_num_channels;
}

int channelGroup::get_channel_id(int index)
{
 if(index<0)
    {
      cerr << "channelGroup::get_channel_id(), index < 0: " << index << '\n';
      return -1;
    }
  if(index>max_num_channels)
    {
      cerr << "channelGroup::get_channel_id(), index > max_num_channles: " << index << '\n';
      return -1;
    }

  return channel_list[index];
}
void channelGroup::set_colours(int index, double b, double r, double g)
{
  if(index<0)
    {
      cerr << "channelGroup::set_colours, index is out of range: " << index << "\n";
      return;
    }
  if(index>num_channels)
    {
      cerr << "channelGroup::set_colours, index is out of range: " << num_channels << "\n";
      return;
    }

  if(b<0||b>1)
    {
      cerr << "channelGroup::set_colours, b is out of range: " << b << "\n";
      return;
    }
  if(r<0||r>1)
    {
      cerr << "channelGroup::set_colours, b is out of range: " << r << "\n";
      return;
    }
  if(g<0||g>1)
    {
      cerr << "channelGroup::set_colours, b is out of range: " << g << "\n";
      return;
    }
  blue[index]=b;
  red[index]=r;
  green[index]=g;
}

void channelGroup::copy_channelGroup(channelGroup &grp)
{
  grp.set_num_channels(num_channels);
  for(int i = 0;i < num_channels;i++)
    {
      grp.set_channel_id(i,channel_list[i]);
    }
}
