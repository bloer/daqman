/** @file ChannelModule.hh
    @brief defines the ChannelModule base class
    @author bloer
    @ingroup modules
*/

#ifndef CHANNELMODULE_h
#define CHANNELMODULE_h

#include "BaseModule.hh"

/** @class ChannelModule 
    @brief abstract class that acts on each channel of an event
    @ingroup modules
*/
class ChannelModule : public BaseModule{
public:
  ChannelModule(const std::string& name="", const std::string& helptext="");
  virtual ~ChannelModule();
  int Process(EventPtr event);
  
  /// Process a single channel in a single trigger. Return 0 if no error.
  virtual int Process(ChannelData* chdata)=0;
  
  /// See if this channel passes cuts
  bool CheckCuts(ChannelData* chdata);
  
protected:
  EventPtr _current_event;  ///< Pointer to current event
  bool _skip_sum;    ///< Do we skip processing the special sum channel?
  bool _sum_only;    ///< Do we process the sum channel only (and not others?)
};

#endif

