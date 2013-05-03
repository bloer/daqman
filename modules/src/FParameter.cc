#include "FParameter.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "ConvertData.hh"
#include "PulseFinder.hh"
#include "intarray.hh"

#include <vector>

FParameter::FParameter():
  ChannelModule(GetDefaultName(), "Calculate fprompt, the fraction of photons which arrive in the early time of a scintillation pulse")
{
  AddDependency<PulseFinder>();

  ///@todo provide helptext for FParameter
 //Register all the config handler parameters
  RegisterParameter("start_fparameter", start_fparameter = 0);
  RegisterParameter("num_fparameter", num_fparameter = 25);
}

FParameter::~FParameter()
{
  Finalize();
}

int FParameter::Initialize()
{
  return 0;
}

int FParameter::Process(ChannelData* chdata)
{
  //Calculate F parameter for  each channel on each board that is enabled
  
  double* wave = chdata->GetWaveform();
  for (size_t j = 0; j < chdata->pulses.size(); j++)
    {
      Pulse& pulse = chdata->pulses[j];
      if (!chdata->baseline.found_baseline || !pulse.found_start || 
	  pulse.peak_saturated)
	continue;
      
      if (pulse.start_index + (int)chdata->sample_rate*0.090 < pulse.end_index)
	pulse.f90 = (chdata->baseline.mean - 
		     wave[pulse.start_index + 
			  (int)(chdata->sample_rate*0.090)])/
	  pulse.peak_amplitude; 
      
      for (int k = pulse.start_index + start_fparameter; 
	   (k < pulse.start_index + start_fparameter + num_fparameter &&  
	    k < pulse.end_index); 
	   k++)
      {
	pulse.f_param.push_back((chdata->baseline.mean - wave[k])/
				pulse.peak_amplitude);
      }
    }
  
  return 0;
}

int FParameter::Finalize()
{   
  return 0;
}
