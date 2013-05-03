/** @file Baseline.hh
    @brief Defines the Baseline data class
    @author rsaldanha
    @ingroup modules
    @ingroup daqroot
*/

#ifndef BASELINE_h
#define BASELINE_h

#include "Spe.hh"

/** @class Baseline
    @brief Variables related to the found baseline for a channel
    @ingroup modules
    @ingroup daqroot
*/
class Baseline{
public:
  Baseline(){ Clear(); }
  virtual ~Baseline() {}
  /// Reset all to default
  void Clear();
public:
  bool found_baseline;    ///< was the baseline finder successful?
  double mean;            ///< the average baseline found
  double variance;        ///< variance of the samples in the baseline region
  int search_start_index; ///< start point used to find baseline
  int length;             ///< num of samples over which baseline was averaged
  bool saturated;         ///< Did the baseline hit the max or min y value?
  bool laserskip;         ///< Did the baseline finder begin interpolating inside the baseline region?
  int ninterpolations;    ///< num of interpolations
  std::vector<Spe> interpolations;     ///< interpolation region details saved as Spe objects
  
  ClassDef(Baseline,3)

};

inline void Baseline::Clear()
{
  found_baseline = false;
  mean = -1;
  variance = -1;
  search_start_index = -1;
  length = -1;
  saturated = false;
  laserskip = false;
  ninterpolations = 0;
  interpolations.clear();
}

#endif
