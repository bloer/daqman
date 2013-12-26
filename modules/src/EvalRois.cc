#include "EvalRois.hh"
#include "BaselineFinder.hh"
#include "ConvertData.hh"
#include "SumChannels.hh"
#include "RootWriter.hh"
#include "intarray.hh"
#include "ChannelData.hh"
#include "Integrator.hh"
#include "Roi.hh"
#include "EventHandler.hh"
#include <algorithm>
#include <numeric>


EvalRois::EvalRois() : 
  ChannelModule(GetDefaultName(),
		"Measure the max, min, and integral of samples over a set of regions of interest defined by start and end times in microseconds")
{
  AddDependency<ConvertData>();
  AddDependency<BaselineFinder>();
  RegisterParameter("regions", _regions, "Start/end time pairs to evaluate");
}

EvalRois::~EvalRois() {}

int EvalRois::Initialize()
{
  if(_regions.size() > 0){
    Message(DEBUG)<<"Saving info for "<<_regions.size()<<" regions.\n";
  }
  else{
    Message(ERROR)<<"No ROIs defined for EvalRois!\n";
    return 1;
  }  
  return 0;
}

int EvalRois::Finalize() { return 0; }

int EvalRois::Process(ChannelData* chdata)
{
  
  Baseline& base = chdata->baseline;
  if(!base.found_baseline) return 0;
  for(size_t window = 0; window < _regions.size(); window++){
    chdata->regions.push_back(Roi());
    Roi& roi = chdata->regions.back();
    roi.start_time = _regions[window].first;
    roi.end_time = _regions[window].second;
    roi.start_index = (int)std::max(roi.start_time * chdata->sample_rate + 
				    chdata->trigger_index , 0.);
    roi.end_index = (int)std::max(roi.end_time * chdata->sample_rate + 
				  chdata->trigger_index, 0.);
    roi.start_index = std::min(roi.start_index, chdata->nsamps);
    roi.end_index = std::min(roi.end_index, chdata->nsamps);
    
    //double* wave = chdata->GetWaveform();
    double* subtractedwave = chdata->GetBaselineSubtractedWaveform();
    double* min_iter = std::min_element(subtractedwave+roi.start_index, subtractedwave+roi.end_index);
    double* max_iter = std::max_element(subtractedwave+roi.start_index, subtractedwave+roi.end_index);
    roi.max = *(max_iter);
    roi.min = *(min_iter);
    roi.min_index = min_iter-subtractedwave;
    
    if(! chdata->integral.empty()){
      roi.integral = chdata->integral[roi.end_index] - 
	chdata->integral[roi.start_index];
    }
    else{
      
      roi.integral = std::accumulate(subtractedwave+roi.start_index,
				     subtractedwave+roi.end_index,
				     0.);
    }
    roi.npe = -roi.integral / chdata->spe_mean;
  }
  
  return 0;
}
     
