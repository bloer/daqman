#include "runinfo.hh"

runinfo::runinfo(long id) : 
  ParameterList("runinfo","metadata about daq runs")
{
  Init();
  runid = id;
}

//reset variables that can be read from the raw data
void runinfo::ResetRunStats()
{
  starttime = 0;
  endtime = 0;
  triggers = 0;
  events = 0;
}

void runinfo::Init(bool reset)
{
  runid=-1;
  starttime=0;
  endtime=0;
  triggers=0;
  events=0;
  comment="";
  post_comment = "";
  pre_trigger_time_us = -1;
  post_trigger_time_us = -1;
  
  metadata.clear();
  required_metadata.clear();
  field_allowed_values.clear();
  channel_metadata.clear();
  
  if(!reset)
    InitializeParameterList();
}

void runinfo::InitializeParameterList()
{
  RegisterParameter("runid", runid, "unique ID number for the run");
  RegisterParameter("starttime",starttime,"Timestamp at start of run");
  RegisterParameter("endtime",endtime, "Timestamp at end of run");
  RegisterParameter("triggers",triggers,"Total triggers requested during run");
  RegisterParameter("events", events, "Number of events recorded during run");
  RegisterParameter("comment",comment,"Generic information about run");
  RegisterParameter("post_comment",post_comment,"More info after run finished");
  RegisterParameter("pre_trigger_time_us",pre_trigger_time_us,
		    "Length of pre-trigger digitization window in us");
  RegisterParameter("post_trigger_time_us",post_trigger_time_us,
		    "Length of post-trigger digitization window in us");
  
  RegisterParameter("metadata", metadata, 
		    "User-defined per run info categories");
  RegisterParameter("required_metadata", required_metadata,
		    "User metadata fields that will be prompted at run start");
  RegisterParameter("field_allowed_values", field_allowed_values,
		    "List of allowed values for fields with limited choices");
  RegisterParameter("channel_metadata", channel_metadata,
		    "map of channel ID to per-channel metadata");
}


bool runinfo::CheckFieldValue(const std::string& fieldname,
			      const std::string& val)
{
  //return true if this value is allowed
  stringsetmap::iterator it = field_allowed_values.find(fieldname);
  if( it != field_allowed_values.end()){
    return it->second.find(val) != it->second.end();
  }
  //else, this field is not limited
  return true;
}
