/** @file runinfo.hh
    @brief Defines the runinfo class that contains run metadata
    @author bloer
**/

#ifndef RUNINFO_h
#define RUNINFO_h

//hide this from ROOT
#ifndef __CINT__
#include "ParameterList.hh"
#endif 


#include <vector>
#include <map>
#include <string>
#include <set>
#include <sstream>
#include <time.h>

/** @class runino
    @brief Contains default and user-specified metadata about each run acquired
*/

#ifndef __CINT__
class runinfo : public ParameterList{
#else
class runinfo{
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
   */
#ifndef __CINT__
  class DialogField : public ParameterList{
#else
    class DialogField{
#endif
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
  
private:
  //these fields allow the user to define additional metadata via config files
  ///Arbitrarty per-run info defined by the user
  stringmap metadata; 
  
  //All the dialog fields refer to entries in the main metadata map
  
  ///List of fields to query the user for at run start
  FieldList prerun_dialog_fields;
  ///List of fields to query the user for at run end
  FieldList postrun_dialog_fields;
  
  ///Per-channel metadata. Note this cannot be listed as required
  std::map<int, stringmap > channel_metadata;
  //(may add a required_channel_metadata in the future)
  
public:
  //all of the following return 0 for success, 1 for user cancelled, <0 on error
  enum FILLTIME {RUNSTART, RUNEND};
  ///Prompt user for any missing/incorrect info. 
  int FillDataForRun(FILLTIME when=RUNSTART, bool forcedialog=false);

  ///Get metadata
  std::string GetMetadata(const std::string& key)
  { stringmap::iterator it = metadata.find(key);
    return it == metadata.end() ? std::string("") : it->second ; }
  ///Explicitly set metadata
  void SetMetadata(const std::string& key, const std::string& val)
  { metadata[key] = val; }

  ///convenience function to set metadata using ostream overload
  template<class T> void SetMetadata(const std::string& key, const T& val){
    std::stringstream s;
    s<<val;
    metadata[key] = s.str();
  }

  
private:

}; 


#endif
