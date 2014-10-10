#include <stdexcept>
#include <stdlib.h>
#include <algorithm>
#include "EventHandler.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "Message.hh"
#include "BaseModule.hh"
#include "AsyncEventHandler.hh"
#include <stdexcept>
#include <sstream>

//These functions are used by the ConfigHandler as command switches
class EnableModule{
  const bool _enable;
public:
  EnableModule(const bool& enable) : _enable(enable) {}
  int operator()(const char* modname){
    EventHandler* h = EventHandler::GetInstance();
    BaseModule* mod = (BaseModule*)(h->GetParameter(modname));
    if(!mod) {
      Message(ERROR)<<"Unknown module "<<modname<<std::endl;
      throw std::invalid_argument("Unknown module");
    }
    mod->enabled = _enable;
    return 0;
  }
};

int ListModules(const char*){
  std::cout<<"The available modules are:\n";
  const std::vector<BaseModule*>* modules = 
    EventHandler::GetInstance()->GetListOfModules();
  for(size_t i=0; i< modules->size(); i++){
    std::cout<<"\t"<<modules->at(i)->GetName()<<std::endl;
  }
  exit(0);
}

//EventHandler member functions

EventHandler::EventHandler() : 
  ParameterList("modules","Takes raw events and delivers it to all enabled modules for processing"), 
  _current_event(), _is_initialized(false), run_id(-1)
{
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->RegisterParameter(this->GetDefaultKey(),*this);
  config->RegisterParameter(_runinfo.GetDefaultKey(), _runinfo);
  
  RegisterParameter("access_database", _access_database = false,
		    "Enable or disable db access. Must configure to work!");
  RegisterParameter("configure_database",_dbconfig,
		    "Init and configure a concrete database interface");
  
  RegisterParameter("fail_on_bad_cal", _fail_on_bad_cal=false,
		    "Fail to initialize if unable to  find calibration data");
  RegisterParameter("run_parallel", _run_parallel=false,
		    "Do we process modules in series, or give them threads?");
  config->AddCommandSwitch(' ',"enable","enable <module>",
			   EnableModule(true),"module");
  config->AddCommandSwitch(' ',"disable","disable <module>",
			   EnableModule(false),"module");
  config->AddCommandSwitch(' ',"list-modules","list the available modules",
			   ListModules);
  config->AddCommandSwitch(' ',"no-db","Skip attempts to access database",
			   CommandSwitch::SetValue<bool>(_access_database,false)
			   ); 
  
}
//Copy, assignment constructors not provided

EventHandler::~EventHandler()
{
  if(_is_initialized)
    Finalize();
  for(size_t i=0; i<_modules.size(); i++){
    delete _modules[i];
  }
  _modules.clear();
  
}

EventHandler* EventHandler::GetInstance()
{
  static EventHandler handler;
  return &handler;
}

int EventHandler::AddModule(BaseModule* mod, bool processme, bool registerme)
{
  if(!mod)
    return -1;
  //make sure module is not already considered
  if(std::find(_modules.begin(),_modules.end(),mod)==_modules.end()){
    _modules.push_back(mod);
  }
  if(processme)
    _processing_modules.push_back(mod);
  if(registerme)
    RegisterParameter(mod->GetName(), *mod);
  return _modules.size();
}

BaseModule* EventHandler::GetModule(const std::string& modname)
{
  for(size_t i=0; i<_modules.size(); ++i){
    if(_modules[i]->GetName() == modname)
      return _modules[i];
  }
  //if we get here, module is not known
  return 0;
}

int EventHandler::Initialize()
{
  if(_is_initialized) return 1;
  Message(DEBUG)<<"EventHandler::Initialize() called with  "<<_modules.size()
		<<" registered modules...\n";
  _is_initialized = true;
  
  /*initialize the runinfo
    order of priority for metadata settings is: (higher numbers override)
    1) saved in run's config file
    2) stored in database
    3) written into current programs runinfo config block
    
    But our existing runinfo is already read from the config file, so 
    we'll have to use some temporaries to get the right override order
  */
  

  if(_runinfo.runid == -1)
    _runinfo.runid = run_id;

  runinfo savedinfo;
  //first look for info in the saved config file
  try{
    ConfigHandler::GetInstance()->LoadParameterList(&savedinfo);
  }
  catch(std::exception& e){
    Message(WARNING)<<"Saved runinfo will not be used in this processing!\n";
  }
  
  //now try the database
  try{
    VDatabaseInterface* db = GetDatabaseInterface();
    if(db){
      db->Connect();
      runinfo dbinfo = db->LoadRuninfo(_runinfo.runid);
      if(dbinfo.runid == _runinfo.runid){ //make sure we actually loaded
	savedinfo.MergeMetadata(&dbinfo, true); //overwrite settings
      }
    }
  }
  catch(std::exception& e){
    Message(ERROR)<<"There was an error reading from the database: "
		  <<e.what()<<"\n";
    if(_fail_on_bad_cal)
      return 1;
    _access_database = false;
  }
  
  //finally merge into "the" runinfo object
  _runinfo.MergeMetadata(&savedinfo, /*overwritedups = */false);
  

  //this info is in the raw file, so reset it:
  _runinfo.ResetRunStats();
  
  //first initialize all enabled modules
  std::set<std::string> enabled_modules;
  
  for(size_t i = 0; i<_modules.size(); i++){
    BaseModule* mod = _modules[i];
    if(!mod->enabled){
      Message(DEBUG)<<"Module "<<mod->GetName()<<" was disabled manually.\n";
      continue;
    }
    //see if all our dependencies are enabled 
    const std::set<std::string>* deps = mod->GetDependencies();
    std::set<std::string>::const_iterator dep = deps->begin();
    for( ; dep != deps->end(); ++dep){
      if( enabled_modules.find(*dep) == enabled_modules.end()){
	mod->enabled = false;
	Message(DEBUG)<<"Module "<<mod->GetName()<<" was disabled by "
		      <<"failed dependency "<<*dep<<".\n";
	break;
      }
    }
    if(!mod->enabled)
      continue;
    //if we get here, the module is enabled and ready to initialize
    Message(DEBUG)<<"Module "<<mod->GetName()<<" is enabled. Initializing...\n";
    enabled_modules.insert(mod->GetName());
    if(mod->Initialize()){
      Message(CRITICAL)<<"Unable to initialize module "<<mod->GetName()
		       <<"; aborting.\n";
      _is_initialized = false;
      return 1;
    }
    if(_run_parallel){
      //give this module an AsyncEventHandler
      AsyncEventHandler* ah = new AsyncEventHandler;
      ah->SetBlockingStatus(true);
      ah->AddModule(mod,false);
      if(_para_handlers.size()==0)
	AddAsyncReceiver(ah);
      else
	_para_handlers.back()->AddReceiver(ah);
      ah->StartRunning();
      _para_handlers.push_back(ah);
    }
  }
  
  return 0;
}

