//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @defgroup ConfigHandler ConfigHandler - Runtime configuration from command line or config files
    
    @file ConfigHandler.hh
    @brief Defines the ConfigHandler global configuration class
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef CONFIGHANDLER_h
#define CONFIGHANDLER_h 1
#include <string>
#include <set>
#include <vector>
#include "ParameterList.hh"
#include "phrase.hh"

/// @addtogroup ConfigHandler
/// @{

/** @class ConfigHandler
    @brief Global singleton ParameterList to handle all configuration
    
    Classes which want their parameters handled by the master config file
    or which want to have command line switches should register them 
    with ConfigHandler at the beginning of main(), or during a static
    initialization routine.
  
    @todo What happens if two classes try to register params/switches with 
    the same name, especially statically?
*/
class ConfigHandler : public ParameterList{
public:
  /// Get the global singleton pointer - only way to construct
  static ConfigHandler* const GetInstance()
  { 
    static ConfigHandler config;
    return &config;
  }
    
  /** @brief Add a command line switch.  
      
      Only one of shortname,longname are required
      use ' ' (space) and "" (empty string) to skip either parameter
      If the switch requires a parameter, it must be specified in the 4th slot
      
      The Action class object supplied MUST be a functor which defines a
      int operator()(const char* ) method, even if it takes no argument
      
      Returns 0 if no error occurred. 
      
      @param shortname allow user to call the switch with a single dash (-<c>)
      @param longname allow user to call switch with double dash name
      @param helptext a short description of what this switch does
      @param action functor which is called when switch is found on command line
      @param parameter name of the argument parameter if the switch needs one
  */
  template<class Action> 
  int AddCommandSwitch(char shortname,
		       const std::string& longname,
		       const std::string& helptext,
		       Action action,
		       const std::string& parameter = "");
  /// Remove a previously registered command line switch
  int RemoveCommandSwitch(char shortname, const std::string& longname);
  
  ///Prints info for the registered switches
  void PrintSwitches(bool quit=true, std::ostream& out = std::cout,
		     bool escape=false);
  ///Set the usage string given by the '-h' or '--help' options
  void SetProgramUsageString(const std::string& usage){ _program_usage=usage;}
  /// Get the program usage string
  std::string GetProgramUsageString(){ return _program_usage; }
  /// Set the program's description
  void SetProgramDescription(const std::string& d){ _program_description=d;}
  /// Get the program's description
  std::string GetProgramDescription(){ return _program_description; }
  
  ///Process the command line for the registered switches.
  ///Returns 0 in case of success
  int ProcessCommandLine(int& argc, char** argv);
  
  ///Get the number of non-switch arguments to the command line.
  ///Rreturns neg if command line has not been processed yet
  int GetNCommandArgs() { return _cmd_args.size(); }
  
  ///Get the nth non-switch command line argument
  const char* GetCommandArg(size_t n) { return _cmd_args.at(n); }
  
  //functions for loading a saved parameter list
  /// Allow classes to read an OLD config file without changing current params
  void SetSavedCfgFile(const std::string& fname){ _saved_cfg = fname; }
  /// Allow classes to read a separate config file than the active one
  const std::string& GetSavedCfgFile(){ return _saved_cfg; }
  /// Load a parameter list from an old config file
  template<class T> bool LoadCreateParameterList(T*& par, std::string key="",
						 bool registerme = false);
  /// Load a parameter list from an old config file with pointer value
  template<class T> bool LoadParameterList(T* par, std::string key="",
					   bool registerme = false);
  /// Set the notes string
  void SetNotes(const std::string& newnotes) { _notes = newnotes; }
  /// Get the string containing the notes for this program
  const std::string& GetNotes(){ return _notes; }

  /// Set the default cfg file
  void SetDefaultCfgFile(const std::string& file) { _default_cfg_file = file; }
  /// Get the default config file
  const std::string& GetDefaultCfgFile(){ return _default_cfg_file; }
  
  //search multiple paths for a config file
  std::string FindConfigFile(const std::string& fname);

  ///Should we hide all children of disabled lists when printing? 
  void CollapseDisabledLists(bool collapse) { _collapse_disabled = collapse; }
  
  
private:
  std::string _program_usage;      ///< String detailing how to use program
  std::string _program_description; ///< String describing the program

