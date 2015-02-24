#define DEBUG_REC
#include "recording.h"
#include <stdlib.h> 
#include <stdint.h>
#include <iostream>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
recording::recording(dataBuffer* datab)
{
#ifdef DEBUG_REC
  cerr << "entering recording::recording()\n";
#endif

  db=datab;
  // Allocate memory for an internal buffer that can be used by other objects 
  buffer_size= MAX_REC_BUFFER_LENGTH;
  buffer = new short int [buffer_size];
  
  // set the home directory as the default directory
  struct passwd *p;
  char *username=getenv("USER");
  p=getpwnam(username);
  directory=strcat(p->pw_dir,"/");
  file_name=strcat(directory,"out.dat");

  proportion_buffer_filled_before_save=FILLING_PROPORTION_BEFORE_SAVE;
    
  number_channels_save=DEFAULT_NUM_CHANNELS_SAVE;
  max_samples_in_buffer=buffer_size/number_channels_save;
  new_samples_in_buffer=0;
  file_size=0;

  inter_recording_sleep_ms=40;
  inter_recording_sleep_timespec=tk.set_timespec_from_ms(inter_recording_sleep_ms);
  number_samples_saved=0;
  for (int i=0;i<number_channels_save;i++)
    channel_list[i]=i;
  
  is_recording=false;

#ifdef DEBUG_REC
  cerr << "file_name: " << file_name << '\n';
  cerr << "leaving recording::recording()\n";
#endif
}

recording::~recording()
{
#ifdef DEBUG_REC
  cerr << "entering recording::~recording()\n";
#endif

  delete[] buffer;
#ifdef DEBUG_REC
  cerr << "leaving recording::~recording()\n";
#endif
}



bool recording::set_recording_channels(int numChannels, unsigned int* channelList)
{
#ifdef DEBUG_REC
  cerr << "entering recording::set_recording_channels()\n";
#endif
  if(numChannels<0||numChannels>RECORDING_MAXIMUM_CHANNELS)
    {
      cerr << "recording::set_recording_channels(), numChannel is out of range: " << numChannels << "\n";
      return false;
    }
  number_channels_save=numChannels;
  for(int i = 0; i < number_channels_save;i++)
    channel_list[i]=channelList[i];
#ifdef DEBUG_REC
  cerr << "leaving recording::set_recording_channels()\n";
#endif

}
bool recording::start_recording()
{
#ifdef DEBUG_REC
  cerr << "entering recording::start_recording()\n";
#endif
  
  if(is_recording==true)
    {
      cerr << "recording::start_recording(), is_recording already true\n";
      return false;
    }
  number_samples_saved=0;
  new_samples_in_buffer=0;
  is_recording=true;
  
  if(open_file()==false)
    {
      cerr << "recording::start_recording(), problem opening file\n";
      is_recording==false;
      return false;
    }


#ifdef DEBUG_REC
  cerr << "leaving recording::start_recording()\n";
#endif

}

bool recording::stop_recording()
{
#ifdef DEBUG_REC
  cerr << "entering recording::stop_recording()\n";
#endif
  if(is_recording==false)
    {
      cerr << "recording::stop_recording(), is_recording already false\n";
    }


  is_recording=false;
  nanosleep(&inter_recording_sleep_timespec,&req);
  nanosleep(&inter_recording_sleep_timespec,&req);
  if(close_file()==false)
    {
      cerr << "recording::stop_recording(), problem closing the file\n";
    }

#ifdef DEBUG_REC
  cerr << "leaving recording::stop_recording()\n";
#endif
}


void *recording::recording_thread_function(void)
{
#ifdef DEBUG_REC
  cerr << "entering recording::recording_thread_function()\n";
#endif
  while(is_recording==true)
    {
      
      // get new data from db
      rec_buffer_ptr=buffer+new_samples_in_buffer*number_channels_save;
      max_samples_to_get=max_samples_in_buffer-new_samples_in_buffer;
      ret_get_samples=db->getNewData(number_samples_saved+new_samples_in_buffer,rec_buffer_ptr,max_samples_to_get,number_channels_save,channel_list);
      if(ret_get_samples==-1)
	{
	  cerr << "recording::recording_thread_function(), problem with getNewData()\n";
	  return 0;
	}
      new_samples_in_buffer=new_samples_in_buffer+ret_get_samples; // add the new samples loaded
      
      if(new_samples_in_buffer>max_samples_in_buffer*proportion_buffer_filled_before_save)
	{
	  save_buffer_to_file();
	  // save buffer to file
	}

      // take a break here instead of looping 100% of PCU
      nanosleep(&inter_recording_sleep_timespec,&req);
    }

#ifdef DEBUG_REC
  cerr << "leaving recording::recording_thread_function()\n";
#endif
}

bool recording::save_buffer_to_file()
{

#ifdef DEBUG_REC
  cerr << "entering recording::save_buffer_to_file()\n";
#endif
  int num_elements;
  num_elements=new_samples_in_buffer*number_channels_save;
  

  if (num_elements<=(int)(buffer_size*proportion_buffer_filled_before_save))
    {
      return 0;
    }
  if(fwrite(buffer,sizeof(short int),num_elements,file)!=num_elements)
    {
      cerr << "recording::save_buffer_to_file(), problem saving data in " << file_name << '\n';
      cerr << "recording will stop here\n";
      is_recording=0;
      return -1;
    }
  number_samples_saved+=new_samples_in_buffer; // update count
  new_samples_in_buffer=0;

#ifdef DEBUG_REC
  cerr << "save_buffer_to_file(), num_elements: " << num_elements << "\n";
#endif


#ifdef DEBUG_REC
  cerr << "leaving recording::save_buffer_to_file()\n";
#endif
  
  return true;
}
bool recording::open_file()
{
#ifdef DEBUG_REC
  cerr << "entering recording::open_file(), file_name: " << file_name << "\n";
#endif
  file=fopen(file_name,"w");
  if (file==NULL)
    {
      cerr << "recording::open_file() error opening " << file_name << '\n';
      return -1;
    }
#ifdef DEBUG_REC
  cerr << "leaving recording::open_file()\n";
#endif

  return true;
}

bool recording::close_file()
{
#ifdef DEBUG_REC
  cerr << "entering recording::close_file()\n";
#endif
  fclose(file);
#ifdef DEBUG_REC
  cerr << "leaving recording::close_file()\n";
#endif
  
  return true;

}
