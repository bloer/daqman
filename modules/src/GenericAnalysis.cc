#include "GenericAnalysis.hh"
#include "ConvertData.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "Integrator.hh"
#include "RootWriter.hh"
#include "intarray.hh"


#include "TRandom3.h"

GenericAnalysis::GenericAnalysis() : 
  BaseModule(GetDefaultName(), "Doesn't do anything by default; empty module meant to serve as an example module or for testing algorithms")
{
  AddDependency<ConvertData>();
  AddDependency<BaselineFinder>();
  //AddDependency<Integrator>();

  //to add something to be controlled by the config file, use the syntax:
  //  ReisterParameter("parameter_name", parameter_variable);
}

GenericAnalysis::~GenericAnalysis()
{ 
  //delete stuff here
}

int GenericAnalysis::Initialize()
{
  //initialize histograms or class variables here

  //return something other than zero for error
  return 0;
}

int GenericAnalysis::Finalize()
{
  //make sure to close/delete anything opened in Initialize()
  
  //return something other than zero for error
  return 0;
}

int GenericAnalysis::Process(EventPtr event)
{
  EventDataPtr data = event->GetEventData();
  //store event-wide results in the 'data' object
  //data->generic.push_back(my_result);
  
  std::vector<ChannelData>::iterator chdata = data->channels.begin();
  for( ; chdata != data->channels.end(); chdata++){
    //chdata is a pointer to a ChannelData object, looping over all channels
 
    //double baseline = chdata->baseline.mean;
    //if we absolutely need the baseline to proceed, do
    // if( ! chdata->baseline.found_baseline) continue;
    
    //to get access to the channel's waveform, use an intarray object 
    //we don't know a priori what the depth of the digitizer is
    //int nsamps = chdata->nsamps;
    //double* wave = chdata->GetWaveform();
    //if you want the integral (make sure to include Integrator as a dependency!) it's in
    //chdata->integral_start;
    
    
    //store your channel-specific results in the chdata->generic vector
    //chdata->generic.push_back(channel_result);
    
  }
  
  //return non-zero in case of error
  return 0;
}
