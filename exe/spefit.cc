/* 
Fit SPE spectra on individual channels and upload results to scenedb
@author Charles Cao
*/


//include all header files as in laserrun except ones know won't need
#include "TNamed.h"
#include "TString.h"
#include "TCanvas.h"
#include "EventData.hh"
#include "TRint.h"
#include "utilities.hh"
#include "RootGraphix.hh"
#include "CommandSwitchFunctions.hh"
#include "ChanFitSettings.hh"
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "TTree.h"
#include "TFile.h"
#include "TF1.h"
#include "TH1F.h"

using namespace std;


const int mNCHANS = 2; // No. of pmt channels
const double max_spe_length = 0.12; // spe length cut

void PrintResults(std::map<int, TH1F*>* spectra){
  cout<<"Fit results are:"<<endl;
  cout<<"Ch\t"
      <<"mean\t"
      <<"mean_er\t"
      <<"sigma\t"
      <<"Chi2\t"
      <<"NDF\t"
      <<"Height1\t"
      <<"Height2\t"
      <<"Height3\t"
      <<"Height4"<<endl;
  map<int, TH1F*>::iterator it = spectra->begin();
  for(; it != spectra->end(); it++){
    TH1F* hist = it->second;
    TF1* spefunc = hist->GetFunction("spef");
    if(spefunc){
      int Channel = it->first;
      double Pdfmean = spefunc->GetParameter(0);
      double Pdfmean_err = spefunc->GetParError(0);
      double Pdfsigma = spefunc->GetParameter(1);
      double Chi2 = spefunc->GetChisquare();
      int Ndf = spefunc->GetNDF();
      double Height1 = spefunc->GetParameter(2);
      double Height2 = spefunc->GetParameter(3);
      double Height3 = spefunc->GetParameter(4);
      double Height4 = spefunc->GetParameter(5);
      cout<<Channel<<"\t"
	  <<setprecision(4)<<Pdfmean<<"\t"
	  <<setprecision(2)<<Pdfmean_err<<"\t"
	  <<setprecision(3)<<Pdfsigma<<"\t"
	  <<setprecision(4)<<Chi2<<"\t"
	  <<Ndf<<"\t"
	  <<setprecision(2)<<Height1<<"\t"
	  <<setprecision(2)<<Height2<<"\t"
	  <<setprecision(2)<<Height3<<"\t"
	  <<setprecision(2)<<Height4<<endl;
    }
  }
  cout<<endl;
}

void DrawSpectra(map<int, TH1F*>* spectra, TCanvas* c){
  c->Clear();
  DividePad(c,spectra->size());
  int padn=1;
  for( std::map<int,TH1F*>::iterator it = spectra->begin(); it != spectra->end(); it++){
    c->cd(padn++);
    gPad->SetLogy();
    TH1* hist = (it->second);
    hist->Draw();
  }
  c->cd(0);    
  //c->Draw();
  c->Update();
}

void FitSPE(TH1F* spe, ChanFitSettings& CFS){
  float height1_start_value = spe->GetMaximum();
  float mean_start_value = spe->GetMaximumBin()+spe->GetXaxis()->GetXmin();
  //cout<< height1_start_value << ",, "<<mean_start_value<<endl;
  // Define spe fit function, sum of gaussians from 1 p.e. up to 4 p.e.
  TF1 *spef = new TF1("spef", "[2]*exp(-(x-[0])^2/(2*[1]^2))+[3]*exp(-(x-[0]*2)^2/(2*(2*[1]^2)))+[4]*exp(-(x-[0]*3)^2/(2*(3*[1]^2)))+[5]*exp(-(x-[0]*4)^2/(2*(4*[1]^2)))", mean_start_value, CFS.range_max);
  
  // Fit once with user specified range
  //spef->SetParameters(CFS.mean_start_value, CFS.sigma_start_value, CFS.constant_start_value, 0.1*CFS.constant_start_value, 0.01*CFS.constant_start_value, 0.001*CFS.constant_start_value);
  spef->SetParameters(mean_start_value, CFS.sigma_start_value, height1_start_value, 0.1*height1_start_value, 0.01*height1_start_value, 0.001*height1_start_value);
  spef->SetParNames("spe_mean","spe_sigma","height1","height2","height3","height4");
  spef->SetParLimits(1, 0, 20);
  spe->Fit("spef","MRE");
  
  double mean = spef->GetParameter(0);
  double sigma = spef->GetParameter(1);
  double height1 = spef->GetParameter(2);
  double height2 = spef->GetParameter(3);
  double height3 = spef->GetParameter(4);
  double height4 = spef->GetParameter(5);

  // Modify fit range and start values, and fit again
  double fitmin = mean - sigma;
  if (fitmin<CFS.pedrange_max) fitmin = CFS.pedrange_max; 
  spef->SetRange(fitmin, 4.0*mean);
  spef->SetParameters(mean,sigma,height1,height2,height3,height4);
  spef->SetLineColor(kRed);
  spe->Fit("spef","MRE");
  return;
}

