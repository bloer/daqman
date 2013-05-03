#include "TROOT.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TCut.h"
#include "TTree.h"
#include "TString.h"
#include <algorithm>
#include <string>
#include <iomanip>
#include <iostream>

//#include "modules/EventData.h"

int GetDarkHits(TTree* Events, double threshold, double trigtime=0)
{
  TString cut = "channels.pulses.integral > ";
  cut += threshold;
  int hits = Events->Draw("channels.pulses.integral", cut,"goff");
  std::cout<<"Number of hits: "<<hits<<std::endl;
  if(trigtime > 0){
    std::cout<<"Dark Rate: "<<1.*hits / (trigtime * Events->GetEntries())
	     <<" +/-  "
	     <<sqrt(hits) / (trigtime * Events->GetEntries())
	     <<std::endl;
  }
  return hits;
}

int ProcessDarkRateFile(const char* filename, double threshold, 
			double trigtime=0)
{
  static bool loaded = false;
  if(!loaded){
    gROOT->ProcessLine(".L lib/libDict.so");
    loaded=true;
  }
  TFile fin(filename);
  if(!fin.IsOpen() || fin.IsZombie()){
    std::cerr<<"Unable to open file "<<filename<<std::endl;
    return -1;
  }
  TTree* Events = (TTree*)(fin.Get("Events"));
  if(!Events){
    std::cerr<<"Unable to open Events tree in file "<<filename<<std::endl;
    return -2;
  }
  return GetDarkHits(Events, threshold, trigtime);
} 
  
