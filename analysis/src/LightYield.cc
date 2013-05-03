#include "LightYield.hh"
#include "utilities.hh"
#include "EventData.hh"

#include "TGraphErrors.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TTree.h"
#include "TPad.h"
#include "TF1.h"
#include "TSpectrum.h"
#include "TString.h"
#include "TRegexp.h"
#include "TLine.h"
#include "TLegend.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TTree.h"
#include "TSystem.h"
#include "TLine.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <map>

using namespace std;

int LightYieldGraph::AddRun(int run, double epeak, int nbins,
	     double emin, double emax)
{
  //construct the filename
  stringstream filename;
  filename<<"/data/test_processing/single_pe/Run"<<setw(6)<<setfill('0')
	  <<run<<".root";
  return AddFile(filename.str().c_str(), epeak, nbins, emin, emax);

}

int LightYieldGraph::AddRuns(const vector<int>& runs, double epeak, int nbins, 
	      double emin, double emax)
{
  int n = 0;
  for(size_t i=0; i< runs.size(); i++){
    n += AddRun(runs[i], epeak, nbins, emin, emax);
  }
  return n;
}

int LightYieldGraph::AddFile(const char* filename, double epeak, int nbins, 
	     double emin, double emax)
{
  cout<<"\n***********************************************\n";
  cout<<"Loading file "<<filename<<endl;
  if(histo) histo->Delete();
  histo = DrawSpectrum(filename,nbins,emin,emax);
  if(!histo) return 0;
  TTree* Events = GetEventsTree(filename);
  if(!Events) return 0;
  //find the start/end time of the event
  double start = Events->GetMinimum("timestamp");
  double end = Events->GetMaximum("timestamp");
  //fit the peak
  double yield, yield_err;
  int error = FitPhotopeak(histo, epeak, yield, yield_err);
  if(gPad){
    gPad->Modified();
    gPad->Update();
    gSystem->ProcessEvents();
    gSystem->Sleep(100);
  }
  //histo->Delete();
  
  if(!error){
    //insert the results
    _x.push_back((end+start)/2.);
    _ex.push_back((end-start)/2.);
    _y.push_back(yield);
    _ey.push_back(yield_err);
    return 1;
  }
  return 0;
}
  
int LightYieldGraph::FitPhotopeak(TH1* h, double epeak, 
				  double& yield, double& yield_err)
{
  TSpectrum spec(1);
  spec.Search(h);
  double searchpoint = spec.GetPositionX()[0];
  
  h->GetXaxis()->SetTitle("Energy [photoelectrons]");
  h->Draw("e");
  h->GetYaxis()->SetRangeUser(0, 1.1*h->GetMaximum());
  TH1* bg = spec.Background(h);
  bg->SetBit(TObject::kCanDelete, true);
  TH1* sig = (TH1*)(h->Clone());
  sig->SetBit(TObject::kCanDelete,true);
  sig->SetLineColor(kGreen);
  
  sig->Add(bg,-1);
  sig->Draw("same e");
  sig->Fit("gaus","m","same e",searchpoint*0.8,searchpoint*1.2);
  TF1* gaus = sig->GetFunction("gaus");
  if(!gaus){
    cerr<<"There was a problem fitting the function!\n";
    return 1;
  }
  gaus->SetLineColor(kGreen);
  
  bg->SetLineColor(kRed);
  bg->Draw("same e");
  
  TLine* line = new TLine(gaus->GetParameter(1),0,
			  gaus->GetParameter(1),h->GetMaximum());
  line->SetLineColor(kBlue);
  line->SetLineWidth(2);
  line->Draw();
  

  yield = searchpoint/epeak;
  yield_err = 0;
  
  cout<<"Results from TSpectrum: \n\t"
      <<"Peak = "<<spec.GetPositionX()[0]<<" p.e.; Light Yield = "
      <<yield<<" p.e./keV"<<endl;
  if(gaus){
    yield = gaus->GetParameter(1)/epeak;
    yield_err = gaus->GetParError(1)/epeak;
    cout<<"Results from BG Subtracted Gaus Fit: \n\t"
	<<"Peak = "<<gaus->GetParameter(1)<<" p.e.; Light Yield = "
	<<yield<<" +/- "<<yield_err<<" p.e./keV"<<endl;
    yield_err = max(yield_err, TMath::Abs(yield-spec.GetPositionX()[0]/epeak));
    
  }
  TLegend* leg = new TLegend(.6,.6,.9,.9);
  leg->AddEntry(h,"Raw Spectrum","lpe");
  leg->AddEntry(bg,"Background","lpe");
  leg->AddEntry(sig,"Signal","lpe");
  char title[20];
  sprintf(title,"Yield = %.2f pe/keV",yield);
  leg->AddEntry(line, title ,"l");
  leg->Draw();

  gPad->Update();
  return 0;
}

  
TGraphErrors* LightYieldGraph::DrawGraph()
{
  TGraphErrors* graph = new TGraphErrors(_x.size(),&_x[0], &_y[0],
					 &_ex[0], &_ey[0]);
  graph->SetName("lightyieldgraph");
  graph->SetTitle("Light Yield vs Time");
  graph->Draw("ape");
  TAxis* xax = graph->GetXaxis();
  xax->SetTitle("Run Time");
  xax->SetTimeDisplay(1);
  xax->SetTimeFormat("%m/%d");
  xax->SetTimeOffset(1,"gmt");
  xax->SetNdivisions(710);
  TAxis* yax = graph->GetYaxis();
  yax->SetTitle("Light Yield [pe/keV]");
  yax->SetTitleOffset(1.2);
  graph->Draw("ape");
  return graph;
}

