/** @file runinfo.hh
    @brief Defines the runinfo class that contains run metadata
    @author bloer
**/

#ifndef RUNINFO_h
#define RUNINFO_h

//hide this from ROOT
#include "Rtypes.h"
#include "TObject.h"
#ifndef __CINT__
#include "ParameterList.hh"
#endif 


#include <vector>
#include <map>
#include <string>
#include <set>
#include <sstream>
#include <time.h>

class TMacro;

/** @class runinfo
    @brief Contains default and user-specified metadata about each run acquired
*/

#ifndef __CINT__
class runinfo : public ParameterList, public TObject{
#else
class runinfo : public TObject{
#endif
public:
  /// default constructor does nothing
  runinfo(long id=-1);
  /// virtual destructor to make root happy
  virtual ~runinfo() {}
  
  /// Initialize parameters to default values
  void Init(bool reset=false);
  ///resets the io keys after a copy operation
  void InitializeParameterList(); 

  /// Reset the statistics which can be read from the file
  void ResetRunStats();
  
  /// Try to load runinfo from config saved in a TMacro; return 0 if success
  int LoadSavedInfo(TMacro* mac);
  
  /// Merge metadata from another source, optionally overwrite common keys
  void MergeMetadata(const runinfo* other, bool overwritedups = false);
  
  //metadata to save for all runs, determined automatically from daq settings
  
  /* note: By default runs have a prefix (per experiment, default 'rawdaq') 
     and suffix (the date expressed as yymmddHHMM). 
     The suffix is equivalent to the runid in this case. 
     
     Alternatively, if the filename is given as *Run######", then the 6 digit
     suffix will be interpreted as the run id. In this case it is up to the 
     user to ensure unique IDs
  */

  long runid;                  ///< unique id for the run
  time_t starttime;            ///< time acquisition started
  time_t endtime;              ///< time acquisition finished
  long triggers;               ///< total number of triggers requested
  long events;                 ///< total number of events stored
  

public:
  typedef std::set<std::string> stringset;
  typedef std::vector<std::string> stringvec;
  typedef std::map<std::string, std::string> stringmap;
  
  /**@class DialogField
     @brief utility class to handle querying user for metadata
     Completely hide this from root!
   */
#ifndef __CINT__
  class DialogField : public ParameterList{
  public:
    DialogField(const std::string& field_="", const std::string& desc = "",
		bool required_=true, const std::string& default_="");
    virtual ~DialogField() {}
    bool IsValueValid(const std::string& val) const;
    std::string fieldname;
    std::string description;
    std::vector<std::string> allowed_values;
    bool required;
    std::string defaultvalue;
  };
  
  typedef std::vector<DialogField> FieldList;
#endif

public:
  //these fields allow the user to define additional metadata via config files
  ///Arbitrarty per-run info defined by the user
  stringmap metadata; 
  
private:
#ifndef __CINT__
  //All the dialog fields refer to entries in the main metadata map
  ///List of fields to query the user for at run start
  FieldList prerun_dialog_fields;
  ///List of fields to query the user for at run end
  FieldList postrun_dialog_fields;
#endif

  ///Display the prerun dialog even if all fields are already valid
  bool force_prerun_dialog;
  ///Display the postrun dialog even if all fields are already valid
  bool force_postrun_dialog;
    
public:
  ///Per-channel metadata. Note this cannot be listed as required
  std::map<int, stringmap > channel_metadata;
  //(may add a required_channel_metadata in the future)
  
public:
  //all of the following return 0 for success, 1 for user cancelled, <0 on error
  enum FILLTIME {RUNSTART, RUNEND};
  ///Prompt user for any missing/incorrect info. 
  int FillDataForRun(FILLTIME when=RUNSTART);

  ///Get metadata
  std::string GetMetadata(const std::string& key, const std::string& def="")
  { stringmap::iterator it = metadata.find(key);
    return it == metadata.end() ? def : it->second ; }

  ///Get metadata as a double for drawing purposes
  double GetValueMetadata(const std::string& key, double def = 0.)
  {  std::string s = GetMetadata(key,""); 
    return s == "" ? def : atof(s.c_str()); }
  
  ///Get channel's metadata
  std::string GetChannelMetadata(int ch, const std::string& key, 
				 const std::string& def ="")
  {
    if(channel_metadata.count(ch)){
      stringmap& cmeta = channel_metadata[ch];
      stringmap::iterator it = cmeta.find(key);
      return it == cmeta.end() ? def : it->second;
    }
    return def;
  }
  
  ///Get channel's metadata as value
  double GetValueChannelMetadata(int ch, const std::string& key, 
				      double def = 0.)
  {
    std::string s = GetChannelMetadata(ch,key,"");
    return s == "" ? def : atof(s.c_str());
  }
  
  ///Explicitly set metadata
  void SetMetadata(const std::string& key, const std::string& val)
  { metadata[key] = val; }
  
  void SetChannelMetadata(int ch, const std::string& key, 
			  const std::string& val)
  { channel_metadata[ch][key] = val; }

  ///convenience function to set metadata using ostream overload
  template<class T> void SetMetadata(const std::string& key, const T& val){
    std::stringstream s;
    s<<val;
    metadata[key] = s.str();
  }
  
  
private:
  ClassDef(runinfo,4);
}; 


#endif
