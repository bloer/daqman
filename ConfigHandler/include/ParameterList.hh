//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file ParameterList.hh
    @brief Defines ParameterList class and associated functions
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef PARAMETERLIST_h
#define PARAMETERLIST_h

#include "VParameterNode.hh"
#include "Parameter.hh"
#include "ConfigFunctor.hh"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>
#include "Message.hh"
#include <type_traits>
#include <memory>


/** @class ParameterList
    @brief Concrete implementation of VParameterNode, contains map of sub-params
    @ingroup ConfigHandler
    
    Classes which would like to have sections managed by config files should
    inherit from ParameterList, then use the RegisterParameter function
    to assign string names to the variables which can be config'd. 

    Note that when ParameterList is copied, all of the registered names are 
    lost.  To handle this, derived classes should override the 
    InitializeParameterList virtual method, and place their registration calls
    there. This function is called before both read and write operations if
    the list of subnodes is empty.
*/
class ParameterList : public VParameterNode{
public:
  /// Default Constructor
  ParameterList(const std::string& key="", const std::string& helptext="") : 
    VParameterNode(key, helptext) { _node_type = PARAMETER_LIST; }
  /// Desctructor
  virtual ~ParameterList();
  ///Clone to new ParameterList
  virtual ParameterList* Clone(const void* from, void* to) const
  {return (ParameterList*)((char*)to + ((const char*)this-(const char*)from));}
protected:
  /// Copy constructor can only be used by derived classes
  ParameterList(const ParameterList& right);
  /// Operator= can only be used by derived classes
  ParameterList& operator=(const ParameterList& right);
  /// Util to copy the guts of a parameter list by relative reference
  void CopyPlistRelative(const ParameterList& right);
public:
  /// Initialize the parameter list, needed to preserve copy functionality
  virtual void InitializeParameterList() { std::cerr<<"InitializeParameterList\n";}

  /// Read the whole list from an istream
  virtual std::istream& ReadFrom( std::istream& in, bool single=false);
  /// Write the whole list to an ostream
  virtual std::ostream& WriteTo( std::ostream& out , bool showhelp=false, 
				 int indent=0) const;
  /// Print description, travel through sub-tree
  virtual int PrintHelp(const std::string& myname="") const;
  /// Register a new subparameter
  template<class T> int RegisterParameter(const std::string& key, T& par,
					  const std::string& helptext="");
  /// Register a new function with read and write operators
  template<class R, class W> 
  int RegisterFunction(const std::string& key,
		       const R& read, 
		       const W& write,
		       const std::string& helptext="");
  /// Register a new function that only works when reading
  template<class R> int RegisterReadFunction(const std::string& key, 
					     const R& read,
					     const std::string& helptext="");
  /// Register a new function that only works when writing
  template<class W> int RegisterWriteFunction(const std::string& key, 
					      const W& write,
					      const std::string& helptext="");
  /// Get a subparameter by name
  const VParameterNode* GetParameter(const std::string& key) const;
  VParameterNode* GetParameter(const std::string& key);
protected:
  /// hide disabled nodes on ostream output
  static bool _collapse_disabled;
  
  /// map of subparameters and their names
  typedef std::map<std::string, VParameterNode*> ParMap;
  ParMap _parameters;   ///< all registered subparameters
  
  /// Safely deletes subparams
  std::vector<std::shared_ptr<VParameterNode> > _deleter; 
  
  /// Actual implementation to register a variable as a sub parameter
  template<class T> 
  int RegisterParameterImp(const std::string& key, T& par, 
			   const std::string& helptext,
			   const std::false_type&);
  /// Register a whole ParameterList as a sub parameter
  template<class T>
  int RegisterParameterImp(const std::string& key, T& par, 
			   const std::string& helptext, 
			   const std::true_type&);
};

template<class T>
inline int ParameterList::RegisterParameter(const std::string& key,
					    T& par,
					    const std::string& helptext)
{
  if( _parameters.find(key) != _parameters.end() ){
    Message(ERROR)<<"Parameter with key \""<<key
	     <<"\" already exists in this list!"<<std::endl;
    return -1;
  }
  
  return RegisterParameterImp(key,par,helptext,
			      std::is_base_of<VParameterNode,T>());
}

//template specialization
template<class T>
inline int ParameterList::RegisterParameterImp(const std::string& key, 
					       T& par,
					       const std::string& helptext,
					       const std::true_type&)
{
  _parameters.insert(std::make_pair(key,&par));
  return 0;
}

template<class T> 
inline int ParameterList::RegisterParameterImp(const std::string& key,
					       T& par,
					       const std::string& helptext,
					       const std::false_type&)
{
  std::shared_ptr<VParameterNode> ptr(new Parameter<T>(par, key, helptext) );
  _deleter.push_back(ptr);
  _parameters.insert(std::make_pair(key,ptr.get()));
  return 0;
}

template<class R, class W>
inline int ParameterList::RegisterFunction(const std::string& key,
					   const R& reader, const W& writer,
					   const std::string& helptext)
{
  if( _parameters.find(key) != _parameters.end() ){
    Message(ERROR)<<"Parameter with key \""<<key
	     <<"\" already exists in this list!"<<std::endl;
    return -1;
  }
  
  std::shared_ptr<VParameterNode> 
    ptr(new ConfigFunctor<R,W>(reader, writer, key, helptext));
  _deleter.push_back(ptr);
  _parameters.insert(std::make_pair(key, ptr.get()));
  return 0;
}

template<class R>
inline int ParameterList::RegisterReadFunction(const std::string& key,
					       const R& reader,
					       const std::string& helptext)
{
  int r = RegisterFunction(key, reader, ConfigFunctorDummyWrite(), helptext); 
  if(r==0)
    _parameters[key]->haswrite = false;
  return r;
}

template<class W>
inline int ParameterList::RegisterWriteFunction(const std::string& key,
						const W& writer,
						const std::string& helptext)
{
  int r = RegisterFunction(key, ConfigFunctorDummyRead(), writer, helptext);
  if(r == 0)
    _parameters[key]->hasread = false;
  return r;
}

#endif
