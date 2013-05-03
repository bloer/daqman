#include "TH1.h"
#include "TF1.h"
#include "TMath.h"
#include "TSpectrum.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TCut.h"
#include <algorithm>
#include <string>
#include <iomanip>
#include <iostream>


#define NPEAKS 10

enum PARAMETERS {CONSTANT, LAMBDA, MEAN, SIGMA, AMP1, L1 , NPAR, AMP2, L2 };


const char* names[] = {"CONSTANT","LAMBDA", "SPE_MEAN", "SPE_SIGMA", "AMP1", "L1", "AMP2","L2","NPAR"};


Double_t background_func(Double_t* x, Double_t* params)
{
  return exp(params[AMP1] + x[0]*params[L1]);
  //  + exp(params[AMP2] + x[0]*params[L2]);
}

Double_t signal_func(Double_t* x, Double_t* params)
{
  double signal = 0;
  
  for(int i=1; i<=NPEAKS; i++){
    signal += params[CONSTANT] * TMath::Poisson(i,params[LAMBDA]) 
      * TMath::Gaus(x[0], i*params[MEAN], sqrt(i)*params[SIGMA], true);
  }

  return signal;
}

Double_t SPEFunc(Double_t* x, Double_t* params)
{
  return signal_func(x, params) + background_func(x,params);
}

TCut get_time_cut(TTree* tree, TCut chan_cut = "")
{
  tree->Draw("channels.pulses.start_index >> h_start_index",chan_cut);
  TH1F* h_start_index = (TH1F*)(gROOT->FindObject("h_start_index"));
  int centerbin = h_start_index->GetMaximumBin();
  int lowbin = centerbin, highbin = centerbin;
  while( h_start_index->GetBinContent(lowbin-1) < 
	 h_start_index->GetBinContent(lowbin) ) lowbin--;
  while( h_start_index->GetBinContent(highbin+1) < 
	 h_start_index->GetBinContent(highbin) ) highbin++;
  
  char cutstr[100];
  sprintf(cutstr, "channels.pulses.start_index > %.1f && channels.pulses.start_index < %.1f", h_start_index->GetBinLowEdge(lowbin),
	  h_start_index->GetBinLowEdge(highbin+1) );
  return TCut(cutstr);
}
  

