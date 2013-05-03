#include "LightYieldCorrection.hh"
#include "utilities.hh"

#include "EventData.hh"
#include "EventHandler.hh"

#include "TROOT.h"
#include "TGraph.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TCut.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TRandom3.h"
#include "TTree.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TNtuple.h"
#include "TFile.h"


#include <iostream>
#include <numeric>
#include <sstream>

TGraph* MakeFakePulse(TGraph* singlepe, int nphotons, TGraph* output,
		      double spe_resolution, double t0, 
		      double fprompt, double tslow, 
		      double tfast, double jitter, bool use_binomial)
{
  //subtract the baseline off of the reference pulse
  //GetBaseline(singlepe, 100, true);
  
  //make a new TGraph if output==0
  if(!output){
    const int npts = 250*9; // -1 to 8 microseconds, 250 samps/microsec
    output = new TGraph(npts, singlepe->GetX(), singlepe->GetY());
  }
  //first determine when each photon ought to arrive
  std::vector<double> initial_times;
  std::vector<double> amplitudes;
  TRandom3 randgen(0);
  //choose how many are in the prompt part from binomial statistics
  int nfast = (int)(nphotons * fprompt);
  if(use_binomial)
    nfast = randgen.Binomial(nphotons, fprompt);
  int nslow = nphotons - nfast;
  //the arrival time is from an exponential disttribution 
  //then convolve with a gaussian jitter
  for(int i=0; i < nfast; i++)
    initial_times.push_back((randgen.Exp(tfast)+t0) * randgen.Gaus(1,jitter));
  for(int i=0; i < nslow; i++)
    initial_times.push_back((randgen.Exp(tslow)+t0) * randgen.Gaus(1,jitter));
  
  //the amplitude is smeared by the resolution
  for(int i=0; i < nphotons; i++)
    amplitudes.push_back(randgen.Gaus(1,spe_resolution));
  
  //fill the pulse with our generated photons
  double* t = output->GetX();
  double t_first = t[0];
  double dt = t[1]-t[0];
  double* y = output->GetY();
  double* yinput = singlepe->GetY();
  double tmin = singlepe->GetX()[0];
  for(int samp=0; samp < output->GetN(); samp++){
    y[samp] = 0;
    for(size_t pulse = 0; pulse < initial_times.size(); pulse++){
      double eval_time = t[samp] - initial_times[pulse];
      if( eval_time < tmin ) continue;
      //instead of using Eval, get an exact sample index
      y[samp] += yinput[(size_t)((eval_time-t_first)/dt)] * amplitudes[pulse];
      //y[samp] += singlepe->Eval(eval_time) * amplitudes[pulse];
    }
  }
  return output;
}
    
TGraph* GetNormalizedSPETemplate(int channel)
{
  TGraph* singlepe = GetAverageGraph("/data/test_processing/Run000281.root",
				     channel);
  //subtract the baseline
  GetBaseline(singlepe, 100, true);
  //integrate the singlepe over the laser window
  //find the laser window from the events tree
  TTree* LaserEventsTree = GetEventsTree("/data/test_processing/Run000281.root");
  EventData* ev = 0;
  LaserEventsTree->SetBranchAddress("EventData", &ev);
  LaserEventsTree->GetEntry(0);
  int start = ev->GetChannelByID(channel)->regions[0].start_index;
  int end = ev->GetChannelByID(channel)->regions[0].end_index;
  double* y = singlepe->GetY();
  double integral = std::accumulate(y+start, y+end, 0.);
  //normalize the template to unit integral
  for(int i=0; i < singlepe->GetN(); i++){
    y[i] /= integral;
  }
  return singlepe;

}

