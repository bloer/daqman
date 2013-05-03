/** @file Tof.hh
    @brief Defines the Tof storage class
    @ingroup modules
    @ingroup daqroot
    @author huajiec
*/

#ifndef TOF_h
#define TOF_h

/** @class TOF
    @brief Store information about a single pulse in the root tree
    @ingroup modules
    @ingroup daqroot
*/
class TOF{
public:
  TOF(){ Clear(); }
  virtual ~TOF() {}
  void Clear(){
    found_pulse = false;
    integral = 0;
    start_time = 0;
    amplitude = 0;
    peak_time = 0;
    length = 0;
    constant_fraction_time = 0;
  }

  bool found_pulse; ///< Boolean on if pulse found in the required window
  double integral; ///< Integral in counts*samples of the found pulse
  double start_time; ///< Time of the start of the pulse
  double amplitude;  ///< Maximum height obtained
  double peak_time; ///< Time at which the pulse reached max amplitude
  double length; ///< Time in us over which integral was evaluated
  double constant_fraction_time; ///< Time in us of the constant fraction time, for ref channel, this is the threshold crossing time
  ClassDef(TOF,1)
};

#endif
