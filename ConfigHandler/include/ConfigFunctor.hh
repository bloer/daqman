//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file ConfigFunctor.hh
    @brief Define the ConfigFunctor template class
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef CONFIGFUNCTOR_h 
#define CONFIGFUNCTOR_h 

/** @class ConfigFunctor
    @brief Treat a functor as a ParameterNode, callable by config file.
    
    This class allows the user to perform operations on a ParameterList-derived
    class more complicated than operations on a single variable, such as 
    resetting lists, adding elements, etc.  
    
    To use, define a functor with an operator() that takes an iostream object.
    If you only want read OR write operations, use the ConfigFunctorDummy* 
    structs.  
    
    This class is used in the ParameterList functions RegisterFunction, 
    RegisterReadFunction, and RegisterWriteFunction

    @ingroup ConfigHandler
*/

#include "ParameterIOimpl.hh"

template <typename readfunc, typename writefunc>
class ConfigFunctor : public VParameterNode{
public:
  ConfigFunctor(const readfunc& r, const writefunc& w, 
		const std::string& key="", const std::string& helptext="") : 
    VParameterNode(key, helptext), reader(r), writer(w) { _node_type=FUNCTION;}
  ~ConfigFunctor(){}
  
  std::istream& ReadFrom(std::istream& in, bool dummy=0){ return reader(in);}
  std::ostream& WriteTo(std::ostream& out, bool dummy1=0, int dummy=0) const
  { return writer(out);}
  
  ConfigFunctor<readfunc, writefunc>* Clone(const void* from, void* to) const
  { return new ConfigFunctor<readfunc, writefunc>(*this); }
private:
  readfunc reader;
  writefunc writer;
};

/// Null-op utility class for ConfigFunctors that don't need read functionality
struct ConfigFunctorDummyRead{
  std::istream& operator()(std::istream& in){ return in; }
};
/// Null-op utility class for ConfigFunctors that don't need write functionality
struct ConfigFunctorDummyWrite{
  std::ostream& operator()(std::ostream& out) const{ return out; }
};

template<class T> struct DeprecatedParameter{
  std::istream& operator()(std::istream& in){ T t; return ParameterIOimpl::read(in,t); }
};

#endif
