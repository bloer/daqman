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
#include "Message.hh"
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
  virtual std::ostream& WriteTo( std::ostream& out , bool, int);
  /// Read unsigned integers from istreams possibly in hex format
  virtual unsigned long ReadUnsignedInt(std::istream& in){
    std::string temp;
    in>>temp;
    if(temp[0] == '0' && (temp[1] == 'x' || temp[1] == 'X'))
      return std::strtoul(temp.c_str(),0,16);
    else
      return std::strtoul(temp.c_str(),0,10);
  }
private:
  T& _val; ///< reference to the wrapped underyling variable
};
  
//template members must go here so they'll actually get compiled
template<class T>
inline std::istream& Parameter<T>::ReadFrom(std::istream& in, bool)
{
  if( !( in >> _val ) && !in.eof()){
    Message e(EXCEPTION);
    e<<"Error trying to read parameter with default key "<<_default_key<<"!\n";
    throw std::invalid_argument(e.str());
  }
  return in;
}

template<class T>
inline std::ostream& Parameter<T>::WriteTo(std::ostream& out, bool, int)
{
  return out<<_val;
}

template<class T>
inline int Parameter<T>::PrintHelp(const std::string& myname) const
{
  VParameterNode::PrintHelp(myname);
  std::cout<<"Parameter type: "<<typeid(_val).name()<<"\n"
	   <<"Current Value:  "<<_val<<std::endl;
  std::string dummy;
  std::cout<<"\nHit <enter> to continue.";
  std::getline(std::cin, dummy);
  std::getline(std::cin, dummy);
  
  return 0;
}

/// Specific ostream overload for booleans
template<> inline std::ostream& Parameter<bool>::WriteTo(std::ostream& out, bool, int)
{
  return out<<std::boolalpha<<_val<<std::noboolalpha;
}

/// specific istream overload for booleans
template<> inline std::istream& Parameter<bool>::ReadFrom(std::istream& in, bool)
{
  std::string temp;
  in>>temp;
  if(temp == "1" || temp == "true" || temp == "TRUE")
    _val = true;
  else if( temp == "0" || temp == "false" || temp == "FALSE" )
    _val = false;
  else{
    Message e(EXCEPTION);
    e<<"Expected boolean value, got "<<temp<<std::endl;
    throw std::invalid_argument(e.str());
  }
  return in;
}

/// Write the 0x prefix on unsigned integers
template<> inline std::ostream& Parameter<unsigned>::WriteTo(std::ostream& out, bool, int)
{
  return out<<std::hex<<std::showbase<<_val<<std::noshowbase<<std::dec;
}

/// Write the 0x prefix on unsigned integers
template<> inline std::ostream& Parameter<unsigned char>::WriteTo(std::ostream& out, bool,int)
{
  return out<<std::hex<<std::showbase<<_val<<std::noshowbase<<std::dec;
}

/// Write the 0x prefix on unsigned integers
template<> inline std::ostream& Parameter<unsigned short>::WriteTo(std::ostream& out, bool, int)
{
  return out<<std::hex<<std::showbase<<_val<<std::noshowbase<<std::dec;
}

/// Write the 0x prefix on unsigned integers
template<> inline std::ostream& Parameter<unsigned long>::WriteTo(std::ostream& out, bool, int)
{
  return out<<std::hex<<std::showbase<<_val<<std::noshowbase<<std::dec;
}

/// Write the 0x prefix on unsigned integers
template<> inline std::ostream& Parameter<unsigned long long>::WriteTo(std::ostream& out, bool, int)
{
  return out<<std::hex<<std::showbase<<_val<<std::noshowbase<<std::dec;
}

/// Read unsigned ints either in decimal or hex format
template<> inline std::istream& Parameter<unsigned>::ReadFrom(std::istream& in, bool)
{
  _val = ReadUnsignedInt(in); return in;
}

/// Read unsigned ints either in decimal or hex format
template<> inline std::istream& Parameter<unsigned char>::ReadFrom(std::istream& in, bool)
{
  _val = ReadUnsignedInt(in); return in;
}

/// Read unsigned ints either in decimal or hex format
template<> inline std::istream& Parameter<unsigned short>::ReadFrom(std::istream& in, bool)
{
  _val = ReadUnsignedInt(in); return in;
}

/// Read unsigned ints either in decimal or hex format
template<> inline std::istream& Parameter<unsigned long>::ReadFrom(std::istream& in, bool)
{
  _val = ReadUnsignedInt(in); return in;
}

/// Read unsigned ints either in decimal or hex format
template<> inline std::istream& Parameter<unsigned long long>::ReadFrom(std::istream& in, bool)
{
  _val = ReadUnsignedInt(in); return in;
}

///Override std::string's to let "" be an empty string
template<> inline std::istream& Parameter<std::string>::ReadFrom(std::istream& in, bool)
{
  std::string temp;
  if(in>>temp){
    if(temp == "\"\"")
      _val = "";
    else 
      _val = temp;
  }
  return in;
}

///Override std::string's to let "" be an empty string
template<> inline std::ostream& Parameter<std::string>::WriteTo(std::ostream& out, bool, int)
{
  if(_val == "")
    return out<<"\"\"";
  return out<<_val;
}
#endif
