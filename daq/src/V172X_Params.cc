#include "V172X_Params.hh"
#include "Message.hh"
#include <exception>
#include <stdexcept>
#include <math.h>

//declare some useful constants
//const double sample_rate =  100.;
//const double V_full_scale = 2.25;
//const int sample_bits = 14;
//const int bytes_per_sample = 2;

const uint32_t max_14bit = 0x3FFF;
const uint32_t acq_control_set = 0x28;
const uint32_t channels_config_set = 0x10;
//const uint32_t max_samples = 524288;
const int event_size_padding = 8;
const int Mbyte = 1024*1024;

const uint16_t halfmax14 = 0x1FFF;
const uint16_t halfmax16 = 0x7FFF;
const uint32_t halfmax32 = 0x7FFFFFFF;
using namespace std;

V172X_ChannelParams::V172X_ChannelParams() : 
  ParameterList("V172X_ChannelParams", 
		"Information about a single channel in a CAEN V172X digitizer")
{
  RegisterParameter("enabled", enabled=true, 
		    "Is this channel active in the current run?");
  RegisterParameter("enable_trigger_source", enable_trigger_source=false,
		    "Does this channel generat local trigger signals?");
  RegisterParameter("enable_trigger_out", enable_trigger_out=false,
		    "Does this channel generate trigger out signals?");
  RegisterParameter("threshold", threshold = halfmax14,
		    "The threshold (in counts) for the signal to trigger on");
  RegisterParameter("thresh_time_us", thresh_time_us = 0,
		    "Time that the signal must be above/below threshold in order to generate a trigger");
  RegisterParameter("dc_offset", dc_offset = halfmax16,
		    "DC offset (16 bit range) applied to the signal before digitizing");
  RegisterParameter("zs_polarity", zs_polarity = TP_FALLING, "Do we suppress the signal if it is below or above the zs_threshold?");
  RegisterParameter("zs_threshold",zs_threshold = halfmax14, "Threshold the signal must cross before it is considered to be non-zero");
  RegisterParameter("zs_thresh_time_us",zs_thresh_time_us = 0,"Time a signal must be above the zs_threshold before it counts");
  RegisterParameter("zs_pre_samps", zs_pre_samps = 10, 
		    "Number of samples to save before a ZLE removed block");
  RegisterParameter("zs_post_samps", zs_post_samps = 10, 
		    "Number of samples to save after a ZLE removed block");
  
  RegisterParameter("label",label="x", 
		    "A descriptive label for the channel");
}

