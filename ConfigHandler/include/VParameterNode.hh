//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file VParameterNode.hh
    @brief Defines VParameterNode abstract base class
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef VPARAMETERNODE_h
#define VPARAMETERNODE_h
#include  <iostream>

/** @class VParameterNode
    @brief Abstract class with iostream operators to store specialized 
    templates in STL containers.
    
    @ingroup ConfigHandler    
*/
class VParameterNode{
public:
  /// Default Constructor
  VParameterNode(const std::string key="", const std::string helptext="") : 
    _default_key(key), _node_type(VIRTUAL) , _helptext(helptext){}
  /// Destructor
  virtual ~VParameterNode() {}
  
  /// Save this parameter to a file
  virtual bool SaveToFile(const char* fname, bool showhelp=false);
  /// Read this parameter froma file by name <key>
  virtual bool ReadFromFile(const char* fname, const std::string& key="",
			    bool suppress_errs=false);
  /// Read this parameter from an istream
  virtual std::istream& ReadFrom( std::istream& in, bool single=false)=0;
  /// Read this parameter identified by <key> from an istream
  virtual std::istream& ReadFromByKey( std::istream& in, 
				       const std::string& key,
				       bool suppress_errs=false);
  /// Write this parameter to an ostream
  virtual std::ostream& WriteTo( std::ostream& out , bool showhelp=false, 
				 int indent=0)=0;
  /// Get the default name of this parameter
  const std::string& GetDefaultKey() const { return _default_key; }
  /// Set the default name of this parameter
  void SetDefaultKey(const std::string& key){ _default_key = key; }
  /// Determine what type of parameter node this is
  int GetNodeType(){ return _node_type; }
  /// Print information about this parameter, and travel the tree
  virtual int PrintHelp(const std::string& myname="") const;
  
  /// Get the associated help text
  const std::string& GetHelpText(){ return _helptext; }
  /// Set the associated help text
  void SetHelpText(const std::string& newtext){ _helptext = newtext; }

protected:
  /** @enum NODE_TYPES
      @brief Different types of nodes inheriting from VParameterNode
  */
  enum NODE_TYPES {VIRTUAL=0, PARAMETER=1, PARAMETER_LIST=2, FUNCTION=3};
  std::string _default_key;  ///< Default name of this parameter
  int _node_type; ///< what NODE_TYPE is this parameter?
  std::string _helptext; ///< A short description of this parameter
};

//inline the redirect overloads
/// Overload istream operator for VParameterNode
inline std::istream& operator>>(std::istream& in, VParameterNode& p)
{
  return p.ReadFrom(in);
}

/// Overload ostream operator for VParameterNode
inline std::ostream& operator<<(std::ostream& out, VParameterNode& p)
{
  return p.WriteTo(out);
}

#endif
