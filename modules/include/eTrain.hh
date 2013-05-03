/** @file eTrain.hh
    @brief defines the eTrain storage class
    @ingroup modules
    @ingroup daqroot
    @author hunter
*/

#ifndef ETRAIN_h
#define ETRAIN_h
#include "TObject.h"

/** @class eTrain
    @brief leaves you the worst channel and first/last bad events for grassy runs.
    @ingroup modules
    @ingroup daqroot
*/
class eTrain
{
public:
  eTrain()
  {
    Clear();
  }
  virtual ~eTrain() {}
  void Clear()
  {
    bright_channel=999;
    first_coinc=0;
    last_coinc=0;
  }
  
  int bright_channel; ///< worst channel (most grassy events)
  int first_coinc; ///< first bad event in worst channel
  int last_coinc; ///< last bad event in worst channel
  std::vector<int> coincidences;
  ClassDef(eTrain,1)
};
  
#endif
