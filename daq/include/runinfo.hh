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

#include "Rtypes.h"
#include <vector>
#include <map>
#include <string>
#include <set>
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
  double pre_trigger_time_us;  ///< length of pre trigger window
  double post_trigger_time_us; ///< length of post trigger window
  
  // the comment field is treated specially from other metadata
  std::string comment;         ///< general comment about run conditions
  std::string post_comment;    ///< Additional comment provided after the run
  
  typedef std::set<std::string> stringset;
  typedef std::map<std::string, std::string> stringmap;
  typedef std::map<std::string, stringset> stringsetmap;
  
  //these fields allow the user to define additional metadata via config files
  ///Arbitrarty per-run info defined by the user
  stringmap metadata; 
  
  ///List of userinfo fields that will be prompted at run start if not defined
  stringset required_metadata; 
  
  ///List of allowed options for specific fields. E.g., runtype may have 
  /// allowed options 'Background', 'Calibration', and 'Normal'
  stringsetmap field_allowed_values;
  
  ///Per-channel metadata. Note this cannot be listed as required
  std::map<int, stringmap > channel_metadata;
  //(may add a required_channel_metadata in the future)
  
private:
  ///See if this value is in the field's allowed set (if defined)
  bool CheckFieldValue(const std::string& fieldname, const std::string& val);
  
  //make visible to ROOT
  ClassDef(runinfo,1)
}; 


#endif