V172X_BoardParams::V172X_BoardParams() : 
  ParameterList("V172X_BoardParams",
		"Information about a single CAEN V172X Digitizer")
{
  RegisterParameter("enabled", enabled = false,
		    "Is this board active in this run?");
  RegisterParameter("address", address = 0xFFFF0000,
		    "Address of the board on the VME crate (set in hardware)");
  RegisterParameter("link", link = 0,
		    "Board optical link number");
  RegisterParameter("usb", usb = false,
		    "Connect through USB instead of optical link?");
  RegisterParameter("chainindex", chainindex=0, 
		    "Order of board on a fiber daisy chain connection");
  RegisterParameter("id", id = -1, "Software id of the board");
  RegisterParameter("board_type", board_type = OTHER,
		    "Specific model of digitizer (V1720, V1724, V1721)");
  RegisterParameter("nchans", nchans = MAXCHANS, 
		    "actual number of channels on this unit");
  RegisterParameter("v_full_scale", v_full_scale = 2,
		    "Full scale range in voltage of the digitizer input");
  RegisterParameter("stupid_size_factor", stupid_size_factor = 1);
  RegisterParameter("sample_bits", sample_bits = 12,
		    "Sampling depth of the ADC conversion");
  RegisterParameter("max_sample_rate", max_sample_rate = 250.,
		    "Maximum number of samples recorded per microsecond");
  RegisterParameter("mem_size", mem_size = 1,
		    "Maximum size of onboard buffer, in MSamples/channel");
  RegisterParameter("bytes_per_sample", bytes_per_sample = 2,
		    "Number of aligned bytes needed for a single sample");
  RegisterParameter("event_size_bytes", event_size_bytes = 0,
		    "Expected size of a single event in bytes");
  RegisterParameter("ns_per_clocktick", ns_per_clocktick = 8,
		    "Time between each tick of the onboard trigger clock");
  RegisterParameter("enable_software_trigger", enable_software_trigger=true,
		    "Can this board receive triggers from the VME plane (from the computer)?");
  RegisterParameter("enable_software_trigger_out",
		    enable_software_trigger_out = true,
		    "Does this board repeat VME triggers to the trigger-out signal?");
  RegisterParameter("enable_external_trigger", enable_external_trigger=true,
		    "Does this board trigger on the trigger-in signal?");
  RegisterParameter("enable_external_trigger_out",
		    enable_external_trigger_out = true,
		    "Does this board repeat the trigger-in to trigger-out?");
  RegisterParameter("local_trigger_coincidence",local_trigger_coincidence = 0,
		    "Number of channels after the first necessary to generate a trigger");
  RegisterParameter("coincidence_window_ticks", coincidence_window_ticks=0,
		    "Width of coinidence window in clockticks (usually 8ns). Note this will delay the trigger time!");
  RegisterParameter("trigout_coincidence",trigout_coincidence = 0,
		    "Majority level for trigger out formation");
  RegisterParameter("pre_trigger_time_us", pre_trigger_time_us = 1,
		    "Length of buffer time to to store before the trigger");
  RegisterParameter("post_trigger_time_us", post_trigger_time_us = 30,
		    "Length of time after the trigger to store");
  RegisterParameter("downsample_factor", downsample_factor = 1,
		    "No longer implemented in CAEN firmware; kept for compatibility only!");
  RegisterParameter("almostfull_reserve",almostfull_reserve = 1,
		    "Assert BUSY when free buffers <= n");
  RegisterParameter("trigger_polarity", trigger_polarity = TP_FALLING,
		    "Determines whether to generate trigger when the signal is above or below the trigger threshold");
  RegisterParameter("count_all_triggers", count_all_triggers = true,
		    "Do we increment the trigger counter when overlapping triggers come in, or the buffer is full?");
  RegisterParameter("zs_type", zs_type = NONE,
		    "Which type of zero suppression to use (should be NONE)");
  RegisterParameter("enable_trigger_overlap", enable_trigger_overlap = false,
		    "Do we generate partial triggers if two come in too close to each other?");
  RegisterParameter("signal_logic", signal_logic = NIM,
		    "Type of logic (NIM or TTL) for external signals");
  RegisterParameter("trgout_mode", trgout_mode = TRIGGER,
		    "Which signal to provide on the front panel TRGOUT");
  RegisterParameter("enable_test_pattern", enable_test_pattern = false,
		    "Generate a triangle wave on the channels instead of measure real signals?");
  //RegisterParameter("acq_control_val", 
  //acq_control_val =  ( 1<<4 * (downsample_factor > 1) + 
  //1<<3 * count_all_triggers ) ); 
  for(int i=0; i<MAXCHANS; i++){
    char key[20];
    sprintf(key,"channel%d",i);
    RegisterParameter(key,channel[i]);
  }  
}

V172X_Params::V172X_Params() : ParameterList("V172X_Params")
{
  RegisterParameter("align64", align64 = true,
		    "Do we include an extra empty sample when using 64-bit size words?");
  RegisterParameter("trigger_timeout_ms", trigger_timeout_ms = 3000,
		    "Time to wait for a trigger before timing out");
  RegisterParameter("max_mem_size", max_mem_size  = 209715200,
		    "Maximum amount of memory we're allowed to use for the raw event buffer");
  RegisterParameter("no_low_mem_warn",no_low_mem_warn = false,
		    "Should we suppress the warning generated when the raw event buffer is full?");
  RegisterParameter("send_start_pulse",send_start_pulse = false,
		    "Do we tell the digitizers to wait to start the event until a synchornize pulse is sent (true), or start immediately (false)");
  RegisterParameter("auto_trigger", auto_trigger = false,
		    "Do we automatically generate a trigger if timeout occurrs?");
  RegisterParameter("vme_bridge_link", vme_bridge_link = 0,
		    "VME bridge optical link number");
  //RegisterParameter("event_size_bytes", event_size_bytes = 0);
  //RegisterParameter("enabled_boards", enabled_boards = 0);
  //RegisterParameter("enabled_channels", enabled_channels = 0);
  for(int i=0; i<nboards; i++){
    char key[20];
    sprintf(key,"board%d",i);
    RegisterParameter(key,board[i]);
    board[i].id = i;
    for(int j=0; j<board[i].MAXCHANS; j++){
      sprintf(key,"b%dch%d",i,j);
      board[i].channel[j].label = key;
    }
  }
}
  
