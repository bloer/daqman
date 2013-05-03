/** @defgroup modules modules - functions to process raw data in series 

    @file BaseModule
    @brief defines the BaseModule abstract processing class
    @author bloer
    @ingroup modules
*/

#ifndef BASEMODULE_h
#define BASEMODULE_h

#include "Event.hh"
#include "ParameterList.hh"
#include "ProcessingCut.hh"
#include <iostream>
#include <string>
#include <set>

/** @class BaseModule
    @brief Abstract base module class

    All processing functions should inherit from BaseModule; EventHandler 
    contains a set of BaseModule pointers. 
    
    Inherited classes must override the Process function, and can optionally 
    override the Initialize and Finalize functions.  For all the templated
    dependency functions to work, modules must also override the 
    const std::string GetDefaultName() function.

    UPDATE 2012-03-22 (bloer): Modules are no longer allowed to disable 
    themselves during the Initialize() step.  If config settings are found
    to have improper values, etc, the module should print an error message
    and return failure.  

    Dependencies have also been updated in several ways:
    1) No more templates; dependencies are specified by name.  If you really
    want to make sure you're using the right name, call 
    AddDependency(Module::GetDefaultName())
    
    2) The only thing the dependency mechanism does now is check that all
    listed dependencies are enabled after configuration. Previously,
    You could add "downstream" modules without explicitly adding their 
    dependencies, e.g., you could add Integrator without first adding
    BaselineFinder, and it would automatically be added.  This is no longer 
    supported.  Automated ordering of modules based on dependencies is also 
    no longer supported.  This means that it is up to the user to add
    modules in the right order, or things will not work!  
    The only check that will be taken is, when you try to register a new module,
    if it's dependencies are not yet registered, you will get an error
    
    3) As a consequence of 2), there is no longer and such thing as an 
    optional dependency, since that only controlled ordering previously.
    ALL dependencies should be declared in the constructor
    
    4) You can additionally add dependencies in the config file. This is 
    primarily to handle auto-disabling of spectrum generators

    @ingroup modules
*/
class BaseModule : public ParameterList{
public:
  //simple constructor and desctructor
  //concrete implemenations should declare dependencies in their constructors
  /// Constructor takes module name
  BaseModule(const std::string& name="", const std::string& helptext="");
  /// virtual desctructor
  virtual ~BaseModule();
  /// Get the registered name of the module (and parameter list)
  const std::string& GetName() { return GetDefaultKey(); }
  /// Set the name of the module
  void SetName(const std::string& name){ SetDefaultKey(name); }
  //Access operators, to be overridden by concrete implemenation
  //all return 0 when no errors
  /// Initialize the module before starting a run. Return 0 if no error
  virtual int Initialize() {return 0;};
  /// Process a single event. Must be overridden. Return 0 if no error
  virtual int Process(EventPtr event)=0;
  /// Finalize state after a run has processed. Return 0 if no error
  virtual int Finalize() {return 0;};
  
  /// This function is called to handle things like cuts before real processing
  int HandleEvent(EventPtr event, bool process_now = false);
  
  //all modules have three parameters by default
  bool enabled;   ///< should this module be called?
  
  /// Get the return value of the last Process() call
  int GetLastProcessReturn(){ return _last_process_return; }
  
  /// State that we want another module to run first
  int AddDependency(const std::string& module);
  
  /// Keep the templated function for backward compatibility
  template<class T> int AddDependency()
  { return AddDependency(T::GetDefaultName()); }

  /// Get the list of modules which this module depends on
  const std::set<std::string>* GetDependencies(){ return &_dependencies; }
    
  /// Add a condition to determine whether this module will process an event
  void AddProcessingCut(ProcessingCut* cut){ _cuts.push_back(cut); }
  /// Get the list of cuts
  const std::vector<ProcessingCut*>* GetCuts(){ return &_cuts; }
  /// Clear the list of cuts
  void ClearCuts();
  /// Check whether <event> passes all defined cuts
  bool CheckCuts(EventPtr event);
  
  /** @typedef ChannelSkipper
      @brief Allows one to skip processing some channels
  */
  typedef std::set<int> ChannelSkipper;
protected:  
  int _last_process_return; ///< Value returned from last Process call
  std::set<std::string> _dependencies; ///< list of modules we need to run first
  std::vector<ProcessingCut*> _cuts; ///< list of cuts to take before processing
  ChannelSkipper _skip_channels; ///< list of channels not to process
  
};

/// overload ostream operator to let ChannelSkipper write to config files
std::ostream& operator<<(std::ostream& out, BaseModule::ChannelSkipper& ch);

/// overload istream operator to let ChannelSkipper read from config files
std::istream& operator>>(std::istream& in, BaseModule::ChannelSkipper& ch);


#endif
  
