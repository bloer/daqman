/** @file MCLaserData.hh
    @brief Defines the MCLaserData storage class
    @author agcocco
    @ingroup modules
*/

#ifndef MCLASERDATA_h
#define MCLASERDATA_h

#include "BaseModule.hh"
#include "ConfigHandler.hh"
#include "MCRawWriter.hh"
#include "MC_RunInfo.hh"
#include "MC_Params.hh"

// #include "TFile.h"
#include "TH1F.h"
#include "TRandom3.h"

class MCLaserData : public BaseModule {
public:

  MCLaserData();
  ~MCLaserData();

  int Initialize();
  int Finalize();
  int Process(EventPtr event);

  int Start_Run(void);
  int End_Run(void);
  void Set_Random_Seed(int ss) {ran.SetSeed(ss);}
  MC_Params* GetParams() { return &the_params;}

  void Set_PoissonMean(double mean) {poisson_mean = mean;}

  static const std::string GetDefaultName() { return "MCLaserData";}

private:

  int mc_written;
  MCRawWriter *mcwriter;
  MC_RunInfo the_runinfo;
  MC_Params the_params;
  unsigned char *buffer;
  TRandom3 ran;
  double ser_integral;
  // TFile *fser;
  // TH1F *hser;
  int    *barePMT;          ///< Array containing the number of photoelectrons detected at each sample time 
  double *histPMT;          ///< Array containing the signal from the phototubes at each sample time

  double poisson_mean;
};

#endif
