/** @file MCOptical.hh
    @brief Defines the MCOptical storage class
    @author agcocco
    @ingroup modules
*/

#ifndef MCOPTICAL_h
#define MCOPTICAL_h

#include "BaseModule.hh"
#include "ConfigHandler.hh"
#include "MCRawWriter.hh"
#include "MC_RunInfo.hh"
#include "MC_Params.hh"

#include "TTree.h"
#include "TFile.h"
#include "TH1F.h"
#include "TRandom.h"

#include "MCOpticalEvent.hh"

class MCOptical : public BaseModule {
public:

  MCOptical();
  ~MCOptical();

  int Initialize();
  int Finalize();
  int Process(EventPtr event);

  int Start_Run(void);
  int End_Run(void);
  void Set_Random_Seed(int ss) {ran.SetSeed(ss);}
  MC_Params* GetParams() { return &the_params;}

  void Set_Filename(char* gg) {input_file = gg;}

  static const std::string GetDefaultName() { return "MCOptical";}

private:

  int mc_written;
  MCRawWriter *mcwriter;
  MC_RunInfo the_runinfo;
  MC_Params the_params;
  unsigned char *buffer;
  TRandom ran;
  double ser_integral;
  TFile *fn;
  TFile *fser;
  TH1F *hser;
  int    *barePMT;
  double *histPMT;

  TString input_file;

  TTree *tree;
  PMHit *pmh;
  TClonesArray *tc1;
  TClonesArray *tc2;
  MCOpticalEvent *ev;

  int nevin;
  int nevcurr;

};

#endif
