/** @file MC_RunInfo.hh
    @brief defines the MC_RunInfo class.
    @author agc
    @ingroup mc
*/

#ifndef MC_RUNINFO_h
#define MC_RUNINFO_h
#include "phrase.hh"

#include "ParameterList.hh"
#include <string>
#include <iostream>
#include <fstream>
#include "stdint.h"

/** @addtogroup mc
    @{
*/

/** @class MC_RunInfo
    @brief parameters relevant the run
*/
class MC_RunInfo : public ParameterList {
public:
  MC_RunInfo();
  ~MC_RunInfo(){};

  /// utility class to verify run type when reading from file
  struct runtype : public std::string {
    runtype& operator=(const std::string& right){assign(right); return *this;}
    virtual ~runtype(){}
  };
  /// utility class to read/write time_t as string
  struct time_param{
    virtual ~time_param(){}
    time_t t;
    time_param& operator=(const time_t& right){ t = right; return *this; }
    operator time_t(){ return t; }
  };

  long runid;           ///< id of the run
  time_param starttime; ///< start timestamp of the run
  time_param endtime;   ///< end timestamp of the run
  long events;          ///< number of events in the tree 
  double livetime;      ///< runtime scaled by the number of accepted triggers
  runtype type;         ///< what class of run it is (laser, background, etc)
  phrase comment;       ///< generic info about the run
  double drift_hv;      ///< setting of the drift_hv field
  double extraction_hv; ///< setting of the extraction_hv field
  double trigger_veto;  ///< length of per-trigger veto in msec, -1 disables
  /** @struct channelinfo
   *  @brief this info is stored for each channel in each run
   */
  class channelinfo : public ParameterList{
  public:
    channelinfo();
    void InitializeParameterList(); 
    int channel;               ///< id number of the channel
    int voltage;               ///< HV power supply voltage
    double amplification;      ///< total amplification before digitizer
    double spe_mean;           ///< single photoelectron mean from database
    double spe_sigma;          ///< std dev of spe response from db
    long calibration_run;      ///< run from which calibration data was loaded
    long calibration_channel;  ///< channel calibration data loaded from
    int cal_filters;           ///< use only calibration runs with n filters
    bool reload_cal;           ///< Use previous calibration or search db?
    bool reload_cal_run;       ///< Search for new optimum calibration run?
  };
  std::vector<channelinfo> channels;  ///< vector of channel info
    
  channelinfo* GetChannelInfo(int channel); ///< Get the info for channel n
    
  class channel_inserter{
    MC_RunInfo* _parent;
  public:
    channel_inserter(MC_RunInfo* parent) : _parent(parent) {}
    static std::string GetFunctionName(){ return "insert_channel"; }
    std::istream& operator()(std::istream& in);
    std::ostream& operator()(std::ostream& out);
    
  };//end channel_inserter utility
    
  class channel_clearer{
    MC_RunInfo* _parent;
  public:
    channel_clearer(MC_RunInfo* parent) : _parent(parent){}
    static std::string GetFunctionName(){ return "clear_channels";}
    std::istream& operator()(std::istream& in);
  };//end channel_clearer utility
};

std::ostream& operator<<(std::ostream& out, MC_RunInfo::time_param& t);
std::istream& operator>>(std::istream& in, MC_RunInfo::time_param& t);

#endif
