/** @file Spe.hh
    @brief Defines the Spe storage class
    @ingroup modules
    @ingroup daqroot
    @author rsaldanha
*/

#ifndef SPE_h
#define SPE_h
#include "TObject.h"

/** @class Spe
    @brief Store information about a single photoelectron pulse in the root tree
    @ingroup modules
    @ingroup daqroot
*/
class Spe
{
public:
  Spe()
  {
    Clear();
  }
  virtual ~Spe() {}
  void Clear()
  {
    integral = 0;
    start_time = 0;
    amplitude=0;
    peak_time=0;
    local_baseline = 0;
    length = 0;
  }
  
  double integral; ///< Integral in counts*samples of the found charge
  double start_time; ///< time of the start of the pulse
  double amplitude;  ///< Maximum height obtained relative to local baseline
  double peak_time; ///< time at which the pulse reached max amplitude
  double local_baseline; ///< value of the local baseline before the pulse
  double length; ///< Time in us over which integral was evaluated
  ClassDef(Spe,3)
};
  
#endif