int EventHandler::Process(RawEventPtr raw)
{
  if(!raw){
    Message(ERROR)<<"Attempted to process empty event pointer.\n";
    return 1;
  }
  EventPtr evt(new Event(raw));
  return Process(evt);
}

int EventHandler::Process(EventPtr evt)
{  
  
  if(!_is_initialized) {
    Message(ERROR)<<"Attempted to process events before initialization!\n";
    throw std::runtime_error("EventHandler::uninitialized process request");
    return 1;
  }
  
  int proc_fail = 0;
  
  _current_event = evt;
  //set the run id here
  _current_event->GetEventData()->run_id = run_id;
  if(!_run_parallel){
    std::vector<BaseModule*>::iterator it;
    for(it=_processing_modules.begin(); it!=_processing_modules.end(); it++){
      BaseModule* mod = *it;
      if(mod->enabled){
	//Message(DEBUG)<<"Processing module "<<mod->GetName()<<std::endl;
	proc_fail += mod->HandleEvent(_current_event);
      }
    }
  }
  for(size_t i=0; i < _async_receivers.size(); ++i)
    _async_receivers[i]->Process(evt);

  return proc_fail;
}

int EventHandler::Finalize()
{
  if(!_is_initialized) {
    Message(WARNING)<<"EventHandler::Finalize() called uninitialized!\n";
  }
  _is_initialized = false;
  //make sure the modules have finished
  for(size_t i=0; i<_async_receivers.size(); ++i){
    _async_receivers[i]->Process(EventPtr());
  }
  if(_run_parallel){
    for(size_t i=0; i<_para_handlers.size(); ++i){
      //this should block until done processing
      _para_handlers[i]->Process(EventPtr());
      _para_handlers[i]->StopRunning();
      delete _para_handlers[i];
      //note: assume no one has messed with the receiver list since Initialize
      if(i==0)
	_async_receivers.pop_back();
    }
  }
  int final_fail = 0;
  Message(DEBUG)<<"Finalizing "<<_modules.size()<<" modules..."<<std::endl;
  //finalization should go in opposite order of initialization
  //but that messes up root file writing, so go in same order...                
  std::vector<BaseModule*>::iterator it;
  for(it = _modules.begin(); it != _modules.end(); it++){
    BaseModule* mod = *it;
    if(mod->enabled){
      Message(DEBUG)<<"Finalizing module "<<mod->GetName()<<std::endl;
      final_fail += mod->Finalize();
    }
  }
  //reset the run info
  _runinfo.Init(true);
  Message(DEBUG)<<"Done finalizing modules.\n";
  if(final_fail)
    Message(WARNING)<<"Finalization returned error code "<<final_fail<<"\n";
  return final_fail;
}   
 
int EventHandler::SetRunIDFromFilename(const std::string& filename)
{
  run_id = -1;
  //see if the filename matches the pattern *Run######.out.gz
  std::string filepart = filename;
  //remove any trailing '/'
  while(*filepart.rbegin() == '/')
    filepart.resize(filepart.size()-1);
  size_t basept = filepart.find_last_of('/');
  if( basept == std::string::npos) 
    basept = 0;
  else 
    basept++;
  filepart = filepart.substr(basept);
  size_t runpt = filepart.find("Run");
  if( runpt != std::string::npos && filepart.size()>runpt+8){
    run_id = atoi(filepart.substr(runpt+3,6).c_str());
  }
  else{
    //might be of the form <name>_yymmddHHMM.<suffix>
    size_t underscore = filepart.find_last_of('_');
    if(underscore != std::string::npos && filepart.size() > underscore+10){
      std::stringstream s(filepart.substr(underscore+1,10));
      s >> run_id;
    }
  }

  Message(INFO)<<"Setting runid to "<<run_id<<" based on filename "<<filename
	       <<std::endl;
 return run_id;
}
  
