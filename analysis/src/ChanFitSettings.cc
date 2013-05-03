#include "ChanFitSettings.hh"
 
ChanFitSettings::ChanFitSettings(const std::string& chan, 
			     const std::string helptext) :
  ParameterList(chan, helptext), _chan(chan)
{
	RegisterParameter("constant_start_value", constant_start_value = -9999, 
		    "Fitting start value for total integral");
	RegisterParameter("constant_min_bound", constant_min_bound = 0, 
		    "Fitting minimum bound for total integral");
	RegisterParameter("constant_max_bound", constant_max_bound = 6e6, 
		    "Fitting maximum bound for total integral");
			
	RegisterParameter("lambda_start_value", lambda_start_value = 0.05, 
		    "Fitting start value for occupancy");
	RegisterParameter("lambda_min_bound", lambda_min_bound = 0, 
		    "Fitting minimum bound for occupancy");
	RegisterParameter("lambda_max_bound", lambda_max_bound = 6, 
		    "Fitting maximum bound for occupancy");
			
	RegisterParameter("mean_start_value", mean_start_value = 100, 
		    "Fitting start value for gaussian center");
	RegisterParameter("mean_min_bound", mean_min_bound = 0, 
		    "Fitting minimum bound for gaussian center");
	RegisterParameter("mean_max_bound", mean_max_bound = 300, 
		    "Fitting maximum bound for gaussian center");

	RegisterParameter("sigma_start_value", sigma_start_value = 27, 
		    "Fitting start value for gaussian width");
	RegisterParameter("sigma_min_bound", sigma_min_bound = 5, 
		    "Fitting minimum bound for gaussian width");
	RegisterParameter("sigma_max_bound", sigma_max_bound = 150, 
		    "Fitting maximum bound for gaussian width");

	RegisterParameter("amp_E_start_value", amp_E_start_value = 50, 
		    "Fitting start value for exponential slope");
	RegisterParameter("amp_E_min_bound", amp_E_min_bound = 5, 
		    "Fitting minimum bound for exponential slope");
	RegisterParameter("amp_E_max_bound", amp_E_max_bound = 150, 
		    "Fitting maximum bound for exponential slope");

	RegisterParameter("p_E_start_value", p_E_start_value = .2, 
		    "Fitting start value for exponential height");
	RegisterParameter("p_E_min_bound", p_E_min_bound = 0, 
		    "Fitting minimum bound for exponential height");
	RegisterParameter("p_E_max_bound", p_E_max_bound = 1, 
		    "Fitting maximum bound for exponential height");			

	RegisterParameter("shotnoise_start_value", shotnoise_start_value = 8, 
		    "Fitting start value for pedestal width");
	RegisterParameter("shotnoise_min_bound", shotnoise_min_bound = 0.1, 
		    "Fitting minimum bound for pedestal width");
	RegisterParameter("shotnoise_max_bound", shotnoise_max_bound = 70, 
		    "Fitting maximum bound for pedestal width");			

	RegisterParameter("pedmean_start_value", pedmean_start_value = 0, 
		    "Fitting start value for pedestal center");
	RegisterParameter("pedmean_min_bound", pedmean_min_bound = -10, 
		    "Fitting minimum bound for pedestal center");
	RegisterParameter("pedmean_max_bound", pedmean_max_bound = 10, 
		    "Fitting maximum bound for pedestal center");			

	
	RegisterParameter("range_min", range_min = -30, 
	    "Fitting range: min");
	RegisterParameter("range_max", range_max = 300, 
	    "Fitting range: max");
	
	RegisterParameter("pedrange_min", pedrange_min = -30, 
	    "Range for pedestal centering:min ");
	RegisterParameter("pedrange_max", pedrange_max = 15, 
	    "Range for pedestal centering:max ");

}

/* 
bool ChanFitSettings::Process(EventDataPtr event)
{
  bool pass_once = false;
  for(size_t i=0; i < event->channels.size(); i++){
    bool pass = ProcessChannel(&(event->channels[i]));
    if( pass && default_pass ) return true;
    if( !pass && !default_pass ) return false;
    if(pass) pass_once = true;
  }
  return pass_once;
}
 */