TGraphErrors* PlotNa22LightYield()
{
  //define the list of runs
  //use only runs where the collimator is at midplane
  std::vector<int> runs;
  runs.push_back(311);
  runs.push_back(314);
  runs.push_back(331);
  runs.push_back(337);
  runs.push_back(340);
  runs.push_back(342);
  runs.push_back(348);
  runs.push_back(349);
  runs.push_back(350);
  runs.push_back(351);
  runs.push_back(352);  
  runs.push_back(353);  
  runs.push_back(354);  
  runs.push_back(355);  
  runs.push_back(356);  
  runs.push_back(357);  
  runs.push_back(358);  
  runs.push_back(364);  
  runs.push_back(365);  
  runs.push_back(385);  
  runs.push_back(388);  
  runs.push_back(392);  
  runs.push_back(395);  
  runs.push_back(416);  
  runs.push_back(423);
  runs.push_back(434);
  runs.push_back(498);  
  runs.push_back(506);  
  runs.push_back(508);  
  runs.push_back(523);  
  runs.push_back(542);  
  runs.push_back(547);  
  runs.push_back(552);  
  runs.push_back(553);  
  runs.push_back(554);  
  runs.push_back(555);  
  runs.push_back(556);  
  runs.push_back(557);  
  runs.push_back(565);  
  runs.push_back(567);  
  
  LightYieldGraph ly;
  ly.AddRuns(runs, 511., 200, 1000, 2500);
  TGraphErrors* e = ly.DrawGraph();
  DrawOperationsBoxes();
  return e;
}
  
enum SPEFUNC_PARAMS {PED_AMP=0, PED_SIGMA, SPE_AMP, SPE_MEAN, SPE_SIGMA, 
		     EXPO_AMP, EXPO_SLOPE, TWOPE_RATIO};

double singlepe_fitfunc(double* x, double* par)
{
  double ped_amp =        par[PED_AMP];
  double ped_sigma =      par[PED_SIGMA];
  double spe_amp =        par[SPE_AMP];
  double spe_mean =       par[SPE_MEAN];
  double spe_sigma =      par[SPE_SIGMA];
  double expo_amp =       par[EXPO_AMP];
  double expo_slope =     par[EXPO_SLOPE];
  double twope_ratio =    par[TWOPE_RATIO];
  
  return ped_amp * TMath::Gaus(*x, 0., ped_sigma) + 
    spe_amp * TMath::Gaus(*x, spe_mean, 
			  sqrt(ped_sigma*ped_sigma + spe_sigma*spe_sigma)) + 
    twope_ratio * spe_amp * TMath::Gaus(*x, 2.*spe_mean,
			  sqrt(ped_sigma*ped_sigma+2*spe_sigma*spe_sigma)) +
    expo_amp * exp(-(*x)/expo_slope);
}

