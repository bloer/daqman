#include "MC_Params.hh"
#include "Message.hh"
// #include <exception>
// #include <stdexcept>
// #include <math.h>

using namespace std;


MC_Params::MC_Params() : ParameterList("MC_Params")
{
  RegisterParameter("mch_run_number",         mch_run_number = 0,         "MC run number");
  RegisterParameter("mch_number_of_events",   mch_number_of_events = 0,   "MC event number");
  RegisterParameter("mch_number_of_channels", mch_number_of_channels = 0, "MC number of channels");
  RegisterParameter("mch_number_of_samples",  mch_number_of_samples = 0,  "MC number of samples");
  RegisterParameter("mch_trigger_delay",      mch_trigger_delay = 0,      "MC triggger delay");
  RegisterParameter("mch_baseline",           mch_baseline = 0,           "MC baseline");
  RegisterParameter("mch_sampling_time",      mch_sampling_time = 0,      "MC sampling time");
  RegisterParameter("mch_single_rate",        mch_single_rate = 0,        "MC single rate");
}
