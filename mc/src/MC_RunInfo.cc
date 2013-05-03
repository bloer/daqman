#include "MC_RunInfo.hh"
#include "Message.hh"
// #include <exception>
// #include <stdexcept>
// #include <math.h>

using namespace std;

MC_RunInfo::MC_RunInfo() : ParameterList("runinfo") {

  RegisterParameter("runid", runid, "unique ID number for the run");
  RegisterParameter("starttime",starttime,"Timestamp at start of run");
  RegisterParameter("endtime",endtime, "Timestamp at end of run");
  RegisterParameter("events", events, "Number of events recorded during run");
  RegisterParameter("livetime", livetime, 
                    "Runtime scaled for number of accepted triggers");
  RegisterParameter("type", type, "Label giving purpose of run");
  RegisterParameter("comment",comment,"Generic information about run");
  RegisterParameter("drift_hv",drift_hv,"Setting of drift_hv field");
  RegisterParameter("extraction_hv",extraction_hv,"Extraction field setting");
  RegisterParameter("trigger_veto",trigger_veto,
                    "Length of per-trigger veto in ms; -1 to disable");
  RegisterFunction(channel_inserter::GetFunctionName(),
                   channel_inserter(this), channel_inserter(this),
                   "Insert a daq channel's info into the database");
  RegisterReadFunction(channel_clearer::GetFunctionName(),
                       channel_clearer(this),
                       "Clear the list of channel info");
}

MC_RunInfo::channelinfo::channelinfo() :
  ParameterList("channelinfo","Information about the state of a daq channel for database insertion"), 
  channel(-1) , voltage(0), amplification(1), spe_mean(1), spe_sigma(0), calibration_run(-1),
  calibration_channel(-1), cal_filters(2), reload_cal(false), 
  reload_cal_run(false)
{

  InitializeParameterList();

}

void MC_RunInfo::channelinfo::InitializeParameterList() {

  RegisterParameter("channel", channel, "Unique ID number");
  RegisterParameter("voltage",voltage, "High Voltage applied to PMT");
  RegisterParameter("amplification", amplification, 
                    "Amplification applied to channel after PMT output");
  RegisterParameter("spe_mean",spe_mean,
                    "Single photoelectron mean from calibration database");
  RegisterParameter("spe_sigma",spe_sigma,
                    "Single photoelectron sigma from calibration database");
  RegisterParameter("calibration_run", calibration_run,
                    "Run from which calibration data was loaded");
  RegisterParameter("calibration_channel", calibration_channel,
                    "Channel which should be used for calibration data");
  RegisterParameter("cal_filters", cal_filters,
                    "Only consider runs with x filters for calibration");
  RegisterParameter("reload_cal", reload_cal,
                    "Search for calibration info in DB rather than saved vals");
  RegisterParameter("reload_cal_run", reload_cal_run,
                    "Search for new calibration run rather than previous one");
}

/// Read time_t from a string
std::istream& operator>>(std::istream& in, MC_RunInfo::time_param& t)
{
  std::string dummy;
  in>>dummy;
  if(dummy[4] == '-'){
    //this is a string
    int year, month, day;
    sscanf(dummy.c_str(),"%4d-%2d-%2d",&year, &month, &day);
    in>>dummy;
    int hours, minutes, seconds;
    sscanf(dummy.c_str(),"%2d:%2d:%2d", &hours, &minutes, &seconds);
    struct tm timeinfo;
    timeinfo.tm_sec = seconds;
    timeinfo.tm_min = minutes;
    timeinfo.tm_hour = hours;
    timeinfo.tm_mday = day;
    timeinfo.tm_mon = month-1;
    timeinfo.tm_year = year-1900;
    timeinfo.tm_isdst=-1;
        
    t.t = timegm(&timeinfo);
    return in;
  }
  //if we get here, it should be an integer
  t.t = atoi(dummy.c_str());
  return in;
}
/// Make time_t print as a string
std::ostream& operator<<(std::ostream& out,MC_RunInfo::time_param& t)
{
  char str[20];
  std::strftime(str,20,"%Y-%m-%d %H:%M:%S",std::gmtime(&t.t));
  out<<str;
  return out;
}

std::istream& MC_RunInfo::channel_clearer::operator()(std::istream& in)
{
  _parent->channels.clear();
  return in;
}  

std::istream& MC_RunInfo::channel_inserter::operator()(std::istream& in){
  channelinfo info;
  in>>info;
  //make sure it isn't there already
  for(size_t i=0; i < _parent->channels.size(); i++){
    if(_parent->channels[i].channel == info.channel){
      _parent->channels[i] = info;
      return in;
    }
  }
  _parent->channels.push_back(info);
  return in;
}

std::ostream& MC_RunInfo::channel_inserter::operator()(std::ostream& out){
  out<<"\n"<<channel_clearer::GetFunctionName()<<" ,\n";
  for(size_t i=0; i < _parent->channels.size(); i++){
    out<<GetFunctionName()<<" "<<_parent->channels.at(i)<<" ";
  }
  return out;
}  
