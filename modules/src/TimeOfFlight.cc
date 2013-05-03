#include "TimeOfFlight.hh"
#include "BaselineFinder.hh"
#include "Integrator.hh"
#include <algorithm>
#include <cmath>

TimeOfFlight::TimeOfFlight() : ChannelModule(GetDefaultName(), "finds the number of particles detected in the channel and the arrive time of the earliest particle detected"){
  AddDependency<BaselineFinder>();
  AddDependency<Integrator>();
  RegisterParameter("search_begin_time" , search_begin_time = -0.5, "time in us to start searching for particle signal");
  RegisterParameter("search_end_time" , search_end_time = 0.5, "time in us to end searching for particle signal");
  RegisterParameter("ref_threshold" , ref_threshold = 2500, "reference trigger threshold");
  RegisterParameter("threshold" , threshold = 20, "particle count threshold");
  RegisterParameter("ref_ch" , ref_ch = 2, "proton beam timing info channel");
  RegisterParameter("constant_fraction", constant_fraction = 0.5, "constant fraction of the maximum for locating the particle arrival time");
}

TimeOfFlight::~TimeOfFlight()
{
  Finalize();
}

int TimeOfFlight::Initialize() 
{ 
  return 0; 
}

int TimeOfFlight::Finalize() { return 0; }

int TimeOfFlight::Process(ChannelData* chdata)
{  
  TOF& tof = chdata->tof;
  int start = chdata->TimeToSample(search_begin_time, true);
  // reference channel, SCENE proton beam trigger
  if (chdata->channel_id == ref_ch){
    const double* ref_wave = chdata->GetWaveform();
    // Move start index to region below ref_threshold
    while (ref_wave[start] > ref_threshold){
      start--; 
    }
    // Find the index when ref_threshold is crossed
    while(ref_wave[start] < ref_threshold && start > 0){
      start--;
    }
    tof.found_pulse = true;
    tof.constant_fraction_time = 1/chdata->sample_rate * (ref_threshold - ref_wave[start]) / (ref_wave[start+1] - ref_wave[start]) + chdata->SampleToTime(start);
    //Message(INFO)<< chdata->channel_id << ", " << tof.constant_fraction_time << "\n";
    return 0;
  }
  
  // detector channel
  std::vector<Spe>& interpolations = chdata->baseline.interpolations;
  int ninterpolations = chdata->baseline.ninterpolations;
  
  for (int i=0; i<ninterpolations; i++){
    Spe pulse = interpolations[i];
    if(pulse.start_time>search_begin_time && pulse.start_time<search_end_time && pulse.amplitude>threshold){
      tof.found_pulse = true;
      tof.integral = pulse.integral;
      tof.start_time = pulse.start_time;
      tof.amplitude = pulse.amplitude;
      tof.peak_time = pulse.peak_time;
      tof.length = pulse.length;
      double cf_threshold = constant_fraction * tof.amplitude;
      const double* wave = chdata->GetBaselineSubtractedWaveform();
      int j=chdata->TimeToSample(tof.peak_time)-1; 
      while(j>chdata->TimeToSample(tof.start_time) && -wave[j]>cf_threshold){
	j--;
      }
      tof.constant_fraction_time = 1/chdata->sample_rate * (cf_threshold + wave[j]) / (wave[j] - wave[j+1]) + chdata->SampleToTime(j);
      //Message(INFO)<< chdata->channel_id << ", " << tof.peak_time << ", " << tof.length << ", " << tof.constant_fraction_time << "\n";
      return 0;
    }
  }
  return 0;
}
