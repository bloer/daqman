#include "BaseModule.hh"
#include "AddCutFunctor.hh"


BaseModule::BaseModule(const std::string& name, const std::string& helptext) : 
  ParameterList(name, helptext), _last_process_return(0)
{
  RegisterParameter("enabled", enabled = true,
		    "Is this module enabled for this run?");
  RegisterParameter("skip_channels",_skip_channels,
		    "List the channels which we don't process");
  //RegisterParameter("cuts", _cuts, "List cuts to pass before processing");
  RegisterParameter("dependencies", _dependencies, 
		    "List of modules that need to process before us");
}

BaseModule::~BaseModule()
{
  
  //delete cuts
  for(size_t i=0; i < _cuts.size(); i++)
    delete _cuts[i];
  _cuts.clear();
}


int BaseModule::HandleEvent(EventPtr event, bool process_now)
{
  // see if our cuts pass, and actually do the processing
  if(CheckCuts(event))
    _last_process_return = Process(event);
  else
    _last_process_return = 0;
  if(_last_process_return){
    Message(ERROR)<<"Module "<<GetName()<<" returns "
		  <<_last_process_return
		  <<" processing event "
		  <<event->GetRawEvent()->GetID()<<"\n";
  }
  return _last_process_return;    
}

int BaseModule::AddDependency(const std::string& module)
{
  _dependencies.insert(module);
  return _dependencies.size();
}

      
bool BaseModule::CheckCuts(EventPtr event){
  
  for(std::vector<ProcessingCut*>::iterator cutit = _cuts.begin(); 
      cutit != _cuts.end(); cutit++){
    if((*cutit) -> Process(event->GetEventData()) == false)
      return false;
  }
  return true;
}

void BaseModule::ClearCuts()
{ 
  for(std::vector<ProcessingCut*>::iterator it = _cuts.begin(); 
      it != _cuts.end(); it++){
    delete *it;
  }
  _cuts.clear();
}

