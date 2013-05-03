#include "Fitter.hh"
#include "EventHandler.hh"
#include "SumChannels.hh"
#include "PulseFinder.hh"
#include "TGraph.h"
#include "RootWriter.hh"
#include "intarray.hh"
#include "TF1.h"
#include <vector>
#include <algorithm>
#include <math.h>

Fitter::Fitter():
  ChannelModule(GetDefaultName(), 
		"Fit the pulse to the known scintillation shape")
{
  AddDependency<PulseFinder>();
  ///@todo Provide helptext for Fitter parameters
  //Register all the config handler parameters
  RegisterParameter("start_fit", start_fit = -100);
  RegisterParameter("end_fit", end_fit = 80000);
  RegisterParameter("slow_sample_rate", slow_sample_rate = 10);
  RegisterParameter("start_slow_sample_rate", start_slow_sample_rate = 500);
}

Fitter::~Fitter()
{
  Finalize();
}

int Fitter::Initialize()
{
  return 0;
}

int Fitter::Process(ChannelData* chdata)
{
  if( chdata->pulses.size()==0) 
    return 0;
  
  double* wave = chdata->GetWaveform();
  
  for(size_t j=0; j < chdata->pulses.size(); j++){
    Pulse& pulse = chdata->pulses[j];
    if(!chdata->baseline.found_baseline || !pulse.found_start || 
       !pulse.found_peak || pulse.peak_saturated)
      return 0;
    
    PulseFit& fit = pulse.fit;
    fit.start_index = std::max(pulse.start_index+start_fit,0);
    fit.end_index = std::min(pulse.start_index+end_fit, pulse.end_index);
    
    int npts = pulse.start_index + start_slow_sample_rate - fit.start_index + 
      (int)ceil((fit.end_index - pulse.start_index - start_slow_sample_rate)/slow_sample_rate) + 1;
    int x[npts];
    int y[npts];
    
    int counter = 0;
    for(int samp=fit.start_index; samp< fit.end_index; samp++)
      {
	if(samp < pulse.start_index + start_slow_sample_rate || 
	   samp%slow_sample_rate == 0)
	  {
	    x[counter] = samp;
	    y[counter] = (int)wave[samp];
	    counter++;
	  }
      }
    TGraph graph(npts,x,y);
    
    
    //perform the fit
    
    
    TF1* func = fit.GetTF1();
    
    func->SetRange(fit.start_index, fit.end_index);
    func->SetParameters(1.09 * pulse.peak_amplitude, 
			0.3, 0.7, 160., 1., 12000., 
			chdata->baseline.mean, pulse.start_index + 0.5, 7500);
    func->SetParLimits(1, 0, 1);
    func->FixParameter(2, 0.70);
    func->FixParameter(6, chdata->baseline.mean);
    
    fit.fit_result = graph.Fit(func,"RWQN");
    
    if(fit.fit_result != 0){
      func->SetParameter(2,0.8);
      fit.fit_result = graph.Fit(func,"RWQN");
    }
    
    fit.fit_done = true;
    fit.StoreParams(func);
    
    delete func;
  }//end loop over pulses
  return 0;
}

int Fitter::Finalize()
{   
  return 0;
}


