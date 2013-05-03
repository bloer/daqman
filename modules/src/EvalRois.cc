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

const std::string helper_string = "add_roi";

class AddRegionToList{
  EvalRois* _boss;
public:
  AddRegionToList(EvalRois* boss) : _boss(boss) {}
  std::istream& operator()(std::istream& in){
    double a,b;
    if(in>>a>>b)
      _boss->AddRoi(a,b);
    return in;
  }
  std::ostream& operator()(std::ostream& out){
    const std::vector<double>* start = _boss->GetStartTimes();
    const std::vector<double>* end = _boss->GetEndTimes();
    for(size_t i = 0; i < start->size(); i++){
      out<<helper_string<<" "<<start->at(i)<<" "<<end->at(i)<<" ";
    }
    return out;
  }
};

class ClearRegionList{
  EvalRois* _boss;
public:
  ClearRegionList(EvalRois* boss) : _boss(boss) {}
  std::istream& operator()(std::istream& in){
    _boss->ClearRegions();
    return in;
  }
};


EvalRois::EvalRois() : 
  ChannelModule(GetDefaultName(),
		"Measure the max, min, and integral of samples over a set of regions of interest defined by start and end times in microseconds")
{
  AddDependency<ConvertData>();
  AddDependency<BaselineFinder>();
  RegisterFunction(helper_string, 
		   AddRegionToList(this), AddRegionToList(this),
		   "Add a new region to the list of regions of interest");  
  RegisterReadFunction("clear_rois", ClearRegionList(this), 
		       "Clear the list of defined regions of interest");
}

EvalRois::~EvalRois() {}

int EvalRois::Initialize()
{
  if(_start_time.size() > 0){
    Message(DEBUG)<<"Saving info for "<<_start_time.size()<<" regions.\n";
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
  RunDB::runinfo* info = EventHandler::GetInstance()->GetRunInfo();
  for(size_t window = 0; window < _start_time.size(); window++){
    chdata->regions.push_back(Roi());
    Roi& roi = chdata->regions.back();
    roi.start_time = _start_time[window];
    roi.end_time = _end_time[window];
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
    RunDB::runinfo::channelinfo* chinfo = 
      info->GetChannelInfo(chdata->channel_id);
    if(chinfo)
      roi.npe = -roi.integral / chinfo->spe_mean;
    else 
      roi.npe = -roi.integral;
  }
  
  return 0;
}
     
