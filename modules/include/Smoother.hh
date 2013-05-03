/** @file Smoother.hh
    @brief Defines the Smoother module
    @author bloer
    @ingroup modules
*/

#ifndef SMOOTHER_h
#define SMOOTHER_h

#include "ChannelModule.hh"

/** @class Smoother
    @brief Module which 'smooths' pulses by taking a moving average window
    @ingroup modules
*/
class Smoother : public ChannelModule{
public:
  Smoother();
  ~Smoother();
  
  int Initialize();
  int Process(ChannelData* chdata);
  int Finalize();
  
  static const std::string GetDefaultName(){ return "Smoother"; }

private:
  int pre_samples;     ///< Number of samples before current to average over
  int post_samples;    ///< Number of samples after current to average over
};

#endif
