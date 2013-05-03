/** @defgroup cuts cuts - Skip processing of some modules based on cuts
    @ingroup modules
*/

/** @file ChanFitSettings.hh
    @brief Defines the ChanFitSettings base class
    @author bloer
    @ingroup cuts
*/

#ifndef CHAN_FIT_SETTINGS_h
#define CHAN_FIT_SETTINGS_h

#include "ParameterList.hh"
class BaseModule;

/** @class ChanFitSettings
    @brief Base class representing a cut to check whether a module should run
    @ingroup cuts
*/
class ChanFitSettings : public ParameterList{
  std::string _chan; ///< The name of the cut for config files
public:
  /// Constructor takes name of the cut
  ChanFitSettings(const std::string& chan="0", const std::string helptext="");
  /// Destructor 
  virtual ~ChanFitSettings(){}
  
  /// Evaluate the entire event to determine if it should pass
 // virtual bool Process(EventDataPtr event);
 
  std::string GetName(){ return _chan; }
  double constant_start_value;
	double constant_min_bound;
	double constant_max_bound;
	
	double lambda_start_value;
	double lambda_min_bound;
	double lambda_max_bound;
			
	double mean_start_value;
	double mean_min_bound;
	double mean_max_bound;
	
	double sigma_start_value;
	double sigma_min_bound;
	double sigma_max_bound;
	
	double amp_E_start_value;
	double amp_E_min_bound;
	double amp_E_max_bound;

	double p_E_start_value;
	double p_E_min_bound;
	double p_E_max_bound;

	double shotnoise_start_value;
	double shotnoise_min_bound;
	double shotnoise_max_bound;

	double pedmean_start_value;
	double pedmean_min_bound;
	double pedmean_max_bound;

	
	double range_min;
	double range_max;
	
	double pedrange_min;
	double pedrange_max;
	
};

/** @namespace ChanFitSettings
    @brief Segments cuts so they don't conflict with modules in empty namespace
    @ingroup cuts
*/
namespace ChanFitSetting{}

#endif
