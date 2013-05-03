#include "WarpVeto.hh"
#include "Smoother.hh"
#include "SumChannels.hh"
#include <algorithm>

WarpVeto::WarpVeto() : 
  BaseModule(GetDefaultName(), "Simple online analysis for the WARP veto DAQ")
{
  RegisterParameter("max_channels",max_channels = 40, 
		    "Maximum number of channels to store");
  AddDependency<Smoother>();
  AddDependency<SumChannels>();
}

WarpVeto::~WarpVeto() {}

int WarpVeto::Initialize()
{
  return 0;
}

int WarpVeto::Finalize()
{
  return 0;
}

int WarpVeto::Process(EventPtr event)
{
  int nchans = event->GetEventData()->channels.size();
  ChannelData& sum = event->GetEventData()->channels.back();
  if(sum.channel_id != ChannelData::CH_SUM){
    Message(ERROR)<<"The last channel in event "
		  <<event->GetEventData()->event_id<<" has id "<<sum.channel_id
		  <<"; expected sum with id "<<ChannelData::CH_SUM<<"\n";
    return 1;
  }
  uint32_t pulsesize = sum.channel_end - sum.channel_start;
  int bufsize = 2*sizeof(uint32_t) + max_channels * sizeof(double) + pulsesize;
  _buffer.resize(bufsize);
  
  char* rawbuffer = &(_buffer[0]);
  
  ((uint32_t*)rawbuffer)[0] = max_channels;
  ((uint32_t*)rawbuffer)[1] = pulsesize;
  double* ch_amp = (double*)( rawbuffer + 2*sizeof(uint32_t));
  char* sum_pulse = rawbuffer + 2*sizeof(uint32_t) +max_channels*sizeof(double);

  std::fill_n(ch_amp, max_channels, 0);
  for(int i=0; i<nchans; i++){
    ChannelData& chdata = event->GetEventData()->channels[i];
    int id = chdata.channel_id;
    if(id >=0 && id < (int)max_channels)
      ch_amp[id] = chdata.smoothed_max - chdata.smoothed_min;
    else if(id >= 0){
      Message(WARNING)<<"Channel with ID "<<id<<" present in event "
		      <<event->GetEventData()->event_id
		      <<"; expected max_channels is "<<max_channels<<"\n";
    }
  }
  std::copy( sum.channel_start, sum.channel_end, sum_pulse);
  return 0;
}