int LightYieldGraph::SetAliasesFromLocalData(TTree* Events, bool draw)
{
  //histogram parameters
  const int nbins = 120;
  const double xmin = 10;
  const double xmax = 250;
  //fill spectra of single photoelectrons in the single_pe vector
  std::map<int,TH1D*> spectra;
  EventData* event = 0;
  Events->SetBranchAddress("EventData", &event);
  std::cout<<"Finding single photoelectron spectra from "<<Events->GetEntries()
	   <<" triggers."<<std::endl;
  for(int i=0; i < Events->GetEntries(); i++){
    Events->GetEntry(i);
    for(size_t ch=0; ch < event->channels.size(); ch++){
      ChannelData& chdata = event->channels[ch];
      int id = chdata.channel_id;
      if(id < 0) continue;
      if( spectra.find(id) == spectra.end()){
	//create a new histogram for this channel
	char name[100];
	sprintf(name, "spe_spectrum_%d",id);
	spectra[id] = new TH1D(name,name,nbins,xmin,xmax);
      }
      for(size_t spe=0; spe < chdata.single_pe.size(); spe++){
	spectra[id]->Fill(chdata.single_pe[spe].integral);
      }
    }
  }
  
  std::cout<<"Found spectra for "<<spectra.size()<<" channels"<<std::endl;
  std::map<int, TH1D*>::iterator it = spectra.begin();
  std::stringstream sumalias("");
  for(it = spectra.begin() ; it != spectra.end(); it++){
    // find the means of all the histograms
    TH1* hist = (it->second);
    TSpectrum spec(10,0.5);
    spec.Search(hist,2,"",0.02);
    int n = spec.GetNPeaks();
    float* x = spec.GetPositionX();
    float mean = *(std::min_element(x, x+n));
    
    TF1* fitfunc = (TF1*)(gROOT->GetFunction("spe_fitfunc"));
    if(!fitfunc)
      fitfunc = new TF1("spe_fitfunc",singlepe_fitfunc,xmin,xmax,8);
    fitfunc->SetParNames("PED_AMP","PED_SIGMA","SPE_AMP","SPE_MEAN","SPE_SIGMA",
			 "EXPO_AMP","EXPO_SLOPE", "TWOPE_RATIO");
    fitfunc->SetParameters(hist->GetBinContent(1),
			   5.,
			   hist->GetBinContent(hist->FindBin(mean)),
			   mean,
			   mean/3.,
			   100,
			   10,
			   0.01);
    fitfunc->SetParLimits(PED_AMP,0,hist->GetEntries());
    fitfunc->SetParLimits(PED_SIGMA,0,40);
    fitfunc->SetParLimits(SPE_AMP,hist->GetEntries()/10000.,2.*hist->GetEntries());
    fitfunc->SetParLimits(SPE_MEAN,xmin,xmax);
    fitfunc->SetParLimits(SPE_SIGMA,0,xmax);
    fitfunc->SetParLimits(EXPO_AMP,0, hist->GetBinContent(1));
    fitfunc->SetParLimits(EXPO_SLOPE,0,2.*xmax);
    fitfunc->SetParLimits(TWOPE_RATIO,0,0.5);
    fitfunc->SetLineColor(kBlue);
    hist->Fit(fitfunc,"QM");			
    
    mean = fitfunc->GetParameter(SPE_MEAN);
    //store the results
    _chan_spe[(it->first)].push_back(mean);
    _chan_spe_err[(it->first)].push_back(fitfunc->GetParError(SPE_MEAN));
    
    
    TLine* line = new TLine(mean,1,mean,fitfunc->GetParameter(SPE_AMP));
    line->SetLineColor(kGreen);
    line->SetLineWidth(2);
    hist->GetListOfFunctions()->Add(line);
    
    if(!sumalias.str().empty())
      sumalias<<"+";
    std::stringstream aliasname(""), aliasdef("");
    aliasname<<"npe"<<(it->first);
    sumalias<<aliasname.str();
    aliasdef<<"(-channels["<<(it->first)<<"].regions[1].integral/"<<mean<<")";
    std::cout<<"Setting alias "<<aliasname.str()<<" = "<<aliasdef.str()
	     <<std::endl;
    Events->SetAlias(aliasname.str().c_str(), aliasdef.str().c_str());
  }
  Events->SetAlias("sumspec",sumalias.str().c_str());

  if(draw){  
    if(!gPad) new TCanvas;
    //go to the top level
    while(gPad->GetMother() != gPad) gPad->GetMother()->cd();
    TCanvas* can = (TCanvas*)gPad;
    can->Clear();
    DividePad(can, spectra.size());
    it = spectra.begin();
    for(size_t i=0; i < spectra.size(); i++){
      can->cd(i+1);
      TH1D* hist = it->second;
      hist->Draw();
      gPad->SetLogy();
      //gPad->Update();
      it++;
    }
    can->cd(0);
    can->Modified();
    can->Update();
    //std::string dummy;
    //std::cout<<"Press <enter> to continue"<<std::endl;
    //std::getline(std::cin, dummy);
  }
  for(it = spectra.begin(); it != spectra.end(); it++){
    delete (it->second);
  }
  return 0;
}

TH1* LightYieldGraph::DrawSpectrum(const char* filename, int nbins, 
				   double xmin, double xmax)
{
  TTree* Events = GetEventsTree(filename);
  if(!Events)
    return 0;
  SetAliasesFromLocalData(Events,true);
  TString title=TString("Spectrum, ") + 
    TString(filename)(TRegexp("Run......"));
  TH1D* hist = new TH1D("spectrum",title, nbins, xmin, xmax);
  Events->Draw("sumspec>>spectrum","!saturated && channels[].baseline.found_baseline","e");
  hist->GetXaxis()->SetTitle("Energy [photoelectrons]");
  return hist;
}

TGraphErrors* LightYieldGraph::DrawChannelSpeGraph(int channel)
{
  //assume our graphs have the same number of points...
  TGraphErrors* graph = new TGraphErrors(_x.size(), &_x[0], &(_chan_spe[channel][0]),
					 &_ex[0], &(_chan_spe_err[channel][0]));
  char name[100];
  sprintf(name, "spegraph%d", channel);
  graph->SetName(name);
  sprintf(name, "SPE Mean vs Time for Channel %d",channel);
  graph->SetTitle(name);
  graph->Draw("ape");
  TAxis* xax = graph->GetXaxis();
  xax->SetTitle("Run Time");
  xax->SetTimeDisplay(1);
  xax->SetTimeFormat("%m/%d");
  xax->SetTimeOffset(1,"gmt");
  xax->SetNdivisions(710);
  TAxis* yax = graph->GetYaxis();
  yax->SetTitle("SPE Mean [counts*samples]");
  yax->SetTitleOffset(1.2);
  graph->Draw("ape");
  return graph;
}