int FitSPE(TH1* spe, int ntriggers) 
{
  double fitmin, fitmax;
  double params[NPAR];
  //find the likely place for the single p.e. peak
  params[MEAN] = spe->GetMean();
  params[SIGMA] = spe->GetRMS();
  int bin = spe->FindBin(params[MEAN]);
  if(spe->GetMaximumBin() == 1){
    spe->Fit("gaus","Q0","",spe->GetBinCenter(bin), 
	     spe->GetBinCenter(bin+20));
  }
  else{
    spe->Fit("gaus","Q0","",spe->GetBinCenter(bin-10), 
	     spe->GetBinCenter(bin+10));
  }
  TF1* gaus = spe->GetFunction("gaus");
  if(gaus->GetParameter(1) > 0)
    params[MEAN] = gaus->GetParameter(1);
  if(gaus->GetParameter(2) > 0 && gaus->GetParameter(2) < params[SIGMA])
    params[SIGMA] = gaus->GetParameter(2);
  
  params[LAMBDA] = 0.5;
  //find the maximum fit range
  bin = spe->GetNbinsX();
  while(spe->GetBinContent(--bin) < 2) {}
  fitmax = spe->GetBinCenter(bin);
  
  spe->Fit("expo","Q0","",spe->GetBinCenter(bin), spe->GetBinCenter(bin-10));
  
  //find the best estimate for the exponentials
  //the first is from the first few bins
  bin = spe->FindBin(params[MEAN]) - 5;
  double compval = spe->GetBinContent(bin--);
  while(spe->GetBinContent(bin) < compval && spe->GetBinContent(bin) != 0){
    compval = spe->GetBinContent(bin--);
  }
  if(spe->GetBinContent(bin) == 0){
    fitmin = spe->GetBinCenter(bin+1);
  }
  else{
    ++bin;
    //now find the max of the turnover
    int minbin = bin;
    compval = spe->GetBinContent(bin--);
    while(spe->GetBinContent(bin) > compval && bin > 0 ){
      compval = spe->GetBinContent(bin--);
    }
    ++bin;
    spe->Fit("expo","Q0","",spe->GetBinCenter(bin), spe->GetBinCenter(minbin));
    params[AMP1] = spe->GetFunction("expo")->GetParameter(0);
    params[L1] = spe->GetFunction("expo")->GetParameter(1);
    fitmin = spe->GetBinCenter(bin+1);
  }
  
  //estimate the peak heights
  
  params[CONSTANT] = ntriggers * spe->GetBinWidth(1);
  fitmin = spe->GetBinCenter(4);
  
  TF1* spefunc = new TF1("spefunc", SPEFunc, fitmin, fitmax, NPAR);
  
  
  spefunc->SetParameters(params);
  for(int i=0; i<NPAR; i++)
    spefunc->SetParName(i, names[i]);
  spefunc->SetParLimits(CONSTANT, 0 , params[CONSTANT]*100.);
  spefunc->SetParLimits(LAMBDA, 0, 2);
  spefunc->SetParLimits(MEAN, std::max(params[MEAN]-params[SIGMA],0.), 
			params[MEAN]+params[SIGMA]);
  spefunc->SetParLimits(SIGMA, 0, spe->GetRMS());
    
  spefunc->SetParLimits(L1, -30, 0);
  spefunc->SetParLimits(AMP1, -30, 30);
  spefunc->SetParLimits(L2, -30, 0);
  spefunc->SetParLimits(AMP2, -30, 30);
    
  //spefunc->FixParameter(CONSTANT, ntriggers * spe->GetBinWidth(1));
  
  //return 0;
  spefunc->SetLineStyle(1);
  spefunc->SetLineColor(kBlue);
  int result = spe->Fit(spefunc,"MRNI");
  spefunc->DrawCopy("same");
  std::cout<<"Fit results: \n"<<
    "\t Chi2/NDF = "<<spefunc->GetChisquare()<<"/"<<spefunc->GetNDF()
	   <<"\n\t Prob = "<<spefunc->GetProb()<<std::endl;
  for(int i=0; i<NPAR; i++)
    params[i] = spefunc->GetParameter(i);
  TF1* background = new TF1("background",background_func,fitmin,fitmax,NPAR);
  background->SetLineColor(kRed);
  background->SetParameters(spefunc->GetParameters());
  background->DrawCopy("same");
  
  TF1* signal = new TF1("signal",signal_func,fitmin,fitmax,NPAR);
  signal->SetLineColor(kGreen); 
  signal->SetParameters(spefunc->GetParameters());
  signal->DrawCopy("same");
  
  TF1* apeak = new TF1("apeak","[0]*TMath::Gaus(x,[1],[2],1)",fitmin, fitmax);
  apeak->SetLineColor(kMagenta);
  apeak->SetLineStyle(2);
  for(int i=1; i<=NPEAKS; i++){
    apeak->SetParameters(params[CONSTANT]*TMath::Poisson(i,params[LAMBDA]),
			 i*params[MEAN], sqrt(i)*params[SIGMA]);
    apeak->DrawCopy("same");
  }
  cout<<"Entries in Events tree: "<<ntriggers<<"; Poisson trials: "
      <<params[CONSTANT] / spe->GetBinWidth(1)<<std::endl;
  
  return result;
}

int ProcessSPEFile(const char* fname, Long_t roi = -1, int channel = -1)
{
  TCanvas* c = new TCanvas;
  c->SetLogy();
  c->SetTitle(fname);
  static bool loaded = false;
  if(!loaded){
    gROOT->ProcessLine(".L lib/libDict.so");
    loaded = true;
  }
  
  TFile* fin = new TFile(fname);
  if(!fin->IsOpen()){
    std::cerr<<"Unable to open file "<<fname<<std::endl;
    return 1;
  }
  
  TTree* Events = (TTree*)(fin->Get("Events"));
  if(!Events){
    std::cerr<<"Unable to load Events tree from file "<<fname<<std::endl;
    return 2;
  }
  TString data_source;
  if(roi == -1) data_source = "channels.pulses.integral";
  else data_source = TString("-channels.regions[") + roi
	 + TString("].integral");
  Events->Draw(data_source+" >> htemp",data_source+" > 0");
  
  TH1* htemp = (TH1*)(gROOT->FindObject("htemp"));
  double emax = htemp->GetMean()*5;
  
  TCut min_en = (data_source+" > 0").Data();
  char chstring[100];
  sprintf(chstring,data_source+" < %.0f",emax);
  TCut max_en = chstring;
  sprintf(chstring,"channels.channel_id == %d",channel);
  TCut chan_cut = (channel == -1 ? "" : chstring);

  TCut time_cut = (roi == -1 ? get_time_cut(Events, chan_cut) : "" );
  
  TCut total_cut = min_en && max_en && time_cut && chan_cut;
    
  Events->Draw(data_source+" >> hspec",total_cut,"e");
  TH1* hspec = (TH1*)(gROOT->FindObject("hspec"));
  

  int val = FitSPE(hspec, Events->GetEntries());
  
  return val;
}
  
