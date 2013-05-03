/** @file Pulse.hh
    @brief Defines the Pulse data class
    @author bloer, rsaldanha
    @ingroup daqroot
    @ingroup modules
*/

#ifndef PULSE_h
#define PULSE_h

#include "PulseFit.hh"
#include "Message.hh"
#include <vector>
#include <iostream>

/** @class Pulse
    @brief Stores information relevant to a single scintillation event
    @ingroup modules
    @ingroup daqroot
*/
class Pulse{
public:
  Pulse() { Clear(); }
  virtual ~Pulse() {  }
  void Clear();
  void Print(int channel, int pulse);
public:
  bool found_start;    ///< did we find a good start index?
  bool found_end;      ///< did we find a good end index?
  bool found_peak;     ///< did we find a good peak?
  bool peak_saturated; ///< was the peak saturated?
  int start_index;     ///< index marking the start of the pulse
  double start_time;   ///< time since trigger at start of pulse
  int end_index;       ///< index for the end of the pulse
  double end_time;     ///< time since trigger at end of pulse
  int peak_index;      ///< index for the pulse peak
  double peak_time;    ///< time since trigger at pulse peak;
  double peak_amplitude; ///< amplitude of the peak
  double integral; ///< integral of the peak
  double npe;      ///< integral scaled for single pe amplitude
  std::vector <double> f_param; ///< f-parameters for different time values
  double f90; ///< f-parameter for 90 ns
  double t05; ///< time to reach 5% of total integral
  double t10; ///< time to reach 10% of total integral
  double t90; ///< time to reach 90% of total integral
  double t95; ///< time to reach 95% of total integral
  double fixed_int1;  ///< integral evaluated at fixed_time1
  double fixed_int2;  ///< integral evaluated at fixed_time2
  bool fixed_int1_valid; ///< did the event extend at least past fixed_time1?
  bool fixed_int2_valid; ///< did the event extend at least past fixed_time2?
  PulseFit fit; ///< fit to the pulse

  bool is_s1;          ///< true = s1; false = s2
  double dt;           ///< Time between the start of this pulse and the next one (or end of trigger)
  bool start_clean;    ///< start of pulse does not overlap with previous 
  bool end_clean;      ///< end of pulse does not overlap with next or window
  bool is_clean;       ///< clean = this pulse is not back to back with another, not cut off by the end of the scan, not an s2 trigger 
  
  double ratio1;       ///< Pulse tagging development param, see Pulsefinder
  double ratio2;       ///< Pulse tagging development param, see Pulsefinder
  double ratio3;       ///< Pulse tagging development param, see Pulsefinder
  
  double gatti;        ///< Gatti parameter for electron and nuclear recoil templates
  double ll_ele;        ///< Log-likelihood for electron recoil template
  double ll_nuc;        ///< Log-likelihood for nuclear recoil template
  double ll_r;        ///< Log-likelihood ratio of nuclear to electron recoil template
  double pulse_shape_int;   ///<Integral of pulse over reference shape - used to normalize
  
  ClassDef(Pulse,7);
};

inline void Pulse::Clear()
{
  found_start = false;
  found_end = false;
  found_peak = false;
  peak_saturated = false;
  start_index = -1;
  start_time = 0;
  end_index = -1;
  end_time = 0;
  peak_index = -1;
  peak_time = 0;
  peak_amplitude = -1;
  integral=0;
  npe=0;
  f_param.clear();
  f90 = -1;
  t05 = 0;
  t10 = 0;
  t90 = 0;
  t95 = 0;
  gatti = 0;
  ll_ele = 0;
  ll_nuc = 0;
  ll_r = 0;
  fixed_int1 = 0;
  fixed_int2 = 0;
  fixed_int1_valid = false;
  fixed_int2_valid = false;
  pulse_shape_int = 0;
  ratio1 = 0;
  ratio2 = 0;
  ratio3 = 0;
  dt = -1;
  is_s1 = false;
  start_clean = false;
  end_clean = false;
  is_clean = false;
};

inline void Pulse::Print(int channel, int pulse)
{
    Message m(INFO);
    m<<std::endl;
    m<<"************************************************************************"<<std::endl;
    m<<"******************* CHANNEL "<<channel<<" PULSE "<<pulse<<"  INFORMATION *******************"<<std::endl;
    m<<"************************************************************************"<<std::endl;
    m<<"Peak Saturated: "<<peak_saturated<<std::endl;
    m<<"Start Index: "<<start_index<<std::endl;
    m<<"Start Time: "<<start_time<<std::endl;
    m<<"End Index: "<<end_index<<std::endl;
    m<<"End Time: "<<end_time<<std::endl;
    m<<"DT: "<<dt<<std::endl;
    m<<"Peak Index: "<<peak_index<<std::endl;
    m<<"Peak Time: "<<peak_time<<std::endl;
    m<<"Peak Amplitude: "<<peak_amplitude<<std::endl;
    m<<"Integral: "<<integral<<std::endl;
    m<<"Npe: "<<npe<<std::endl;
    m<<"T05: "<<t05<<std::endl;
    m<<"T10: "<<t10<<std::endl;
    m<<"T90: "<<t90<<std::endl;
    m<<"T95: "<<t95<<std::endl;
    m<<"F90: "<<f90<<std::endl;
    m<<"Gatti: "<<gatti<<std::endl;
    m<<"LL Electron: "<<ll_ele<<std::endl;
    m<<"LL Nuclear: "<<ll_nuc<<std::endl;
    m<<"LL Ratio: "<<ll_r<<std::endl;
    m<<"Pulse Shape Integral: "<<pulse_shape_int<<std::endl;
    m<<"Fixed Integral 1 Valid: "<<fixed_int1_valid<<std::endl;
    m<<"Fixed Integral 1 : "<<fixed_int1<<std::endl;
    m<<"Fixed Integral 2 Valid: "<<fixed_int2_valid<<std::endl;
    m<<"Fixed Integral 2 : "<<fixed_int2<<std::endl;
    m<<"Is S1: "<<is_s1<<std::endl;
    m<<"Start Clean: "<<start_clean<<std::endl;
    m<<"End Clean: "<<end_clean<<std::endl;
    m<<"Is Clean: "<<is_clean<<std::endl;
    m<<"************************************************************************"<<std::endl;
}
#endif