int V172X_BoardParams::UpdateBoardSpecificVariables()
{
  switch(board_type){
  case V1724:
    max_sample_rate = 100.;
    sample_bits = 14;
    bytes_per_sample = 2;
    v_full_scale = 2.25;
    stupid_size_factor = 1;
    ns_per_clocktick = 10;
    break;
  case V1720:
    max_sample_rate = 250;
    sample_bits = 12;
    bytes_per_sample = 2;
    v_full_scale = 2;
    stupid_size_factor = 2;
    ns_per_clocktick = 8;
    break;
  case V1721:
     max_sample_rate = 1000;
    sample_bits = 8;
    bytes_per_sample = 1;
    v_full_scale = 2;
    stupid_size_factor = 1;
    ns_per_clocktick = 8;
    break;
  case V1751:
    max_sample_rate = 1000;
    sample_bits = 10;
    bytes_per_sample = 2;
    v_full_scale = 1;
    stupid_size_factor = 7;
    ns_per_clocktick = 4;
    break;
  case V1730:
    max_sample_rate = 500;
    sample_bits = 14;
    bytes_per_sample = 2;
    v_full_scale = 2;
    stupid_size_factor = 4;
    ns_per_clocktick = 8;
    break;
  default:
    return -1;
    
  }
  return 0;
}

//Utility functions
//returns sample rate in samples per microsecond
double V172X_BoardParams::GetSampleRate(bool downsamp) const
{
  //if(downsample_factor > 10) downsample_factor = 10;
  return downsamp ? max_sample_rate/downsample_factor : max_sample_rate;
}

uint32_t V172X_BoardParams::GetPostNSamps(bool downsamp) const
{
  return (uint32_t)(post_trigger_time_us * GetSampleRate(downsamp));
}

uint32_t V172X_BoardParams::GetPreNSamps(bool downsamp) const
{
  return (uint32_t)(pre_trigger_time_us * GetSampleRate(downsamp));
}

uint32_t V172X_BoardParams::GetTotalNSamps(bool downsamp) const
{
  uint32_t total_nsamps = (uint32_t)
    (( pre_trigger_time_us + post_trigger_time_us ) * GetSampleRate(downsamp));
  while( total_nsamps % (4/bytes_per_sample)) total_nsamps++;
  //if(total_nsamps%2) total_nsamps++;
  return total_nsamps;
}

uint32_t V172X_BoardParams::GetTotalNBuffers() const
{
  return (1<<GetBufferCode());
}

uint32_t V172X_BoardParams::GetBufferCode() const
{
  int max_samples = mem_size*Mbyte / bytes_per_sample;
  if(board_type == V1751)
    max_samples = (int)(Mbyte * (mem_size == 0x02 ? 1.835 : 14.4 ));
  else if(board_type == V1730)
    max_samples = ( mem_size == 1 ? 640*1024 : (int)(5.12*Mbyte));
  uint32_t buffer_code = (uint32_t)floor(log2(max_samples / 
					      GetTotalNSamps(false)));
  if(buffer_code > 0xA)
    buffer_code = 0xA;
  return buffer_code;
}

uint32_t V172X_BoardParams::GetCustomSizeSetting() const
{
  if(board_type == V1751)
    return GetTotalNSamps(false)/stupid_size_factor;
  else if(board_type == V1730)
    return GetTotalNSamps(false) / 10;
  return GetTotalNSamps(false)*bytes_per_sample/
    ( sizeof(uint32_t) * stupid_size_factor);
}

uint32_t V172X_BoardParams::GetPostTriggerSetting() const
{
  const int latency = 10;
  if(board_type == V1751)
    return GetPostNSamps(false) / 16 - latency;
  return GetPostNSamps(false)/(2*stupid_size_factor) - latency;
  
}

//return the index corresponding to the sample at the trigger time
int V172X_BoardParams::GetTriggerIndex(bool downsamp) const
{
  return GetPreNSamps(downsamp);
}

uint64_t V172X_BoardParams::GetTimestampRange() const
{
  return 0x7fffffff * ns_per_clocktick;
}

int V172X_Params::GetEnabledBoards()
{
  enabled_boards = 0;			       
  for(int i=0; i<nboards; i++)
    if(board[i].enabled) enabled_boards++;
  return enabled_boards;
}

int V172X_Params::GetEnabledChannels()
{
  enabled_boards = 0;
  enabled_channels = 0;
  for(int i=0; i<nboards; i++){
    if(!board[i].enabled) continue;
    enabled_boards++;
    for(int j=0; j<board[i].nchans; j++){
      if(board[i].channel[j].enabled) enabled_channels++;
    }
  }
  return enabled_channels;
}

//Get the max expected event size in bytes
int V172X_Params::GetEventSize(bool downsamp)
{
  enabled_boards = 0;
  enabled_channels = 0;
  event_size_bytes = 0;
  //event_size_bytes = event_size_padding;
  for(int i=0; i<nboards; i++){
    if(!board[i].enabled) continue;
    enabled_boards++;
    board[i].event_size_bytes = event_size_padding + 16; //padding + header
    for(int j =0; j<board[i].nchans; j++){
      if(board[i].channel[j].enabled){
	enabled_channels++;
	board[i].event_size_bytes += ( board[i].GetTotalNSamps(downsamp) * 
				       board[i].bytes_per_sample )
	  + (board[i].zs_type == ZLE ? 8 : 0 );
      }
    }//end for loop over channels
    event_size_bytes += board[i].event_size_bytes;
  }//end for loop over boards
  
  return event_size_bytes;
}

