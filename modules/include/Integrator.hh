/** @file Integrator.hh
    @brief defines the Integrator module
    @author bloer
    @ingroup modules
*/

#ifndef INTEGRATOR_h
#define INTEGRATOR_h

#include "ChannelModule.hh"
/** @class Integrator
    @brief Integrate a channel's waveform digitally
    @ingroup modules
*/
class Integrator : public ChannelModule
{
public:
  Integrator();
  ~Integrator();
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "Integrator"; }
private:
  double threshold; ///< minimum value about baseline to count integral
  
};

#endif
