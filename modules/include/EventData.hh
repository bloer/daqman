/** @file EventData.hh
    @brief Defines the EventData storage class
    @author bloer
    @ingroup modules
    @ingroup daqroot
*/

#ifndef EVENTDATA_h
#define EVENTDATA_h

#include <vector>
#include <string>
#include <stdint.h>
#include <iomanip>

#include "Rtypes.h" //has the classdef macro
#include "ChannelData.hh"
#include "SumOfIntegral.hh"


//notice: members with comment starting with ! are not saved

/** @class EventData
    @brief Processed data for a trigger which is stored in the ROOT tree
    @ingroup modules
    @ingroup daqroot
*/
class EventData{
  //interface members
public:
  EventData() { Clear(); }
  virtual ~EventData() {} //anything need cleaning up?
  /// Reset all variables to defaults
  void Clear(); //inlined below
  static const char* GetBranchName(){ return "event"; }
  void Print (int verbosity);
public:
  
  enum STATUS_FLAGS { NORMAL=0, ID_MISMATCH=1, /*enter more here*/};
  
  //data members
  int run_id; ///< id of this run
  int event_id; ///< id of this event withing the run
  uint64_t status;             ///< flag denoting event status
  int trigger_count; ///< number of triggers (including unaccepted triggers)
  long timestamp; ///< unix timestamp for the event
  uint64_t dt; ///< time since the last event in ns
  uint64_t event_time; ///< time since run start in ns
  int nchans; ///< physical channels that lit up 
  bool saturated; ///< true if any channel hit the limit of its digitizer range
  bool pulses_aligned; ///< true if the pulse edges on all channels are aligned
  std::vector<double> generic; ///< vector of generic results for testing
  std::vector<ChannelData> channels;  ///< results for each channel
  std::vector<SumOfIntegral> sum_of_int;  ///< vector of sum of integrals for each pulse across all channels
  std::vector<Roi> roi_sum_of_int;       ///< vector of sum of integrals for each region of interest across all channels  
  
  // Energy parameters
  bool s1_valid;  ///< did we find a good s1 pulse?
  bool s1_fixed_valid; ///< is the s1_fixed variable valid?
  bool s2_valid;  ///< did we find a good s2 pulse as well?
  bool s2_fixed_valid; ///< is the s2_fixed variable valid?
  bool s1s2_valid; ///< did we find clean s1 and s2 pulses with nothing else?
  bool s1s2_fixed_valid; ///< are both s1_ and s2_ fixed valid?
  double s1_start_time; ///< start time of s1 tagged pulse in microsec
  double s1_end_time; ///< start time of s1 tagged pulse in microsec
  double s2_start_time; ///< start time of s1 tagged pulse in microsec
  double s2_end_time; ///< start time of s1 tagged pulse in microsec
  
  double drift_time; ///< time between start of s1 and s2 pulses
  
  double s1_full;   ///< s1 integral over the full identified pulse window
  double s2_full;   ///< s2 integral over the full identified pulse window
  double s1_fixed;   ///< s1 integral over a fixed window from pulse start
  double s2_fixed;   ///< s2 integral over a fixed window from pulse start
  double max_s1;    ///< maximum value of s1 in a single channel
  double max_s2;    ///< maximum value of s2 in a single channel
  int max_s1_chan; ///< channel which had the highest s1
  int max_s2_chan; ///< channel which had the highest s2
  double f90_full;  ///< fprompt value at 90 ns compared to the full s1 window
  double f90_fixed; ///< fprompt at 90 ns compared to fixed width s1 
  double gatti; ///< Gatti parameter for first pulse
  double ll_r; ///< Log-likelihood ratio parameter for first pulse
    
  //position  
  bool position_valid; ///< did position recon actually run?
  double x;          ///< reconstructed x position in cm
  double y;          ///< reconstructed y position in cm
  double z;          ///< reconstructed z position in cm
  double bary_valid; ///< was the barycenter calculated ?
  double bary_x;     ///< position of the x barycenter in cm
  double bary_y;     ///< position of the z barycenter in cm
    
  /// Return channel info pointer by id, rather than vector index
  ChannelData* GetChannelByID(int id){
    std::vector<ChannelData>::iterator it = channels.begin();
    while(it != channels.end() && it->channel_id != id) it++;
    return (it == channels.end() ? 0 : &(*it) );
  }
  
  ///Get a pointer to the sum s1 pulse
  Pulse* GetPulse(size_t pulsenum, int channel_id = ChannelData::CH_SUM){ 
    ChannelData* ch = GetChannelByID(channel_id);
    if(ch && ch->pulses.size() > pulsenum)
      return &(ch->pulses[pulsenum]);
    return 0;
  }
  Roi* GetROI(size_t region, int channel_id = ChannelData::CH_SUM){
    ChannelData* ch = GetChannelByID(channel_id);
    if(ch && ch->regions.size() > region)
      return &(ch->regions[region]);
    return 0;
  }
  
  ClassDef(EventData,14)
};



inline void EventData::Clear()
{
  run_id = -1;
  event_id = -1;
  status = NORMAL;
  trigger_count = -1;
  timestamp = 0;
  dt = 0;
  event_time = 0;
  nchans = -1;
  saturated = false;
  pulses_aligned = false;
  generic.clear();
  s1_valid = false;
  s2_valid = false;
  s1s2_valid = false;
  s1_start_time = 0;
  s1_end_time = 0;
  s2_start_time = 0;
  s2_end_time = 0;
  s1_full = 0;
  s2_full = 0;
  s1_fixed = 0;
  s2_fixed = 0;
  max_s1 = 0;
  max_s2 = 0;
  max_s1_chan = -1;
  max_s2_chan = -1;
  f90_full = 0;
  f90_fixed = 0;
  gatti = 0;
  ll_r = 0;
  drift_time = 0;
  position_valid = false;
  x=0; 
  y=0;
  z=0;
  bary_valid = false;
  bary_x = 0;
  bary_y = 0;
  channels.clear();
}
#endif
