#include "CheckNPulses.hh"
#include "AddCutFunctor.hh"
#include "PulseFinder.hh"

using namespace ProcessingCuts;

REGISTER_CUT(CheckNPulses)

CheckNPulses::CheckNPulses() : 
ProcessingCut(GetCutName(), "Count the number of pulses found by PulseFinder")
{
  RegisterParameter("min_pulses",min_pulses=0, 
		    "Minimum allowed number of pulses");
  RegisterParameter("max_pulses",max_pulses=100,
		    "Maximum allowed number of pulses");
  default_pass = true;
}

bool CheckNPulses::ProcessChannel(ChannelData* chdata)
{
  return chdata->npulses >= min_pulses && chdata->npulses <= max_pulses;
}

int CheckNPulses::AddDependenciesToModule(BaseModule* mod)
{
  mod->AddDependency("PulseFinder");
  return 1;
}
