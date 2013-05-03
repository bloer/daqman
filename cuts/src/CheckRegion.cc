#include "CheckRegion.hh"
#include "EvalRois.hh"
#include "AddCutFunctor.hh"
using namespace ProcessingCuts;
AddCutFunctor::CutRegistrar<CheckRegion> _a;

ProcessingCuts::CheckRegion::CheckRegion() : 
  ProcessingCut(GetCutName(), "Check that the defined variable for a certain region of interest is within the proscribed range") 
{
  RegisterParameter("channel", channel = -1, 
		    "Which channel to look at? -1 for all");
  RegisterParameter("region", region = 0, "Which region to look at?");
  RegisterParameter("minimum", minimum = -1.e9, 
		    "Minimum allowed value for variable");
  RegisterParameter("maximum", maximum = 1.e9,
		    "Maximum allowed variable");
  RegisterParameter("variable", variable = INTEGRAL,
		    "Which variable to look at? INTEGRAL, MAX, MIN, AMPLITUDE");
  
}

bool ProcessingCuts::CheckRegion::Process(EventDataPtr event)
{
  if( channel == -1 )
    return ProcessingCut::Process(event);
  ChannelData* chdata = event->GetChannelByID(channel);
  if(!channel)
    return false;
  return ProcessChannel(chdata);
}

bool ProcessingCuts::CheckRegion::ProcessChannel(ChannelData* chdata)
{
  if(region >= (int)(chdata->regions.size())) return default_pass;
  Roi& roi = chdata->regions[region];
  double value=0;
  switch(variable){
  case MIN:
    value = roi.min;
    break;
  case MAX:
    value = roi.max;
    break;
  case INTEGRAL:
    value = roi.integral;
    break;
  case NPE:
    value = roi.npe;
    break;
  case AMPLITUDE:
    value = chdata->baseline.mean - roi.min;
    break;
  };
  return (value > minimum) && (value < maximum);
}

int ProcessingCuts::CheckRegion::AddDependenciesToModule(BaseModule* mod)
{
  mod->AddDependency("EvalRois");
  return 1;
}
