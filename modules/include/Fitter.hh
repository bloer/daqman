/** @file Fitter.hh
    @brief Defines the Fitter module
    @author rsaldanha
    @ingroup modules
*/
#ifndef FITTER_h
#define FITTER_h

#include "ChannelModule.hh"

#include <vector>
/** @class Fitter
    @brief Fit a pulse with some PDF
    @ingroup modules
*/
class Fitter : public ChannelModule
{
public:
  Fitter();
  ~Fitter();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "Fitter";}

  //parameters
  int start_fit;                 ///< first sample in fit range
  int end_fit;                   ///< last sample in fit range
  int slow_sample_rate;          
  int start_slow_sample_rate;
  
private:
  
};


#endif
