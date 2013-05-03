#include "BaseModule.hh"
#include "AddCutFunctor.hh"

struct CutClearer{
  BaseModule* _parent;
  CutClearer(BaseModule* parent) : _parent(parent) {}
  std::istream& operator()(std::istream& in){
    _parent->ClearCuts();
    return in;
  }
};

struct DependencyAdder{
  BaseModule* _parent;
  DependencyAdder(BaseModule* parent) : _parent(parent){}
  std::istream& operator()(std::istream& in){
    std::string depmodule;
    in>>depmodule;
    _parent->AddDependency(depmodule);
    return in;
  }
};

BaseModule::BaseModule(const std::string& name, const std::string& helptext) : 
  ParameterList(name, helptext), _last_process_return(0)
{
  RegisterParameter("enabled", enabled = true,
		    "Is this module enabled for this run?");
  RegisterParameter("skip_channels",_skip_channels,
		    "List the channels which we don't process");
  RegisterFunction(AddCutFunctor::GetFunctionName(), 
		   AddCutFunctor(this), AddCutFunctor(this),
		   "Add a cut which events must pass before processing");
  RegisterReadFunction("clear_cuts", CutClearer(this),
		       "Clear the list of defined cuts");
  RegisterReadFunction("add_dependency",DependencyAdder(this),
		       "List another module as a dependency");
  
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

std::ostream& operator<<(std::ostream& out, BaseModule::ChannelSkipper& ch)
{
  out<<" ( ";
  for( std::set<int>::iterator it = ch.begin(); it != ch.end(); it++)
    out<<*it<<" ";
  out<<" ) ";
  return out;
}

std::istream& operator>>(std::istream& in, BaseModule::ChannelSkipper& ch)
{
  ch.clear();
  std::string temp="";
  while( in>>temp && temp != ")" && temp != "}" && temp != "]"){
    if(temp == "," || temp == "(" || temp == "[" || temp == "{" ) continue;
    int channel = atoi(temp.c_str());
    ch.insert(channel);
  }
  return in;
}
