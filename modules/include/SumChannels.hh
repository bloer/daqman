/** @file SumChannels.hh
    @brief Defines the SumChannels module
    @author bloer
    @ingroup modules
*/

#ifndef SUMCHANNELS_h
#define SUMCHANNELS_h

#include "BaseModule.hh"

/** @class SumChannels
    @brief Creates an extra 'channel' which is the sum of all channels 
    @ingroup modules
*/
class SumChannels : public BaseModule
{
public:
  SumChannels();
  ~SumChannels();
  
  int Initialize();
  int Finalize();
  int Process(EventPtr event);
  
  static const std::string GetDefaultName(){ return "SumChannels";}
  
};

#endif
