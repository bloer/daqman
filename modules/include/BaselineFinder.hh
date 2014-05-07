/** @file BaselineFinder.hh
    @brief Defines BaselineFinder module.
    @author rsaldanha
    @ingroup modules
*/

#ifndef BASELINEFINDER_h
#define BASELINEFINDER_h

#include "ChannelModule.hh"

#include <vector>
/** @class BaselineFinder
    @brief searches the beginning of a channel's waveform to determine baseline
    @ingroup modules
    
    Baseline finder searches for a (possibly) moving baseline DC offset
    in pulse data.  It does this by taking a moving average (pre_samps before
    and post_samps after) for those points where the sample amplitude within
    the full average window is less than max_amplitude (or max_sum_amplitude)
    
    The algorithm goes:
    1) Find the highest sample in the time before signal_begin_time
    2) search for a region which is pre_samps+post_samps long which has no
       sample below the max found in the last step - 2*max_amplitude
       Call this the original baseline.  If no clean region is found before
       signal_begin_time, we fail and abort
    3) Once a first guess at baseline is found, call that moving_base. From then
       on, the baseline is evaluated if all samples in a window are within
       max_amplitude of the last evaluated moving baseline.
    4) For samples of large amplitude, the baseline is linearly interpolated
       from the nearest good points. 
    5) Finally we subtract the moving baseline from the signal and store it
       in the subtracted_waveform
*/
class BaselineFinder : public ChannelModule
{
public:
  BaselineFinder();
  ~BaselineFinder();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "BaselineFinder";}

  //parameters
  bool fixed_baseline;         // use fixed baseline algorithm
  bool linear_interpolation;  // use few baseline estimates and interpolate
  
  ParameterList fixed_params;
  int segment_samps;
  int min_valid_samps;
  double max_sigma;
  double max_sigma_diff;
  double max_mean_diff;

  ParameterList interp_params;
  int avg_samps;
  double max_sigma_factor;
  double pulse_threshold;
  int cooldown;
  int pre_cooldown;

  ParameterList drifting_params;
  double max_amplitude;   ///< max amplitude for sample to be part of baseline
  double max_return_amplitude; ///< max amplitude for resuming baseline following after an excluded region
  double max_sum_amplitude; ///< max_amplitude for sum channel
  double signal_begin_time;  ///< position to start searching for baseline
  int pre_samps;           ///< samples before point to average
  int post_samps;          ///< samples after point to average
  bool save_interpolations;          ///< Save drift baseline interpolation regions as spe
  double laserwindow_begin_time;
  double laserwindow_end_time;
  bool laserwindow_freeze;
  
private:
  int FixedBaseline(ChannelData* chdata);
  int DriftingBaseline(ChannelData* chdata);

};


#endif
