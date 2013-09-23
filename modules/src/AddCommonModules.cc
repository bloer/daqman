#include "EventHandler.hh"
#include "BaselineFinder.hh"
#include "ConvertData.hh"
#include "Differentiator.hh"
#include "Fitter.hh"
#include "FParameter.hh"
#include "PulseFinder.hh"
#include "SpeFinder.hh"
#include "eTrainFinder.hh"
#include "SumChannels.hh"
#include "Smoother.hh"
#include "EvalRois.hh"
#include "Integrator.hh"
#include "AverageWaveforms.hh"
#include "S1S2Evaluation.hh"
#include "PulseShapeEval.hh"
#include "SumOfIntegralEval.hh"
#include "TimeOfFlight.hh"

int EventHandler::AddCommonModules()
{
  AddModule<ConvertData>();
  AddModule<SumChannels>();
  AddModule<BaselineFinder>();
  AddModule<Integrator>();
  //AddModule<Differentiator>();
  //AddModule<Fitter>();
  //AddModule<FParameter>();
  AddModule<PulseFinder>();
  //AddModule<Smoother>();
  AddModule<EvalRois>();
  AddModule<S1S2Evaluation>();
  AddModule<eTrainFinder>();
  AddModule<SpeFinder>();
  AddModule<PulseShapeEval>();
  AddModule<SumOfIntegralEval>();
  AddModule<AverageWaveforms>();
#ifdef SCENEDB
  AddModule<TimeOfFlight>();
#endif
  return 13;
}
