#include "PositionRecon.hh"
#include "PulseFinder.hh"
#include "S1S2Evaluation.hh"
#include "RunDB.hh"
#include "RootWriter.hh"

#include "TMinuitMinimizer.h"

#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

PositionRecon::PositionRecon() :
  BaseModule(GetDefaultName(), "Calculate the x,y,z position of an event")
{
  AddDependency<PulseFinder>();
  AddDependency<S1S2Evaluation>();
  
  RegisterParameter("drift_speed", drift_speed = 1,
		    "Electron drift speed in mm/microsec");
  RegisterParameter("use_full_s2", use_full_s2 = true,
		    "Use the s2_full variable (true) or s2_fixed (false) to evaluate the barycenter");
  RegisterParameter("enable_full_recon",enable_full_recon = true,
		    "Enable the full (slow) position recon");
  RegisterParameter("calib_file",calib_file="auxiliary_files/ds10_recon_calib.txt",
		    "File containing calibration data for full recon");
  RegisterParameter("use_pmt_efficiency",use_pmt_efficiency=false,
		    "Take into account measured QE of PMTs?");
}

int PositionRecon::Initialize()
{
  
  if(enable_full_recon)
    return LoadCalibration();

  return 0;
}

int PositionRecon::Finalize()
{
  return 0;
}

int PositionRecon::LoadCalibration()
{
  std::ifstream fin(calib_file.c_str());
  if(!fin.is_open()){
    Message(ERROR)<<"Unable to open position calibration file "
		  <<calib_file<<".\n";
    return 1;
  }
  
  _calib_points.clear();
  std::string line;
  while(std::getline(fin,line)){
    std::stringstream s(line);
    double val;
    std::vector<double> vals;
    while(s>>val)
      vals.push_back(val);
    //make sure we're actually within the fiducial volume...
    if( sqrt(vals[0]*vals[0]+vals[1]*vals[1]) <= 8.25*2.54/2. )
      _calib_points.push_back(vals);
  }
  Message(DEBUG)<<"Loaded "<<_calib_points.size()<<" calibration points.\n";
  if(_calib_points.size() == 0)
    return 1;
  
  //make sure we have the right number of PMTs
  /*
    size_t ns2 = 0;
  for(std::map<int,PMT>::iterator it = _pmts.begin(); it != _pmts.end(); ++it){
    if(it->second.use_for_s2)
      ns2++;
  }
  if( ns2 != _calib_points[0].size()-2){
    Message(ERROR)<<"Position calibration file does not contain the right "
		  <<"number of channels! Expect "<<ns2<<"; got "
		  <<_calib_points[0].size()-2<<"!\n";
    return 1;
  }
  */
  return 0;  
}

int PositionRecon::Process(EventPtr evt)
{
  EventDataPtr data = evt->GetEventData();
  data->position_valid = false;
  if (!data->pulses_aligned && data->channels.size() > 1)
      return 0;
  if(!data->s1s2_valid){
    //can't do anything
    return 0;
  }
  //z = 0 is the bottom of the active volume
  data->z = 24.1 - data->drift_time*drift_speed/10.; //speed mm/us, need cm
  
  /*********** Barycenter ************/
  //x, y are the weighted averages of top pmts
  double x=0, y=0, total=0;
  std::vector<double> recons2; //< for calibrated recon
  for(size_t i=0; i < data->channels.size(); i++)
  {
      ChannelData& chdata = data->channels[i];
      int id = chdata.channel_id;
      if(id < 0 || _skip_channels.find(chdata.channel_id) != _skip_channels.end())
	  continue;
      
    if(chdata.pmt.photocathode_z > 0)
    {
	double s2 = (use_full_s2 ? chdata.s2_full : chdata.s2_fixed);
	double escaled = s2/(chdata.pmt.photocathode_exp_area * chdata.pmt.qe);
	double s2use = (use_pmt_efficiency ? escaled : s2);
	recons2.push_back(s2use);
	x += chdata.pmt.photocathode_x * s2use;
	y += chdata.pmt.photocathode_y * s2use;
	total += s2use;
    }
  }

  if(total > 0){
    data->bary_x = x/total;
    data->bary_y = y/total;
    data->bary_valid = true;
  }
    
  /************* Full recon *****************/
  std::vector<double> frac(recons2.size());
  std::vector<double> err(recons2.size());
  for(size_t i=0; i<recons2.size(); ++i){
    frac[i] = recons2[i]/total;
    err[i] = sqrt(recons2[i])/total;
  }
			   
  if(enable_full_recon && data->bary_valid){
      data->position_valid = true;
    //find the minimum chi2 on all calib points
    double minchi2=1.e12;
    for(size_t pt=0; pt<_calib_points.size(); ++pt){
      double chi2=0;
      for(size_t ch=0; ch<recons2.size(); ++ch){
	double val = _calib_points[pt][2+ch];
	chi2 += (frac[ch]-val)*(frac[ch]-val)/(err[ch]*err[ch]);
      }
      if(chi2<minchi2){
	minchi2 = chi2;
	data->x = _calib_points[pt][0];
	data->y = _calib_points[pt][1];
      }
    }
  }
    
  return 0;
}

