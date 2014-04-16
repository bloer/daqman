//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file CommandSwitchFunctions.hh
    @brief Defines some commonly used functors for command switches
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef COMMANDSWITCHFUNCTIONS_h
#define COMMANDSWITCHFUNCTIONS_h
#include <sstream>
#include <string>
#include "VParameterNode.hh"
#include "ConfigHandler.hh"
 
/** @namespace CommandSwitch
    @brief Defines commonly used functors for command switches
    @ingroup ConfigHandler
*/
namespace CommandSwitch{
  
  /// @addtogroup ConfigHandler
  /// @{
  
  /** @class DefaultRead
      @brief read a single variable from the command line using stream operator
  */
  template<class T> class DefaultRead{
    T& param;
  public:
    DefaultRead(T& t) : param(t) {}
    int operator()(const char* newval)
    { 
      std::istringstream s(newval);
      if(s >> param || s.eof())
	return 0;
      return -1;
    }
  };
  //specialize for string
  template<> class DefaultRead<std::string>{
    std::string& param;
  public:
    DefaultRead(std::string& s) : param(s) {}
    int operator()(const char* newval) { param = newval; return 0; }
  };
   
  /** @class LoadConfigFile
      @brief Load a parameter list from the specified config file
  */
  class LoadConfigFile{
    VParameterNode *l;
  public:
    LoadConfigFile(VParameterNode* par) : l(par) {}
    int operator()(const char* file)
    { 
      std::string filepath = ConfigHandler::GetInstance()->FindConfigFile(file);
      if(filepath=="")
	return 1;
      return l->ReadFromFile(filepath.c_str()); 
    }
  };
  
  /** @class SetValue
      @brief Set a variable to the pre-defined supplied value
  */
  template<class T> class SetValue{
    T& param;
    const T value;
  public:
    SetValue(T& par, const T& val) : param(par), value(val) {}
    int operator()(const char*)
    {
      param = value;
      return 0;
    }
  };
  
  /** @class Increment
      @brief Increment a variable using the ++ prefix operator
  */
  template<class T> class Increment{
    T& param;
  public:
    Increment(T& t) : param(t) {}
    int operator()(const char*){ ++param; return 0;}
  };

  /** @class Decrement
      @brief Decrement a variable using the -- prefix operator
  */
  template<class T> class Decrement{
    T& param;
  public:
    Decrement(T& t) : param(t) {}
    int operator()(const char*){ --param; return 0;}
  };
  
  /** @class AddVal
      @brief Increment a variable by the amount supplied on the command line
  */
  template<class T> class AddVal{
    T& param;
  public:
    AddVal(T& t) : param(t) {}
    int operator()(const char* val){ 
      std::istringstream s(val);
      T valT;
      if( !(val >> valT) )
	return -1;
      param = param + valT;
      return 0;
    }
  };
  
  /** @class Toggle
      @brief Toggle the value of a boolean variable
  */
  class Toggle{
    bool& param;
  public:
    Toggle(bool& b) : param(b) {}
    int operator()(const char*){ param = !param; return 0; }
  };
  /// @}
}
#endif
