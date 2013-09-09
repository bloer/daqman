#include "ConvertData.hh"
#include "V172X_Event.hh"
#include "V172X_Params.hh"
#include "RootWriter.hh"
#include "EventHandler.hh"
#include "RunDB.hh"
#include "ConfigHandler.hh"
#include <vector>


std::istream& ConvertData::ChOffsetLoader::operator()(std::istream& in)
{
  int ch;
  double offset;
  in >> ch >> offset;
  _parent->SetChOffset(ch,offset);
  return in;
}

std::ostream& ConvertData::ChOffsetLoader::operator()(std::ostream& out)
{
  std::map<int,double>* offsets = _parent->GetChOffsetMap();
  std::map<int,double>::iterator it = offsets->begin();
  for( ; it != offsets->end(); it++){
    out<<GetFuncName()<<" "<<it->first<<" "<<it->second<<std::endl;
  }
  return out;
}
  
ConvertData::ConvertData():
  BaseModule(GetDefaultName(), 
	     "Convert the binary data to useable vectors and timestamps")
{
  RegisterFunction(ChOffsetLoader::GetFuncName(), ChOffsetLoader(this),
		   ChOffsetLoader(this));
  _v172X_params = 0;
  _headers_only = false;
}

ConvertData::~ConvertData()
{
  
}

int ConvertData::Initialize()
{
  start_time = 0;
  previous_event_time = 0;
  _id_mismatches = 0;
  ConfigHandler* config = ConfigHandler::GetInstance();

  //initialize stuff for decoding V172X events
  if(!_v172X_params) 
    _v172X_params = dynamic_cast<V172X_Params*>
      (config->GetParameter(V172X_Params().GetDefaultKey()));
    //if that didn't work, we need to load it
  bool err = 0;
  if(config->GetSavedCfgFile() != "")
    err = config->LoadCreateParameterList(_v172X_params);
  if(_v172X_params){
    Message(DEBUG)<<"Number of V172X boards in this run: "
		  <<_v172X_params->GetEnabledBoards()<<"\n";
    //make sure all the per-board params are set
    for(int i=0; i<_v172X_params->GetEnabledBoards(); i++){
      V172X_BoardParams& board = _v172X_params->board[i];
      board.UpdateBoardSpecificVariables();
    }
    //make sure some intermediate values are set
    _v172X_params->GetEventSize();
  }
  
  
  if(!_v172X_params){
     Message(ERROR)<<"Unable to load saved configuration information!\n";
     return 1;
  }
  
  _info = EventHandler::GetInstance()->GetRunInfo();
  //Load campaign information
  _cpinfo = EventHandler::GetInstance()->GetCampaignInfo();
  
  return 0;
}

int ConvertData::Finalize()
{   
  //reset the parameter lists
  _v172X_params = 0;
  if(_id_mismatches){
    Message(WARNING)<<_id_mismatches<<" events had mismatched IDs!\n";
  }
  return 0;
}