  std::string _notes;                ///< specify comments in a config file
  std::string _default_cfg_file;    ///< default config file for this program
  std::string _saved_cfg;             ///< name of old config file

  std::vector<std::string> _cfg_paths; ///<list of paths to search for cfg files
  
  /// Default constructor is private; singleton implementation
  ConfigHandler(); 
  /// Destructor also private
  ~ConfigHandler();
  
  /// Copy constructor private
  ConfigHandler(const ConfigHandler& right) : ParameterList(),
					      _switches(right._switches) {}
  
  /// Copy constructor private
  ConfigHandler& operator=(const ConfigHandler& right)
  { _switches = right._switches; return *this; }
  
  std::vector<char*> _cmd_args;  ///< command-line arguments read
  
  
  /** @class VCommandSwitch
      @brief Abstract base class represents command line switch for containers
  */
  class VCommandSwitch{
  public:
    char shortname;               ///< single dash callable character
    std::string longname;         ///< double dash callable string
    std::string helptext;         ///< Short descriptive phrase 
    std::string parameter;        ///< Name of parameter if required
    /// Constructor
    VCommandSwitch(char short_name, const std::string& long_name,
		   const std::string& help_text, const std::string& par) :
      shortname(short_name), longname(long_name), helptext(help_text), 
      parameter(par) {}
    /// Destructor
    virtual ~VCommandSwitch() {}
    /// Process a command switch, optionally reading an argument
    virtual int Process(const char* arg) = 0;
  };
  
  /** @class CommandSwitch
      @brief Concrete implementation of VCommandSwitch
      
      Holds the user-defined function to call when the switch is read
  */
  template<class Action> class TCommandSwitch : public VCommandSwitch{
    Action do_action;
  public:
    TCommandSwitch(char short_name, const std::string& long_name,
		  const std::string& help_text, const std::string& par,
		  Action action) : 
      VCommandSwitch(short_name, long_name, help_text, par), 
      do_action(action) {}
    virtual ~TCommandSwitch() {}
    virtual int Process(const char* arg);
  };
  
  ///utility class for sorting command switches
  struct OrderCommandSwitchPointers{
    bool operator()(VCommandSwitch* a, VCommandSwitch* b);
  };

  typedef std::set<VCommandSwitch*, OrderCommandSwitchPointers> SwitchSet;
  SwitchSet _switches; ///< set of all registered command switches
};

//templates must be defined in header
template<class Action> 
int ConfigHandler::AddCommandSwitch(char shortname,
				    const std::string& longname,
				    const std::string& helptext,
				    Action action,
				    const std::string& parameter)
{
  //Make sure both shortname and longname aren't empty
  if(shortname == ' ' && longname == ""){
    std::cerr<<"Error: At least one of shortname and longname must be non-empty for switches!"<<std::endl;
    return -1;
  }
  //Make sure there isn't already a registered switch  with the same keys
  for( SwitchSet::iterator it = _switches.begin(); it != _switches.end(); it++){
    if( shortname != ' ' && shortname == (*it)->shortname){
      std::cerr<<"Switch already registered with shortname '"
	       <<shortname<<"'"<<std::endl;
      return -2;
    }
    else if( longname != "" && longname == (*it)->longname){
      std::cerr<<"Switch already registered with longname \""
	       <<longname<<"\""<<std::endl;
      return -3;
    }
  }
  _switches.insert(new TCommandSwitch<Action>(shortname, longname, helptext,
					     parameter, action) );
  return 0;
}


template<class Action> inline
int ConfigHandler::TCommandSwitch<Action>::Process(const char* arg)
{
  return do_action(arg);
}


//load a saved parameter list
template<class T> inline
bool ConfigHandler::LoadCreateParameterList(T*& par, 
					    std::string key, 
					    bool registerme)
{
  if(!par){
    boost::shared_ptr<VParameterNode> ptr(par = new T);
    //..par = ptr.get();
    _deleter.push_back(ptr);
  }
  return LoadParameterList(par, key, registerme);
}

template<class T> inline 
bool ConfigHandler::LoadParameterList(T* par,
				      std::string key,
				      bool registerme)
{
  bool err=1;
  if(key == "")
    key = par->GetDefaultKey();
  if(registerme)
    RegisterParameter(key, *par);
  if(_saved_cfg != "")
    err = !(par->ReadFromFile(_saved_cfg.c_str(), key, true));
  return err;
}

/// @}
#endif
