#include "Integrator.hh"
#include "intarray.hh"

#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "RootWriter.hh"
#include <algorithm>
#include <cmath>

Integrator::Integrator() : 
  ChannelModule(GetDefaultName(), 
		"Numerically integrate each channel's waveform")
{
  AddDependency<BaselineFinder>();
  RegisterParameter("threshold" , threshold = 0,
		    "Assume samples less than threshold away from baseline are zero");
}

Integrator::~Integrator()
{
  Finalize();
}

int Integrator::Initialize() 
{ 
  return 0; 
}

int Integrator::Finalize() { return 0; }

int Integrator::Process(ChannelData* chdata)
{
  Baseline& baseline = chdata->baseline;
  if(!baseline.found_baseline)
    return 0;
  
  //get the relevant variables
  const double* wave = chdata->GetBaselineSubtractedWaveform();
  
  const int nsamps = chdata->nsamps;
  std::vector<double>& integral = chdata->integral;
  integral.resize(nsamps);
  
  //perform the integration
  integral[0] = wave[0] ;
  for(int samp = 1; samp < nsamps; samp++){
    double onestep = wave[samp] ;
    integral[samp] = integral[samp-1]
      + ( std::abs(onestep) > threshold ? onestep : 0 );
  }
  
  //find the min/max
  chdata->integral_max_index = std::max_element(integral.begin(), 
						integral.end())
    - integral.begin();
  chdata->integral_min_index = std::min_element(integral.begin(),
						integral.end())
    - integral.begin();
  chdata->integral_max = integral[chdata->integral_max_index];
  chdata->integral_min = integral[chdata->integral_min_index];
  chdata->integral_max_time = chdata->SampleToTime(chdata->integral_max_index);
  chdata->integral_min_time = chdata->SampleToTime(chdata->integral_min_index);

  //baseline interpolation integral
  for(int i=0; i<(int)baseline.interpolations.size(); i++){
    Spe* pe = &baseline.interpolations[i];
    pe->integral = integral[chdata->TimeToSample(pe->start_time)]-integral[chdata->TimeToSample(pe->start_time+pe->length)];
  }
  return 0;
}