void UpdateDatabase(map<int,TH1F*>* spectra, TTree* Events){
  //dummy function
  return;
}

void QueryUser(map<int, TH1F*>* spectra, RootGraphix* root, TTree* Events, TCanvas* c){
  bool showresults=true;
  string response = "";
  while(response != "q"){
    if(showresults){
      RootGraphix::Lock lock = root->AcquireLock();
      DrawSpectra(spectra, c);
      PrintResults(spectra);
    }
    showresults=false;
    cout<<"Enter \n q) to quit, \n"
	<<" r) to reprint the results, \n"
	<<endl;
    cin >> response;
    if(response == "q"){
      cout<<"Aborting without saving results."<<endl;
      break;
    }
    if(response == "r"){
      showresults=true;
    }
    
  }
  if(c) delete c;
}

int ProcessRun(const char* fname){
  //open the root file
  RootGraphix root;
  root.Initialize();
  TTree* Events = GetEventsTree(fname);
  if (!Events) {
    cout<<"!Events"<<endl; 
    return -1;
  }
  EventData* event = 0;
  Events->SetBranchAddress("event",&event);
  Events->GetEntry(0);  
  // Print run id, no. of channels and no. of events
  int run = event->run_id;
  if(run < 0){
    std::cerr<<"Unable to read runid from rootfile! Aborting."<<std::endl;
    return -1;
  }
  int nChans = event->channels.size();
  cout<<"Processing Run "<<run<<" with "<<nChans<<" channels."<<endl;
  cout<<"There are "<<Events->GetEntries()<<" events in this run."<<endl;

  // Load fit parameters
  ParameterList* ChanSettingsHandler = new ParameterList("ChannelsSettings","Stores channels of fit settings");
  ChanFitSettings ChannelsSettings[mNCHANS];
  for(int j=0; j< mNCHANS; j++){
    stringstream name;
    stringstream help;
    name<<"chan"<<j;
    help<<"Fit settings for channel "<<j;
    ChanSettingsHandler->RegisterParameter(name.str(),ChannelsSettings[j], help.str());
  }
  ifstream CFSConfig("cfg/speCFS.cfg");
  ChanSettingsHandler->ReadFrom(CFSConfig);
  CFSConfig.close();

  const int nbins=240;
  const double start=-40, end=200; 
  std::map<int,TH1F*> spectra;
  
  // Populate histograms
  for(int i=0; i < Events->GetEntries(); i++){
    if(i%5000 == 0) cout<<"Processing Entry "<<i<<endl;
    Events->GetEntry(i);
    // Loop over all pmt channels
    for(int j=0; j < mNCHANS; j++){
      ChannelData* channel = &(event->channels.at(j));
      if(channel->channel_id < 0){
	cout<<"channel->channel_id < 0"<<endl;
	continue;
      }
      if(!channel->baseline.found_baseline){
	continue;
      }
      std::map<int,TH1F*>::iterator mapit = spectra.find(channel->channel_id);
      TH1F* histo=0;
      if(mapit == spectra.end()){
	TString name = "channel";
	name += channel->channel_id;
	cout<<"Creating new histogram with name "<<name<<endl;
	histo = new TH1F(name,name,nbins,start,end);
	spectra.insert(std::make_pair(channel->channel_id, histo));
      }
      else{
	histo = (mapit->second);
      }
      //fill the histogram
      if(!histo){
	cerr<<"Null pointer passed!\n";
	return -2;
      }
      for(size_t n=0; n<channel->single_pe.size(); n++){
	//Cuts
	if (channel->single_pe[n].length>max_spe_length) continue;
	// passing all cuts
	histo->Fill(channel->single_pe[n].integral);
      }
    }
  }
  cout<<endl<<"About to fit"<<endl;
  //Draw all the histograms
  root.AcquireLock();
  TCanvas* c = new TCanvas("c",fname);
  c->SetWindowSize(2*c->GetWw(),c->GetWh());
  for(std::map<int,TH1F*>::iterator it = spectra.begin(); it != spectra.end(); it++){
    TH1F* hist = (it->second);
    cout<<endl<<"About to fit channel: "<<it->first<<endl<<endl;
    FitSPE(hist, ChannelsSettings[it->first]);
    cout<<endl<<"Done fitting channel: "<<it->first<<endl<<endl;
  } 
  QueryUser(&spectra, &root, Events, c);
  return 0;
}


int main( int argc, char** argv){
  if (argc != 2){
    cout<<"Usage: specfit <genroot-output-filename>"<<endl;
    return -1;
  } 
  //generate the run filename; make sure root file is there manually!
  std::stringstream rootfile;
  rootfile<<argv[1]; //to enter location explicitly at command line
  return ProcessRun(rootfile.str().c_str());
}
