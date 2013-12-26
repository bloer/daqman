/** @file EventHandler.hh
    @brief Defines the EventHandler module controller
    @author bloer
    @ingroup modules
*/

#ifndef EVENTHANDLER_h
#define EVENTHANDLER_h

#include "ParameterList.hh"
#include "Event.hh"
#include "runinfo.hh"
#include <string>
#include <vector>

class BaseModule;
class AsyncEventHandler;

/** @class EventHandler
    @brief Master class which controls processing events by enabled modules
    @ingroup modules
*/
class EventHandler : public ParameterList{
private:
  ///EventHandler is singleton, so no public constructor
  EventHandler();
  ///EventHandler is singleton, so no public constructor
  EventHandler(const EventHandler& right);
  /// Copy constructor private
  EventHandler& operator=(const EventHandler& right);

public:
  /// Get the global singleton pointer
  static EventHandler* GetInstance();
  /// Destructor
  ~EventHandler();
  
  /// Add a new module to handle. If !processme, assume it's called outside 
  int AddModule(BaseModule* mod, 
		bool processme = true,
		bool registerme = true);
  /// Get a module by name, return 0 if doesn't exist
  BaseModule* GetModule(const std::string& modname);
  
  /// Keep the template methods for compatibility 
  /// Add a module by class
  template<class Module> 
  Module* AddModule(const std::string& name=Module::GetDefaultName(),
		    bool processme = true,
		    bool registerme = true);
  /// Get an already registered module by class; return null if not found
  template<class Module>
  Module* GetModule(const std::string& name=Module::GetDefaultName());
  
  /// Add all of the processing modules, but not writers or viewers
  int AddCommonModules(); ///< @todo: should we get rid of this?
  
  /// Add an asynchronous event handler to receive processed events
  void  AddAsyncReceiver(AsyncEventHandler* handler)
  { if(handler) _async_receivers.push_back(handler); }
  /// Clear all async receivers
  void ClearAsyncReceivers(){ _async_receivers.clear(); }
  
  //public interface functions
  //return 0 if no errors
  /// Initialize all registered modules
  int Initialize();
  /// Create an Event and process it with all enabled modules
  int Process(RawEventPtr raw);
  /// Process externally created event on all enabled modules
  int Process(EventPtr evt);
  /// Finalize all registered and enabled modules
  int Finalize();
  
  /// Get a pointer to the current event being processed
  EventPtr GetCurrentEvent(){ return _current_event; }
  /// Get the list of all defined modules const-ly
  const std::vector<BaseModule*>* GetListOfModules() const{ return &_modules; }
  /// Get the list of all defined modules
  std::vector<BaseModule*>* GetListOfModules(){ return &_modules;}
  /// Get only modules that we call Process on const-ly
  const std::vector<BaseModule*>* GetProcessingModules() const 
  { return &_processing_modules; }
  /// Get only modules that we call Process on
  std::vector<BaseModule*>* GetProcessingModules() 
  { return &_processing_modules;}
  
  /// Set the ID number of this run
  void SetRunID(int id){ run_id = id; }
  /// Set the ID number of this run based on a filename
  int SetRunIDFromFilename(const std::string& filename);
  /// Get the ID number of this run
  int GetRunID(){ return run_id; }
  /// Get the database info about the run
  runinfo* GetRunInfo(){ return &_runinfo; }
  /// Set whether to load calibration info from database at initialize
  void AllowDatabaseAccess(bool setval){ _access_database=setval; }
  /// Check whether we are supposed to fail on bad calibration
  bool GetFailOnBadCal() const { return _fail_on_bad_cal;}
private:
  std::vector<BaseModule*> _modules;
  std::vector<BaseModule*> _processing_modules;
  std::vector<AsyncEventHandler*> _async_receivers;
  EventPtr _current_event;
  bool _is_initialized;  ///< are the modules initialized?
  int run_id;
  runinfo _runinfo; ///< general metadata about the run
  bool _access_database; ///< do we query the database for missing info?
  bool _fail_on_bad_cal; ///< Fail to initialize if no calibration data found
  
  bool _run_parallel;   ///< process modules in parallel
  std::vector<AsyncEventHandler*> _para_handlers;
};

#include "BaseModule.hh"

template<class Module> inline 
Module* EventHandler::AddModule(const std::string& name,
				bool processme,
				bool registerme)
{
  for(std::vector<BaseModule*>::iterator it = _modules.begin(); 
      it != _modules.end(); it++){
    if( (*it)->GetName() == name && dynamic_cast<Module*>(*it) )
      return dynamic_cast<Module*>( *it );
  }
  //if we get here, this module doesn't exist yet, so add it
  BaseModule* newmod = new Module;
  newmod->SetName(name);
  AddModule(newmod, processme, registerme);
  return dynamic_cast<Module*>( newmod );
}

template<class Module> inline 
Module* EventHandler::GetModule(const std::string& name)
{
  for(std::vector<BaseModule*>::iterator it = _modules.begin(); 
      it != _modules.end(); it++){
    if( (*it)->GetName() == name && dynamic_cast<Module*>(*it) )
      return dynamic_cast<Module*>( *it );
  }
  //if we get here, this module doesn't exist yet
  return 0;
}


#endif
  
