/** @file CheckNPulses.hh
    @brief Defines the CheckNPulses cut
    @author bloer
    @ingroup cuts
*/
#ifndef CHECKNPULSES_h
#define CHECKNPULSES_h

#include "ProcessingCut.hh"

namespace ProcessingCuts{
  /** @class CheckNPulses
      @brief Cut to pass an event based on the total number of 'pulses' detected
      @ingroup cuts
  */
  class CheckNPulses : public ProcessingCut{
  public:
    CheckNPulses();
    ~CheckNPulses(){}
    static std::string GetCutName(){ return "CheckNPulses";}
    
    bool ProcessChannel(ChannelData* chdata);
    int AddDependenciesToModule(BaseModule* mod);
    
  private:
    int min_pulses;   ///< minimum number of pulses allowed to pass
    int max_pulses;   ///< maximum number of pulses allowed to pass
    
  };
}

#endif
