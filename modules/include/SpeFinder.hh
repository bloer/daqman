/** @file SpeFinder.hh
    @brief Defines the SpeFinder module
    @author rsaldanha
    @ingroup modules
*/

#ifndef SPEFINDER_h
#define SPEFINDER_h

#include "ChannelModule.hh"

/** @class SpeFinder
    @brief Search for single-photoelectron events in the tails of scintillation
    @ingroup modules
*/
class SpeFinder : public ChannelModule
{
public:
  SpeFinder();
  ~SpeFinder();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "SpeFinder";}
  
  //parameters
  double search_start_time; ///< Time from start of pulse to begin search [us]
  double search_end_time;   ///< Time from start of pulse to end search [us]
  double rough_threshold; ///< ADC threshold relative to global baseline
  double threshold_fraction; ///< fraction of threshold for nearby samples
  double return_fraction; ///< fraction of threshold for return to baseline
  double fine_threshold; ///< ADC threshold relative to local baseline
  double pre_window; ///< Time in us to evaluate local baseline before pulse
  double post_window; ///< Time in us after pulse to ensure return to 0
  double pulse_window; ///< Window to integrate around found pulse
  int max_photons; ///< maximum number of photons to find before exiting
  bool debug; ///< debug mode; useful for graphics
};
#endif
