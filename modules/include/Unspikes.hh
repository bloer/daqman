/** @file Unspikes.hh
    @brief Defines the Unspikes storage class
    @ingroup modules
    @ingroup daqroot
    @author hunter
*/

#ifndef UNSPIKES_h
#define UNSPIKES_h
#include "TObject.h"

/** @class Unspikes
    @brief Store number of rising edges found by eTrainFinder
    @ingroup modules
    @ingroup daqroot
*/
class Unspikes
{
public:
  Unspikes()
  {
    Clear();
  }
  virtual ~Unspikes() {}
  void Clear()
  {
    nbad=0;
  }
  
  int nbad; ///< number of spikes found where they don't belong
  ClassDef(Unspikes,1)
};
  
#endif
