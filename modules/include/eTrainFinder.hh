/** @file eTrainFinder.hh
    @brief Defines the eTrainFinder module
    @author hunter
    @ingroup modules
*/

#ifndef ETRAINFINDER_h
#define ETRAINFINDER_h

#include "ChannelModule.hh"
#include "eTrain.hh"
#include "Unspikes.hh"

/** @class eTrainFinder
    @brief Check for pulses in the pre-S1 window
    @ingroup modules
*/
class eTrainFinder : public ChannelModule
{
public:
  eTrainFinder();
  ~eTrainFinder();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "eTrainFinder";}
  
  //parameters
  double search_start_time; ///< Time from start of pulse to begin search [us]
  double search_stop_time;   ///< Time from start of pulse to end search [us]
  double rough_threshold; ///< ADC threshold relative to global baseline
  int distance; ///minimum separation in samples of distinct spikes
  int coinc_dist; ///maximum number of events between two events that constitutes a coincidence
  int eMin; ///minimum number of spikes for a bad event
  int eMax; ///maximum number of spikes for a bad event

private:
  int _n;
  std::vector<int> _nbad;
  std::vector<int> _coinc_track;
  std::vector<int> _coinc;
  std::vector<int> _first;
  std::vector<int> _last;
};
#endif
