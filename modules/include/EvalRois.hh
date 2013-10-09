/** @file EvalRois.hh
    @brief defines the EvalRois module
    @author bloer
    @ingroup modules
*/

#ifndef EVAL_ROIS_h
#define EVAL_ROIS_h

#include "ChannelModule.hh"
#include <vector>

/** @class EvalRois
    @brief Evaluate statistics over defined regions of interest
    
    Regions are defined by a start and end time; these are converted to 
    sample indices.  Any number of regions can be defined.
    
    @ingroup modules
*/
class EvalRois : public ChannelModule{
public:  
  EvalRois();
  ~EvalRois();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "EvalRois"; }
  
  /// Add a new region of interest to evaluate.  Times in microseconds
  void AddRoi(double t_start, double t_end){
    _regions.push_back(std::make_pair(t_start, t_end));
  }
  
  /// Clear the regions already defined
  void ClearRegions(){ _regions.clear(); }
  
  /// Get the number of regions defined so far
  int GetNRegions() const { return _regions.size(); }

private:
  std::vector<std::pair<double, double> > _regions;
  
  
};

#endif