const uint64_t ns_per_s = 1000000000;
int ConvertData::Process(EventPtr event)
{
  RawEventPtr raw = event->GetRawEvent();
  EventDataPtr data = event->GetEventData();
  
  //get basic info from the headers
  data->event_id = raw->GetID();
  data->timestamp = raw->GetTimestamp();
  if(start_time == 0){
    //this is the first event
    start_time = data->timestamp;
  }
  //set the most basic first, other decoders can override
  data->event_time = 1000000000*(data->timestamp-start_time);
  //get the real data from datablocks
  data->channels.clear();
  for(size_t blocknum=0; blocknum<raw->GetNumDataBlocks(); blocknum++){
    switch(raw->GetDataBlockType(blocknum)){
    case RawEvent::CAEN_V172X :
      DecodeV172XData(raw->GetRawDataBlock(blocknum), 
		      raw->GetDataBlockSize(blocknum), 
		      data);
      break;
    case RawEvent::MONTECARLO :
    default :
      Message(ERROR)<<"Decoding has not yet been implemented for block type "
		    <<raw->GetDataBlockType(blocknum)<<"\n";
      continue;
    }
  }
  
  if(!_headers_only){
    data->nchans = data->channels.size();
    //find the max and min of each channel
    for(int ch=0; ch<data->nchans; ch++)
      {
	ChannelData& chdata = data->channels[ch];
	double* wave = chdata.GetWaveform();
	double* max_samp = std::max_element(wave, wave+chdata.nsamps);
	double* min_samp = std::min_element(wave, wave+chdata.nsamps);
	//data is saturated if it hit 0 or maximum range
	chdata.saturated = (*min_samp == 0 || 
			    *max_samp == chdata.GetVerticalRange());
	if(chdata.saturated) data->saturated = true;
	chdata.maximum = *max_samp;
	chdata.minimum = *min_samp;
	chdata.max_time = chdata.SampleToTime(max_samp - wave);
	chdata.min_time = chdata.SampleToTime(min_samp - wave);
	//find the single photoelectron peak for this channel
	RunDB::runinfo::channelinfo* chinfo = 
	  _info->GetChannelInfo(chdata.channel_id);
	if(chinfo)
	  chdata.spe_mean = chinfo->spe_mean;
	else
	  {
	    chdata.spe_mean = 1;
	    bool fail = EventHandler::GetInstance()->GetFailOnBadCal();
	    if(fail){
	      Message(ERROR)<<"No calibration info for channel "
			    <<chdata.channel_id<<" in event "
			    <<data->event_id<<" in run "<<data->run_id<<"\n";
	      return -1;
	    }
	  }

	// get the PMT information for this channel
	chdata.pmt.Load(_cpinfo->pmts[chdata.channel_id]);
      }
    data->nchans = data->channels.size();
  }// end skipped section if headers only

  data->dt = ( previous_event_time > 0 ? 
	       data->event_time - previous_event_time : 0 );
  previous_event_time = data->event_time;
  
  //since we don't know the last event, set info for each event
  _info->starttime = start_time;
  _info->endtime = data->timestamp;
  _info->events = data->event_id+1;
  
  if(_info->trigger_veto > 0){
    _info->livetime = 1.*data->event_time/ns_per_s - 
      _info->trigger_veto/1000. * (1.*_info->events);
    //should  the acquisition window be "live"????
  }
  else if(data->trigger_count)
    _info->livetime = 1.*_info->events / data->trigger_count * 
      data->event_time / ns_per_s;
  if(data->nchans > 0 && 
     (_info->pre_trigger_time_us < 0 || _info->post_trigger_time_us < 0) ){
    //WARNING: this assumes all channels have same acquisition times!!!!!
    ChannelData& ch = data->channels[0];
    _info->pre_trigger_time_us = -ch.SampleToTime(0);
    _info->post_trigger_time_us = ch.SampleToTime(ch.nsamps);
    ///@todo: should that be ch.nsamps-1?
  }
  return 0;
}

