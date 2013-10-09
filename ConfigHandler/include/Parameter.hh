//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file Parameter.hh
    @brief defines the Parameter template class
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef PARAMETER_h
#define PARAMETER_h
#include "VParameterNode.hh"
#include "ParameterIOimpl.hh"
#include "Message.hh"
#include "phrase.hh"

#include <stdexcept>
#include <string>
#include <cstdlib>
#include <typeinfo>
#include <ctime>
#include <cstdio>



/** @class Parameter
    @brief Template implementation of VParamterNode, allows any variable to be 
    set/read via stl iostreams
    
    @ingroup ConfigHandler
*/
template<class T> class Parameter : public VParameterNode{
public:
  /// DefaultConstructor
  Parameter(T& t, const std::string& key="", const std::string& helptext = "") :
    VParameterNode(key, helptext), _val(t) {_node_type = PARAMETER;}
  /// Copy constructor
  Parameter(const Parameter& right) : 
    VParameterNode(right._default_key, right._helptext), _val(right._val) {}
  /// Desctructor
  virtual ~Parameter() {};
  /// Assignment operator
  Parameter& operator=(const Parameter& right){ _val=right._val; return *this;}
  /// Return the underlying variable by reference
  const T& GetValue(){ return _val; }
  /// Print information about this parameter
  virtual int PrintHelp(const std::string& myname="") const;
protected:
  /// Read the underlying variable from an istream
  virtual std::istream& ReadFrom( std::istream& in , bool single=false);
  /// Write the underlying variable to an ostream
  virtual std::ostream& WriteTo( std::ostream& out , bool, int) const;
  
  //implementation of read/write is done with impls so we can overload
  std::istream& read_impl(std::istream& in, T& t){ return in>>t; }
  std::ostream& write_impl(std::ostream& out, T& t){ return out<<t; }
  //specific impl overload
  
private:
  T& _val; ///< reference to the wrapped underyling variable
};

template<class T> 
inline std::istream& Parameter<T>::ReadFrom(std::istream& in, bool)
{
  if( !ParameterIOimpl::read(in, _val) && !in.eof()){
    Message e(EXCEPTION);
    e<<"Error trying to read parameter with default key "<<_default_key<<"!\n";
    throw std::invalid_argument(e.str());
  }
  return in;
}

template<class T> 
inline std::ostream& Parameter<T>::WriteTo(std::ostream& out, bool, int) const
{
  return ParameterIOimpl::write(out, _val);
}

template<class T>
inline int Parameter<T>::PrintHelp(const std::string& myname) const
{
  VParameterNode::PrintHelp(myname);
  std::cout<<"Parameter type: "<<typeid(_val).name()<<"\n"
	   <<"Current Value:  ";
  ParameterIOimpl::write(std::cout, _val);
  std::cout<<std::endl;
  std::string dummy;
  std::cout<<"\nHit <enter> to continue.";
  std::getline(std::cin, dummy);
  std::getline(std::cin, dummy);
  
  return 0;
}


#endif
