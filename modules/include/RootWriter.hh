/** @file RootWriter.hh
    @brief Defines the RootWriter module
    @author bloer
    @ingroup modules
*/

#ifndef ROOTWRITER_h
#define ROOTWRITER_h

#include "BaseModule.hh"
#include <iostream>
#include <string>

//forward declarations
class TFile;
class TTree;
class RootWriter;
class runinfo;

//utility classes
/** @class BranchEnabler
    @brief Utility functor to allow enabling of a branch from the config file
*/
class BranchEnabler{
public:
  RootWriter* writer;
  BranchEnabler(RootWriter* w) : writer(w) {}
};

/** @class BranchDisabler
    @brief Utility functor to allow disabling of a branch from the config file
*/
class BranchDisabler{
public:
  RootWriter* writer;
  BranchDisabler(RootWriter* w) : writer(w) {}
};

/** @class RootWriter
    @brief Store processed EventData objects for each trigger into a ROOT tree
    @ingroup modules
*/
class RootWriter : public BaseModule{
public:
  static const std::string GetDefaultName(){ return "RootWriter"; }
  RootWriter();
  ~RootWriter();
  
  /// Get the output tree
  TTree* GetTree(){ return _tree; }
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  
  /// Get the output ROOT filename
  const std::string GetFilename(){ return _filename; }
  /// Set the output ROOT filename
  void SetFilename(const std::string& name){ _filename=name; }
  /// Get the default ROOT output filename
  static const std::string GetDefaultFilename(){ return "out.root"; }
  
  /// Enable a given branch in the stored tree
  void EnableBranch(const char* classname, const char* branchname,
		    bool enable=true);
  /// Disable a given branch in the stored tree
  void DisableBranch(const char* classname, const char* branchname);
  
  /// Construct a friend tree with metadata info
  TTree* BuildMetadataTree(runinfo* info);
private:
  void SaveConfig();
  std::string _filename;
  std::string _directory;
  std::string _mode;
  TFile* _outfile;
  TTree* _tree;
  bool default_saveall;
  BranchEnabler enabler;
  BranchDisabler disabler;
};

/// Overload istream to call BranchEnabler from config file
inline std::istream& operator>>(std::istream& in, BranchEnabler& e)
{
  std::string dummy;
  in>>dummy;
  size_t pos = dummy.find('.');
  if(pos == std::string::npos){
    Message(ERROR)<<"Branches to be enabled should follow the syntax CLASS.BRANCH.\n";
    return in;
  }
  e.writer->EnableBranch(dummy.substr(0,pos).c_str(), 
			  dummy.substr(pos+1).c_str());
  return in;
}

/// Overload istream to call BranchDisabler from config file
inline std::istream& operator>>(std::istream& in, BranchDisabler& d)
{
  std::string dummy;
  in>>dummy;
  size_t pos = dummy.find('.');
  if(pos == std::string::npos){
    Message(ERROR)<<"Branches to be disabled should follow the syntax CLASS.BRANCH.\n";
    return in;
  }
  d.writer->DisableBranch(dummy.substr(0,pos).c_str(), 
			  dummy.substr(pos+1).c_str());
  return in;
}

/// Don't print anything for BranchEnabler
inline std::ostream& operator<<(std::ostream& out, const BranchEnabler& )
{ return out; }
/// Don't print anything for BranchDisabler
inline std::ostream& operator<<(std::ostream&out, const BranchDisabler& )
{ return out; }

#endif