std::ostream& operator<<(std::ostream& out, const SIGNAL_LOGIC& logic)
{
  if(logic==NIM) 
    return out<<"NIM";
  else
    return out<<"TTL";
}

std::ostream& operator<<(std::ostream& out, const ZERO_SUPPRESSION_TYPE& zs)
{
  if(zs == NONE) 
    return out<<"NONE";
  else if(zs == ZS_INT)
    return out<<"ZS_INT";
  else if(zs == ZLE)
    return out<<"ZLE";
  else
    return out<<"ZS_AMP";
}

std::ostream& operator<<(std::ostream& out, const TRIGGER_POLARITY& pol)
{
  if(pol == TP_RISING)
    return out<<"TP_RISING";
  else 
    return out<<"TP_FALLING";
}

std::ostream& operator<<(std::ostream& out, const BOARD_TYPE& type)
{
  if(type == V1724)
    return out<<"V1724";
  else if (type == V1720)
    return out<<"V1720";
  else if (type == V1721)
    return out<<"V1721";
  else if (type == V1751)
    return out<<"V1751";
  else 
    return out<<"OTHER";
}

std::istream& operator>>(std::istream& in, SIGNAL_LOGIC &logic)
{
  std::string temp;
  in>>temp;
  if(temp == "NIM" || temp == "nim")
    logic = NIM;
  else if(temp == "TTL" || temp == "ttl")
    logic = TTL;
  else{
    Message e(EXCEPTION);
    e<<temp<<" is not a valid value for SIGNAL_LOGIC"<<std::endl;
    throw std::invalid_argument(e.str());
  }
  return in;
}
  
std::istream& operator>>(std::istream& in, ZERO_SUPPRESSION_TYPE &zs)
{
  std::string temp;
  in>>temp;
  if(temp == "NONE" || temp == "none")
    zs = NONE;
  else if(temp == "ZS_INT" || temp == "zs_int")
    zs = ZS_INT;
  else if(temp == "ZLE" || temp == "zle")
    zs = ZLE;
  else if(temp == "ZS_AMP" || temp == "zs_amp")
    zs = ZS_AMP;
  else{
    Message e(EXCEPTION);
    e<<temp<<" is not a valid value for ZERO_SUPPRESSION_TYPE"<<std::endl;
    throw std::invalid_argument(e.str());
  }
  return in;
}

std::istream& operator>>(std::istream& in, TRIGGER_POLARITY &pol)
{
  std::string temp;
  in>>temp;
  if(temp == "TP_RISING" || temp == "tp_rising")
    pol = TP_RISING;
  else if(temp == "TP_FALLING" || temp == "tp_falling")
    pol = TP_FALLING;
  else{
    Message e(EXCEPTION);
    e<<temp<<" is not a valid value for TRIGGER_POLARITY"<<std::endl;
    throw std::invalid_argument(e.str());
  }
  return in;
}

std::istream& operator>>(std::istream& in, BOARD_TYPE& type)
{
  std::string temp;
  in>>temp;
  if(temp == "V1724")
     type = V1724;
  else if(temp == "V1720")
    type = V1720;
  else if (temp == "V1721")
    type = V1721;
  else if (temp == "V1751")
    type = V1751;
  else if (temp == "OTHER")
    type = OTHER;
  else{
    type = OTHER;
    Message e(EXCEPTION);
    e<<temp<<" is not a valid value for BOARD_TYPE"<<std::endl;
    throw std::invalid_argument(e.str());
  }
  return in;
}


std::ostream& operator<<(std::ostream& out, const TRGOUT_MODE& m)
{
  std::string word="";
  switch(m){
  case TRIGGER:
    word = "TRIGGER";
    break;
  case BUSY:
    word = "BUSY";
    break;
  }
  //shouldn't get here
  return out<<word;
}

std::istream& operator>>(std::istream& in, TRGOUT_MODE& m)
{
  std::string tmp;
  in>>tmp;
  if(tmp=="TRIGGER")
    m = TRIGGER;
  else if(tmp == "BUSY")
    m = BUSY;
  else{
    Message e(EXCEPTION);
    e<<tmp<<" is not a valid value for TRGOUT_MODE\n";
    throw std::invalid_argument(e.str());
  }
  return in;
}
