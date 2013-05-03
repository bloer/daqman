/** @file PulseFinder.hh
    @brief Defines the PulseFinder module
    @author rsaldanha, bloer
    @ingroup modules
*/
#ifndef PULSEFINDER_h
#define PULSEFINDER_h

#include "BaseModule.hh"

/** @class PulseFinder
    @brief Searches for individual scintillation events within a trigger
    @ingroup modules
*/
class PulseFinder : public BaseModule{
public:
  PulseFinder();
  ~PulseFinder();
   
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  
  /// Evaluate times, integral, fparams for pulse
  int EvaluatePulse(Pulse& pulse, ChannelData* chdata,
		    int start_index, int end_index) const;
  
  /// Search for pulses using a simple discrimination threshold
  void DiscriminatorSearch(ChannelData* chdata,
			   std::vector<int>& start_index, 
			   std::vector<int>& end_index);
  
  /// Search for pulses based on the measured variance of the baseline
  void VarianceSearch(ChannelData* chdata,
		      std::vector<int>& start_index, 
		      std::vector<int>& end_index);
  
  void IntegralSearch(ChannelData* chdata,
		      std::vector<int>& start_index,
		      std::vector<int>& end_index);
  
  /// Search for pulses based on the curvature of the integral
  void CurvatureSearch(ChannelData* chdata,
		       std::vector<int>& start_index,
		       std::vector<int>& end_index);
  
  static const std::string GetDefaultName(){ return "PulseFinder"; }
  
  enum SEARCH_MODE { VARIANCE , DISCRIMINATOR , INTEGRAL , CURVATURE };

private:
  //parameters
  bool align_pulses;               ///< Single set of pulse edges for all channels?
  SEARCH_MODE mode;                ///< Which search function to use
  bool normalize;                  ///< Scale all searches by spemean?
  int start_window;                ///< sample window to start looking in
  double min_start_variance;       ///< min variance to define a start
  int min_resolution;              ///< What is this?
  int min_start_peak_sep;          ///< min samples peaks must be separated by
  bool discriminator_relative;     ///< is disc value relative to baseline?
  double discriminator_value;      ///< discriminator treshold value in counts
  int discriminator_start_add;     ///< n samples to add before start
  int discriminator_end_add;       ///< n samples to add after end
  double integral_start_time;      ///< time in us over which photons arrive
  double integral_end_time;        ///< time at end of pulse to be below thresh
  double integral_start_threshold; ///< amount in npe integral must fall
  double integral_end_threshold;   ///< end when npe integral below thresh
  double min_sep_time;             ///< minimum time two pulses must be apart
  double multipulse_thresh_value;  ///< secondary must be > this*prev integral
  double amplitude_start_threshold;///< before broad integral search, look for signal above this threshold
  double amplitude_end_threshold;  ///< signal must fall below this threshold to end pulse
  double min_pulse_time;           ///< minimum length of pulse before ends naturally
  double lookback_time;            ///< samples to look back for pileup
  ///Get the fractional of photoelectrons expected to arrive between t0 and t

  //parameters for curvature search
  int down_sample_factor;          ///< reduce the integral vector size by this factor 
  double pulse_start_curvature;    ///< curvature threshold to start a new pulse  
  int pile_up_curvature;           ///< curvature threshold to start a pile up pulse
  double pulse_end_slope;          ///< slope threshold to end a pulse
  double fixed_time1;      ///< fixed time1 at which to evaluate pulse integral
  double fixed_time2;      ///< fixed time2 at which to evaluate pulse integral
  
  double PredictedNPE(double t0, double t, double fprompt, 
		      double tau1, double tau2)
  {
    return fprompt*(1.-exp(-(t-t0)/tau1)) + (1-fprompt)*(1.-exp(-(t-t0)/tau2));
  }

};

//override stream ops for SearchMode
std::istream& operator>>(std::istream& in, PulseFinder::SEARCH_MODE& m);
std::ostream& operator<<(std::ostream& out, PulseFinder::SEARCH_MODE& m);

#endif
