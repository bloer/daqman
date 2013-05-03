/** @file WarpVeto.hh
    @brief Defines the WarpVeto module
    @author bloer
    @ingroup modules
    @ingroup daqman
*/

#ifndef WARPVETO_h
#define WARPVETO_h

#include "BaseModule.hh"
#include <boost/smart_ptr.hpp>

/** @class WarpVeto
    @brief Calculate in real-time simple statistics for the warp veto
    @ingroup daqman
    @ingroup modules
*/
class WarpVeto : public BaseModule{
public:
  WarpVeto();
  ~WarpVeto();
  
  static std::string GetDefaultName(){ return "WarpVeto"; }
  
  int Initialize();
  int Finalize();
  int Process(EventPtr event);
  
  const std::vector<char>* GetDataBuffer(){ return &_buffer; }

private:
  uint32_t max_channels;
  std::vector<char> _buffer;
};

#endif
