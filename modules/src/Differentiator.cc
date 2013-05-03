#include "Differentiator.hh"
#include "EventHandler.hh"
#include "V172X_Params.hh"
#include "RootWriter.hh"
#include "ConvertData.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"

#include "TMath.h"
#include <vector>

#include "intarray.hh"

Differentiator::Differentiator():
  ChannelModule(GetDefaultName(),
		"Numerically differentiate the channel's waveform")
{
  AddDependency<ConvertData>();
  AddDependency<BaselineFinder>();
  
  //Register all the config handler parameters
  RegisterParameter("decay_constant", decay_constant = 120.00,
		    "Decay time constant in microsec of the integrator");
}

Differentiator::~Differentiator()
{
  Finalize();
}

int Differentiator::Initialize()
{
  return 0;
}

int Differentiator::Process(ChannelData* chdata)
{
  
  const Baseline& baseline = chdata->baseline;
  if(!baseline.found_baseline)
    return 0;
  double* wave = chdata->GetWaveform();
  
  const int& nsamps = chdata->nsamps;
  chdata->derivative.resize(nsamps);
  
  std::vector<double>& derivative = chdata->derivative;
  double sample_interval = 1./(double)chdata->sample_rate;
  derivative[0] = 0;
  for(int index = 1; index < nsamps; index++)
    {
      double prev_value = baseline.mean - wave[index-1];
      double curr_value = baseline.mean - wave[index];
      derivative[index] = (curr_value - 
			   prev_value*TMath::Exp(-sample_interval/decay_constant))*(1 + sample_interval/decay_constant);
    }
  
  
  return 0;
}

int Differentiator::Finalize()
{   
    return 0;
}
