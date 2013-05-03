/** @file Differentiator.hh
    @brief defines the Differentiator module
    @author rsaldanha
    @ingroup modules
*/

#ifndef DIFFERENTIATOR_h
#define DIFFERENTIATOR_h

#include "ChannelModule.hh"
/** @class Differentiator
    @brief Numerically differentiate a channel's waveform
    @ingroup modules
*/
class Differentiator : public ChannelModule
{
public:
  Differentiator();
  ~Differentiator();

  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);

  static const std::string GetDefaultName(){ return "Differentiator";}
  
  //parameters
  double decay_constant; ///< Decay time constant of preamp integrator in us

private:
  
};

#endif
