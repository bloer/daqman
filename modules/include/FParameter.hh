/** @file FParameter.hh
    @brief defines FParameter calculation module
    @author rsaldanha
    @ingroup modules
*/

#ifndef FPARAMETER_h
#define FPARAMETER_h

#include "ChannelModule.hh"
  
/** @class FParameter
    @brief Calculate the fparameter (ratio of fast/slow amplitudes) for a pulse
    @ingroup modules
*/
class FParameter : public ChannelModule
{
public:
  FParameter();
  ~FParameter();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "FParameter";}
    
private:
  //parameters
  int start_fparameter; ///< Starting index to measure f parameter
  int num_fparameter;   ///< Number of f parameter calculations
};


#endif
