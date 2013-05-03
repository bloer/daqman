/** @file PositionRecon.hh
    @brief Defines the PositionRecon module
    @author bloer
    @ingroup modules
*/
#ifndef POSITIONRECON_h
#define POSITIONRECON_h

#include "BaseModule.hh"
#include <vector>
#include <map>

/** @class PositionRecon
    @brief Calculate the x,y,z position of an event
    @ingroup modules
 */
class PositionRecon : public BaseModule{
public:
  ///@todo Document class members
  PositionRecon();
  virtual ~PositionRecon() {}
  
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  
  static std::string GetDefaultName(){ return "PositionRecon"; }
  
private:
  double drift_speed;     ///< electron drift speed in mm/microsec
  bool use_full_s2;      ///< use the s2_full (true) or s2_fixed (false) varible to evaluate the barycenter;
  bool enable_full_recon; ///< enable or not the full (slow) x-y recon
  std::string calib_file; ///< file containing the recon calibration points
  bool use_pmt_efficiency; ///< take into account measured QE of PMTs?
  
  //recon using simulation output
  std::vector<std::vector<double> > _calib_points;
  int LoadCalibration();
    
};

#endif
