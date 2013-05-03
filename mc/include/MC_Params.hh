/** @file MC_Params.hh
    @brief defines the MC_Params class.
    @author agc
    @ingroup mc
*/

#ifndef MC_PARAMS_h
#define MC_PARAMS_h

#include "ParameterList.hh"
#include <string>
#include <iostream>
#include <fstream>
#include "stdint.h"

/** @addtogroup mc
    @{
*/

/** @class MC_Params
    @brief parameters relevant to all boards in the run
*/
class MC_Params : public ParameterList
{
public:
  //Access functions
  /// Default Constructor
  MC_Params();
  /// Destructor does nothing
  ~MC_Params(){}
  
  //public data members
  
  int mch_run_number;            ///< mc run number
  int mch_number_of_events;      ///< number of events in this run
  int mch_number_of_channels;    ///< number of channels enabled in this run 
  int mch_number_of_samples;    ///< number of channels enabled in this run 
  int mch_trigger_delay;
  int mch_baseline;
  int mch_sampling_time;
  int mch_single_rate;

  //utility functions
  /// Get the run_number
  int GetRunNumber(void) {return mch_run_number;}; 
  /// Get the number of boards enabled in this run
  int GetNumberOfEvents(void) {return mch_number_of_events;};
  /// Get the number of channels enabled in this run
  int GetNumberOfChannels(void) {return mch_number_of_channels;};
  
};


/// @}
#endif

