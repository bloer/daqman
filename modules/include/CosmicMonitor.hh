/** @file CosmicMonitor.hh
    @brief defines the CosmicMonitor module
    @author huajiec
    @ingroup modules
*/

#ifndef COSMICMONITOR_h
#define COSMICMONITOR_h

#include "ChannelModule.hh"
/** @class CosmicMonitor
    @brief CosmicMonitor processes info on the cosmic ray channel
    @ingroup modules
*/
class CosmicMonitor : public ChannelModule
{
public:
  CosmicMonitor();
  ~CosmicMonitor();
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "CosmicMonitor"; }
private:
  double threshold; ///< threshold
  
};

#endif
