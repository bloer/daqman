#include "ChannelModule.hh"
#include "ConvertData.hh"
#include <algorithm>
ChannelModule::ChannelModule(const std::string& name, 
			     const std::string& helptext) :
  BaseModule(name, helptext)
{
  AddDependency("ConvertData");
  RegisterParameter("skip_sum",_skip_sum = false,
		    "Skip processing the sum channel?");
  RegisterParameter("sum_only",_sum_only = false,
		    "Process only the sum channel and not others?");
}

ChannelModule::~ChannelModule()
{
}

int ChannelModule::Process(EventPtr event)
{
  int returnval = 0;
  _current_event = event;
  EventDataPtr data = event->GetEventData();
  for(size_t ch=0; ch < data->channels.size(); ch++){
    ChannelData* chdata = &(data->channels[ch]);
    // did we ask to skip this channel manually?
    if( _skip_channels.find( chdata->channel_id ) != _skip_channels.end())
      continue;
    if( _skip_sum && chdata->channel_id == ChannelData::CH_SUM)
      continue;
    if( _sum_only && chdata->channel_id != ChannelData::CH_SUM)
      continue;
    // Does this channel pass all cuts?
    if(CheckCuts(chdata)){
      returnval += Process(chdata);
    }
  }
  return returnval;
}

bool ChannelModule::CheckCuts(ChannelData* chdata)
{
  for(std::vector<ProcessingCut*>::iterator cutit = _cuts.begin();
      cutit != _cuts.end(); cutit++){
    if((*cutit) -> ProcessChannel(chdata) == false)
      return false;
  }
  return true;
}