int ConvertData::DecodeV172XData(const unsigned char* rawdata, 
				  uint32_t datasize, 
				  EventDataPtr data)
{
  V172XEventPtr v172X(new V172X_Event(rawdata,
				      datasize, 
				      _v172X_params) );
  const V172X_Params* params = v172X->GetParameters(); 
  int n_boards = v172X->GetNBoards();
  //estimate the number of channels and reserve size in the vector
  int reserve_size = (params->enabled_channels > 0 ? params->enabled_channels :
		      n_boards * 10 );
  data->channels.reserve( reserve_size + 5);
  bool id_mismatch=false;
  for(int i=0; i<n_boards; i++){
    const V172X_BoardData& board_data = v172X->GetBoard(i);
    const V172X_BoardParams& board_params = 
	params->board[board_data.board_id];
    //check for ID mismatch
    if(i==0)
      data->trigger_count = board_data.event_counter;
    else if(board_data.event_counter != (uint32_t)data->trigger_count){
      id_mismatch = true;
    }
    							 
    uint64_t s_since_start = data->timestamp - start_time;
    //if(s_since_start > 1) s_since_start--;
    if(board_data.timestamp * board_params.ns_per_clocktick <
       board_params.GetTimestampRange()/2)
      s_since_start+=board_params.GetTimestampRange()/ns_per_s/4;
    else
      s_since_start-=board_params.GetTimestampRange()/ns_per_s/4;
    uint64_t n_resets = (s_since_start * ns_per_s)/ 
      ( board_params.GetTimestampRange());
    /*uint64_t n_resets2 = ((s_since_start+2)*ns_per_s)/
      ( board_params.GetTimestampRange());
    if( n_resets2 > n_resets && 
	(board_params.ns_per_clocktick*board_data.timestamp)<2*ns_per_s  
	&& s_since_start > 1 )
	n_resets++;*/
    data->event_time = board_params.GetTimestampRange() * n_resets + 
      board_params.ns_per_clocktick * board_data.timestamp;
    for(int j=0; j<board_data.nchans; j++){
      if(board_data.channel_start[j] == NULL)
	continue;
      int channel_id = board_data.nchans * board_data.board_id + j;
      if( _skip_channels.find(channel_id) != _skip_channels.end())
	continue;
      const V172X_ChannelParams& ch_params = board_params.channel[j];
      data->channels.push_back(ChannelData());
      ChannelData& chdata = data->channels.back();
      chdata.board_id = board_data.board_id;
      chdata.board_num = i;
      chdata.channel_num = j;
      chdata.channel_id = channel_id;
      chdata.label = ch_params.label;
      chdata.timestamp = board_data.timestamp * 10; //Fix this!
      chdata.sample_bits = board_params.sample_bits;
      chdata.sample_rate = board_params.GetSampleRate();
      chdata.trigger_index = board_params.GetTriggerIndex() - 
	(int)(GetChOffset(channel_id) * chdata.sample_rate);
      
      //copy over the actual waveform information
      if(_headers_only) continue;
      chdata.channel_start = (char*)(board_data.channel_start[j]);
      chdata.channel_end = (char*)(board_data.channel_end[j]);
      if(board_params.zs_type != ZLE){
	// copy the data as a double 
	if(chdata.sample_bits < 9)
	  chdata.waveform.assign((uint8_t*)chdata.channel_start,
				 (uint8_t*)chdata.channel_end);
        else if (chdata.sample_bits == 10) {
          chdata.waveform.clear ();
          for (uint32_t* ptr = (uint32_t*)chdata.channel_start; 
	       ptr < (uint32_t*)chdata.channel_end; ptr ++) {
            uint32_t raw_v = *ptr;
	    const int ns = (raw_v>>30)&3;
            for (int i = 0; i < ns; i++) {
              uint16_t sample = raw_v & 0x3ff;
              raw_v = raw_v >> 10;
              chdata.waveform.push_back (sample);
	    }
	  }
	}
	else if(chdata.sample_bits < 17)
	  chdata.waveform.assign((uint16_t*)chdata.channel_start,
				 (uint16_t*)chdata.channel_end);
	else
	  chdata.waveform.assign((uint32_t*)chdata.channel_start,
				 (uint32_t*)chdata.channel_end);
      }
      else{
	//we need to evaluate the zero skipped data
	//first word is the size of this channel's data
	uint32_t nwords = *((uint32_t*)(chdata.channel_start));
	chdata.nsamps = 0;
	uint32_t offset = 1;
	std::vector<const char*> data_blocks;
	while(offset < nwords){
	  //get the control word
	  uint32_t control = *( ((uint32_t*)(chdata.channel_start)) + offset);
	  uint32_t subwords = control & 0x1FFFFF;
	  uint32_t subsamps = subwords * board_params.bytes_per_sample;
	  bool good = control & 0x80000000;
	  
	  if(good){
	    data_blocks.push_back( chdata.channel_start + 4*(offset+1) );
	    std::pair<int,int> region(chdata.nsamps,
				      chdata.nsamps + subsamps);
	    chdata.unsuppressed_regions.push_back(region);
	    offset += subwords;
	  }
	  
	  chdata.nsamps += subsamps;
	  offset += 1;
	}
	//assign all the value of 1 by default so it doesn't show as saturated
	chdata.waveform.assign(chdata.nsamps, 1);
	std::vector<double>& wave = chdata.waveform;
	//now try to fill all the valid samples
	for(size_t block=0; block < data_blocks.size(); block++){
	  int subsamps = chdata.unsuppressed_regions[block].second - 
	      chdata.unsuppressed_regions[block].first;
	  if(chdata.sample_bits < 9){
	    std::copy( (uint8_t*)data_blocks[block],
		       ((uint8_t*)data_blocks[block]) + subsamps,
		       &(wave[chdata.unsuppressed_regions[block].first]) );
	  }
	  else if (chdata.sample_bits < 17){
	    std::copy( (uint16_t*)data_blocks[block],
		       ((uint16_t*)data_blocks[block]) + subsamps,
		       &(wave[chdata.unsuppressed_regions[block].first]) );
	  }
	  else {
	    std::copy( (uint32_t*)data_blocks[block],
		       ((uint32_t*)data_blocks[block]) + subsamps,
		       &(wave[chdata.unsuppressed_regions[block].first]) );
	  }
	}
	//finally set the baseline to the nearest sample
	for(size_t block=0; block < data_blocks.size(); block++){
	  std::pair<int,int> region = chdata.unsuppressed_regions[block];
	  int fill_start = (block > 0 ? 
			    chdata.unsuppressed_regions[block-1].second : 0 );
	  int fill_end = region.first;
	  if(fill_end > fill_start)
	    std::fill( &(wave[fill_start]), &(wave[fill_end]), wave[fill_end]);
	}
	//check the last region
	if( !chdata.unsuppressed_regions.empty() && 
	    chdata.unsuppressed_regions.back().second < chdata.nsamps){
	  std::fill( &(wave[chdata.unsuppressed_regions.back().second]),
		     &(wave[chdata.nsamps]), 
		     wave[chdata.unsuppressed_regions.back().second-1] );
	}
      }// end check for zero suppressed data
      chdata.nsamps = chdata.waveform.size();
    }
  }
  if(id_mismatch){
    if(_id_mismatches==0){
      Message(WARNING)<<"Event ID mismatch found on event "
		      <<data->event_id<<"!\n";
      Message(WARNING)<<"Further mismatches will be silent.\n";
    }
    
    data->status |= EventData::ID_MISMATCH;
    _id_mismatches++;
  }	
  
  return 0;
}