void TestCorrectionWithNphotons(int channel, int ntrials, int npoints,
				int min_photons, int photons_step)
{
  //define the start and end indices for integration (-0.05 to 7 microseconds)
  const int start_index = 238;
  const int end_index = 2001;
  const int fprompt_start_index = 238;
  const int fprompt_end_index = 275;
  //first load the singlepe template
  TGraph* singlepe = GetNormalizedSPETemplate(channel);
  TGraph* output = 0;
  //create the output ntuple
  std::stringstream fname;
  fname<<"analysis/LightYieldCorrection_"<<time(0)<<".root";
  std::cout<<"Saving output to file "<<fname.str()<<std::endl;
  TFile f(fname.str().c_str(),"RECREATE");
  TNtuple tree("yield","Integral vs NPE","npe:integral:fprompt_measured");
  for(int point = 0; point < npoints; point++){
    int nphotons = min_photons + point * photons_step;
    for(int trial = 0; trial < ntrials; trial++){
      if(trial%100 == 0){
	std::cout<<"Simulating "<<nphotons<<" photons,  trial "
		 <<trial<<std::endl;
      }
      output = MakeFakePulse(singlepe, nphotons, output, 0., -0.18,
			     0.25, 1.4,0.007,0.,false);
      double* y = output->GetY();
      //integrate the fake pulse 
      double integral = std::accumulate(y+start_index, y+end_index, 0.);
      double fprompt_measured = 
	std::accumulate(y+fprompt_start_index, y+fprompt_end_index, 0.) /
	integral;
      tree.Fill(nphotons, integral, fprompt_measured);
    }
    //save after every batch
    tree.Write("",TObject::kWriteDelete);
  }

  f.Close();
}

void TestCorrectionWithTimeParams(int channel, int ntrials,
				  double t0center, double t0width,
				  double fpromptcenter, double fpromptwidth,
				  double tslowcenter, double tslowwidth)
{
  //define the start and end indices for integration (-0.05 to 7 microseconds)
  const int start_index = 238;
  const int end_index = 2001;
  const int fprompt_start_index = 238;
  const int fprompt_end_index = 275;
  //define the fixed parameters
  const int nphotons = 300;
  const double spe_resolution = 0.;
  
  //first load the singlepe template
  TGraph* singlepe = GetNormalizedSPETemplate(channel);
  TGraph* output = 0;
  //create the output ntuple
  std::stringstream fname;
  fname<<"analysis/LightYieldCorrection_"<<time(0)<<".root";
  std::cout<<"Saving output to file "<<fname.str()<<std::endl;
  TFile f(fname.str().c_str(),"RECREATE");
  TNtuple tree("yield","Integral vs time params",
	       "npe:integral:fprompt_measured:t0:fprompt:tslow");
  TRandom3 myrand(0);
  for(int i=0; i < ntrials; i++){
    //choose all the appropriate values from random gaussians
    double t0 = myrand.Gaus(t0center, t0width);
    double fprompt = myrand.Gaus(fpromptcenter,fpromptwidth);
    double tslow = myrand.Gaus(tslowcenter, tslowwidth);
    if(i%100 == 0)
      std::cout<<"Simulating trial "<<i<<std::endl;
    output = MakeFakePulse(singlepe, nphotons, output, spe_resolution,
			   t0, fprompt, tslow);
    double* y = output->GetY();
    double integral = std::accumulate(y+start_index, y+end_index, 0.);
    double fprompt_measured = 
	std::accumulate(y+fprompt_start_index, y+fprompt_end_index, 0.) /
	integral;
    tree.Fill(nphotons, integral, fprompt_measured, t0, fprompt, tslow);
    //save the tree intermittently
    if(i%100 == 0)
      tree.Write("",TObject::kWriteDelete);
  }
  //save the tree
  tree.Write("",TObject::kWriteDelete);
}

TGraphErrors* PlotYieldvsNPE(const char* file, int npoints, int min_photons,
			     int photons_step, const char* treename)
{
  TFile fin(file);
  if(!fin.IsOpen())
    return 0;
  TTree* tree = (TTree*)(fin.Get(treename));
  if(!tree)
    return 0;
  
  TH1* hist = 0;
  double x[npoints];
  double y[npoints];
  double ye[npoints];
  for(int i=0; i < npoints; i++){
    int nphotons = min_photons + i * photons_step;
    x[i] = nphotons;
    char photoncut[100];
    sprintf(photoncut, "npe == %d", nphotons);
    tree->Draw("npe/integral",photoncut);
    hist = (TH1*)(gROOT->FindObject("htemp"));
    if(hist){
      y[i] = hist->GetMean();
      ye[i] = hist->GetRMS();
    }
  }
  TGraphErrors* g = new TGraphErrors(npoints, x, y, 0, ye);
  g->SetTitle("Light Yield Correction vs NPE Generated");
  g->GetXaxis()->SetTitle("NPE Generated");
  g->GetYaxis()->SetTitle("Light Yield Correction Factor");
  g->Draw("alpe");
  g->Fit("pol1");
  
  return g;
}
