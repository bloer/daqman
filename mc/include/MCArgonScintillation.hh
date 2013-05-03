/** @file MCLaserData.hh
    @brief Defines the MCLaserData storage class
    @author agcocco
    @ingroup modules
*/

#ifndef MCARGONSCINT_h
#define MCARGONSCINT_h

#include "BaseModule.hh"
#include "ConfigHandler.hh"
#include "MCRawWriter.hh"
#include "MC_RunInfo.hh"
#include "MC_Params.hh"

// #include "TFile.h"
// #include "TH1F.h"
#include "TRandom.h"

enum {neutron = 1, alpha = 2, electron = 3, ffrag = 4};

const double fast_component_neutron_liquid  = 2.5/(1.0+2.5);
const double fast_component_alpha_liquid    = 1.3/(1.0+1.3);
const double fast_component_electron_liquid = 0.3/(1.0+0.3);
const double fast_component_ffrag_liquid    = 3.0/(1.0+3.0);
const double tau_fast_component_liquid    = 0.007;  // microsec
const double tau_fast_component_gas       = 0.007;  // microsec
const double tau_slow_component_liquid    = 1.40;   // microsec
const double tau_slow_component_gas       = 3.20;   // microsec

class MCArgonScintillation : public BaseModule {
public:

  MCArgonScintillation();
  ~MCArgonScintillation();

  int Initialize();
  int Finalize();
  int Process(EventPtr event);

  int Start_Run(void);
  int End_Run(void);
  void Set_Random_Seed(int ss) {ran.SetSeed(ss);}
  MC_Params* GetParams() { return &the_params;}

  void Set_Pulse(int nphe) {pulse = nphe;}
  void Set_Particle(int id) {part_id = id;}

  static const std::string GetDefaultName() { return "MCArgonScintillation";}

private:

  int mc_written;
  MCRawWriter *mcwriter;
  MC_RunInfo the_runinfo;
  MC_Params the_params;
  unsigned char *buffer;
  TRandom ran;
  double ser_integral;
  //  TFile *fser;
  //  TH1F *hser;
  int    *barePMT;
  double *histPMT;
  
  int pulse;
  int part_id;
};

#endif
