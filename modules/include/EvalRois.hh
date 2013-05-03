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
    _start_time.push_back(t_start);
    _end_time.push_back(t_end);
  }
  
  /// Clear the regions already defined
  void ClearRegions(){ _start_time.clear(); _end_time.clear(); }
  
  /// Get the number of regions defined so far
  int GetNRegions() const { return _start_time.size(); }
  /// Get the start times of the defined regions
  const std::vector<double>* GetStartTimes() const { return &_start_time; }
  /// Get the end times of the defined regions
  const std::vector<double>* GetEndTimes() const { return &_end_time; }
private:
  std::vector<double> _start_time;  ///< vector of defined start times
  std::vector<double> _end_time;    ///< vector of defined end times
  
  
};

#endif
