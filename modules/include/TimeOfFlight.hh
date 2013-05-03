/** @file TimeOfFlight.hh
    @brief defines the TimeOfFlight module
    @author huajiec
    @ingroup modules
*/

#ifndef TIMEOFFLIGHT_h
#define TIMEOFFLIGHT_h

#include "ChannelModule.hh"
/** @class TimeOfFlight
    @brief TimeOfFlight finds the number of particles detected in the defined region of the channel and the arrive time of the earliest particle detected.
    @ingroup modules
*/
class TimeOfFlight : public ChannelModule{
public:
  TimeOfFlight();
  ~TimeOfFlight();
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "TimeOfFlight"; }
private:
  double search_begin_time;///< time in us to start searching for particle signal
  double search_end_time;///< time in us to end searching for particle signal
  double ref_threshold; ///< reference trigger threshold
  double threshold; ///< particle count threshold
  int ref_ch; ///< proton beam timing info channel
  double constant_fraction; ///< constant fraction of the maximum for locating the particle arrival time
};

#endif
