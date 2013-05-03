/** @file CheckRegion.hh
    @brief Defines the CheckRegion cut
    @author bloer
    @ingroup cuts
*/

#ifndef CHECKREGION_h 
#define CHECKREGION_h 
#include <stdexcept>
#include "ProcessingCut.hh"
namespace ProcessingCuts{
  
  /** @class CheckRegion
      @brief Pass an event based on a value from a specific region of interest
      @ingroup cuts
  */
  class CheckRegion : public ProcessingCut{
  public:
    CheckRegion();
    ~CheckRegion() {}
    
    static std::string GetCutName(){ return "CheckRegion";}
    
    bool Process(EventDataPtr event);
    int AddDependenciesToModule(BaseModule* mod);
    /// Available variables to check
    enum variable_t {MIN, MAX, INTEGRAL, NPE, AMPLITUDE};
  private:
    bool ProcessChannel(ChannelData* chdata);
    int channel;                 ///< which channel to evaluate?
    int region;                  ///< which region to evaluate?
    double minimum;                 ///< maximum allowed value for variable 
    double maximum;                 ///< minimum allowed value for variable
    variable_t variable;         ///< which variable of the ROI to check?
    bool default_pass;           ///< require all channels to pass to be true?
  };

  /// Overload ostream operator to stream variable_t
  inline
  std::ostream& operator<<(std::ostream& out, CheckRegion::variable_t& var){
    if(var == CheckRegion::MIN)
      return out<<"min";
    else if(var == CheckRegion::MAX)
      return out<<"max";
    else if(var == CheckRegion::INTEGRAL)
      return out<<"integral";
    else if(var == CheckRegion::NPE)
      return out<<"npe";
    else if(var == CheckRegion::AMPLITUDE)
      return out<<"amplitude";
    return out;
  }
  /// Overload istream operator to stream variable_t
  inline
  std::istream& operator>>(std::istream& in, CheckRegion::variable_t& var){
    std::string vname;
    in>>vname;
    if(vname == "min") var = CheckRegion::MIN;
    else if (vname == "max") var = CheckRegion::MAX;
    else if (vname == "integral") var = CheckRegion::INTEGRAL;
    else if (vname == "npe") var = CheckRegion::NPE;
    else if (vname == "amplitude") var = CheckRegion::AMPLITUDE;
    else{
      std::cerr<<"Unknown variable type "<<vname<<std::endl;
      throw std::invalid_argument("CheckRegion::variable_t has no entry"+vname);
    }
    return in;
  }
}

#endif
