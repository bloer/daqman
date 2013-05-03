#include "Smoother.hh"
#include "SumChannels.hh"
#include "ProcessedPlotter.hh"
#include "ConvertData.hh"
#include "EventHandler.hh"
#include "intarray.hh"
#include "RootWriter.hh"
#include <algorithm> 
Smoother::Smoother() : 
  ChannelModule(GetDefaultName(),
		"Smooth each channel's waveform using a moving average window")
{
  RegisterParameter("pre_samples", pre_samples = 2,
		    "Number of samples to include in the average before the current one");
  RegisterParameter("post_samples", post_samples = 2,
		    "Number of samples to include in the average after the current one");

  AddDependency<ConvertData>();
}

Smoother::~Smoother()
{}

int Smoother::Initialize()
{ 
  return 0; 
}

int Smoother::Finalize()
{ return 0; }

int Smoother::Process(ChannelData* chdata)
{
  const int nsamps = chdata->nsamps;
  chdata->smoothed_data.resize(nsamps);
  double* smoothdata = &(chdata->smoothed_data[0]);
  //double* smoothdata = new double[nsamps];
  double* wave = chdata->GetWaveform();
  double running_sum = 0;
  double samps_in_sum = post_samples;
  for(int j = 0; j<post_samples && j<nsamps; j++)
    running_sum += wave[j];
  for(int samp = 0; samp < nsamps; samp++){
    if(samp < nsamps - post_samples){
      running_sum += wave[samp+post_samples];
      samps_in_sum++;
    }
    if(samp > pre_samples){
      running_sum -= wave[samp-pre_samples-1];
      samps_in_sum--;
    }
    smoothdata[samp] = running_sum / samps_in_sum;
  }
  chdata->smoothed_min = *std::min_element(smoothdata, smoothdata+nsamps);
  chdata->smoothed_max = *std::max_element(smoothdata, smoothdata+nsamps);
  
  return 0;
}
