#include "ProcessingCut.hh"
 
ProcessingCut::ProcessingCut(const std::string& cut_name, 
			     const std::string helptext) :
  ParameterList(cut_name, helptext), _cut_name(cut_name)
{
  RegisterParameter("default_pass", default_pass = true, 
		    "Do we require all channels to pass for the event to pass (false) or only one (true)?");
}


bool ProcessingCut::Process(EventDataPtr event)
{
  bool pass_once = false;
  for(size_t i=0; i < event->channels.size(); i++){
    bool pass = ProcessChannel(&(event->channels[i]));
    if( pass && default_pass ) return true;
    if( !pass && !default_pass ) return false;
    if(pass) pass_once = true;
  }
  return pass_once;
}
