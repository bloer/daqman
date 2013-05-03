#include "CosmicMonitor.hh"
#include "intarray.hh"

#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "RootWriter.hh"
#include <algorithm>
#include <cmath>

CosmicMonitor::CosmicMonitor() : 
  ChannelModule(GetDefaultName(), 
		"processes info on the cosmic ray channel")
{
  AddDependency<BaselineFinder>();
  RegisterParameter("threshold" , threshold = 0, "Cosmic threshold");
}

CosmicMonitor::~CosmicMonitor()
{
  Finalize();
}

int CosmicMonitor::Initialize() 
{ 
  return 0; 
}

int CosmicMonitor::Finalize() { return 0; }

int CosmicMonitor::Process(ChannelData* chdata)
{
  
  const Baseline& baseline = chdata->baseline;
  if(!baseline.found_baseline)
    return 0;
  
  //get the relevant variables
  double* wave = chdata->GetBaselineSubtractedWaveform();
  
  const int nsamps = chdata->nsamps;
  /*
  std::vector<double>& integral = chdata->integral;
  integral.resize(nsamps);
  
  //perform the integration
  integral[0] = wave[0] ;
  */
  
  for(int samp = 1; samp < nsamps; samp++){
    if (wave[samp] > threshold) {  
      //chdata->has_cosmic = true;
      return 0;
    }
  } 
  return 0;
}
