/** @file LightYield.hh
    @brief Classes and functions for calculating light yield for source runs.
    @author bloer
    @ingroup daqroot
*/

#ifndef LIGHT_YIELD_h
#define LIGHT_YIELD_h

#include "Rtypes.h"
#include <map>
#include <vector>
#include "TH1.h"
class TGraphErrors;
class TTree;

/// @addtogroup daqroot
/** 
 *  @class LightYieldGraph
 *  @brief Class to generate a scaled/calibrated plot of light yield vs time
 *
 *  @ingroup daqroot
 */

class LightYieldGraph{
  //these stick around
  std::vector<double> _x, _y, _ex, _ey; ///< arrays/errors for each axis
  std::map<int, std::vector<double> > _chan_spe; ///< single pe mean per chan
  std::map<int, std::vector<double> > _chan_spe_err; ///< spe error per chann
  TH1* histo; ///< spectrum for a single run
public:
  /// Default constructor does nothing
  LightYieldGraph() : histo(0) {} 
  virtual ~LightYieldGraph(){ if(histo) delete histo; }
  ///Add a run by run number
  int AddRun(int run, double epeak, int nbins=200, 
	     double emin=1000, double emax=2500);
  ///Add a vector of runs by number
  int AddRuns(const std::vector<int>& runs, double epeak, int nbins=200, 
	      double emin=1000, double emax=2500);

  ///Add a run by filename
  int AddFile(const char* filename, double epeak, int nbins=200, 
	     double emin=1000, double emax=2500);
  /// Find the peak of the single photoelectron distribution using in-run data
  int SetAliasesFromLocalData(TTree* Events, bool draw = false);
  
  /// Draw a spectrum up to ~1.2 MeV
  TH1* DrawSpectrum(const char* filename, int nbins = 200, double xmin=0, 
		    double xmax = 4500);
  ///Produce the graph for all the runs defined so far
  TGraphErrors* DrawGraph();
  
  /// Make a graph of the spe mean for a channel vs time
  TGraphErrors* DrawChannelSpeGraph(int channel);
  
  ///Fit the spectrum in h with a peak at energy epeak, return yield+/-yield_err
  static int FitPhotopeak(TH1* h, double epeak, 
			  double& yield, double& yield_err);

  ClassDef(LightYieldGraph,1);
};

/** @function PlotNa22LightYield
    @brief Choose specific runs to plot the Na22 coincidence light yield vs time
    This uses the LightYieldGraph interface, and adds most of the Na22 
    coincidene runs taken so far where the collimator is at the center of the 
    detector, etc. 
*/
TGraphErrors* PlotNa22LightYield();
#endif
