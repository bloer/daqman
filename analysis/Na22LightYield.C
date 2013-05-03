#include "TF1.h"
#include "TSpectrum.h"
#include "TH1.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TLegend.h"
#include "TRegexp.h"
#include "TLine.h"
#include "TGraphErrors.h"
#include "analysis/utilities.C"
#include <iostream>
#include <string>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace std;
double epeak = 511.;

double Fit511Photopeak(TH1* h, double* error=0)
{
  TSpectrum spec(1);
  spec.Search(h);
  h->GetXaxis()->SetTitle("Energy [photoelectrons]");
  h->Draw("e");
  TH1* bg = spec.Background(h);
  TH1* sig = (TH1*)(h->Clone());
  sig->SetLineColor(kGreen);
  
  sig->Add(bg,-1);
  sig->Draw("same e");
  sig->Fit("gaus","m","same e");
  TF1* gaus = sig->GetFunction("gaus");
  if(gaus)
    gaus->SetLineColor(kGreen);
  
  bg->SetLineColor(kRed);
  bg->Draw("same e");
  
  TLine* line = new TLine(gaus->GetParameter(1),0,
			  gaus->GetParameter(1),h->GetMaximum());
  line->SetLineColor(kBlue);
  line->SetLineWidth(2);
  line->Draw();
  

  double yield = spec.GetPositionX()[0]/epeak;
  double err = 0;
  
  cout<<"Results from TSpectrum: \n\t"
      <<"Peak = "<<spec.GetPositionX()[0]<<" p.e.; Light Yield = "
      <<yield<<" p.e./keV"<<endl;
  if(gaus){
    yield = gaus->GetParameter(1)/epeak;
    err = gaus->GetParError(1)/epeak;
    cout<<"Results from BG Subtracted Gaus Fit: \n\t"
	<<"Peak = "<<gaus->GetParameter(1)<<" p.e.; Light Yield = "
	<<yield<<" +/- "<<err<<" p.e./keV"<<endl;
    err = max(err, TMath::Abs(yield-spec.GetPositionX()[0]/epeak));
    
  }
  TLegend* leg = new TLegend(.6,.6,.9,.9);
  leg->AddEntry(h,"Raw Spectrum","lpe");
  leg->AddEntry(bg,"Background","lpe");
  leg->AddEntry(sig,"Signal","lpe");
  char title[20];
  sprintf(title,"Yield = %.2f pe/keV",yield);
  leg->AddEntry(line, title ,"l");
  leg->Draw();

  if(error) *error = err;
  return yield;
}

double Na22LightYield(const char* filename, double* error=0)
{
  TTree* Events = GetEventsTree(filename);
  if(!Events)
    return 0;
  gROOT->ProcessLine(".x analysis/Aliases.C");
  //get the run number from the filename
  TString title = TString("Na22 Spectrum, ") + 
    TString(filename)(TRegexp("Run......"));
  TH1F* hist = new TH1F("Na22Spec",title,200,1000,2500);
  Events->Draw("sumspec >> Na22Spec","min > 0","e");
  return Fit511Photopeak(hist, error);

}

TGraphErrors* PlotLightYieldGraph()
{
  string filename;
  vector<double> x,y,ex,ey;
  while(1){
    cout<<"\n\nEnter next file to process; <enter> to finish."<<endl;
    getline(cin, filename);
    if(filename=="")
      break;
    
    //load the tree
    TTree* Events = GetEventsTree(filename.c_str());
    if(!Events)
      continue;
    gROOT->ProcessLine(".x analysis/Aliases.C");
    double start = Events->GetMinimum("timestamp");
    double end = Events->GetMaximum("timestamp");
    double error = 0;
    TString title = TString("Na22 Spectrum, ") + 
      TString(filename)(TRegexp("Run......"));
    TH1F* hist = new TH1F("Na22Spec",title,200,1000,2500);
    Events->Draw("sumspec >> Na22Spec","min > 0","e");
    double yield = Fit511Photopeak(hist,&error);
    
    x.push_back((start+end)/2.);
    ex.push_back((end-start)/2.);
    y.push_back(yield);
    ey.push_back(error);
  }
  if(x.size() == 0){
    cerr<<"No valid points found!"<<endl;
    return 0;
  }
  TGraphErrors* graph = new TGraphErrors(x.size(),&x[0],&y[0],&ex[0],&ey[0]);
  graph->Draw("ape");
  TAxis* xax = graph->GetXaxis();
  xax->SetTitle("Run Time");
  xax->SetTimeDisplay(1);
  xax->SetTimeFormat("%Y/%m/%d");
  xax->SetTimeOffset(1,"gmt");
  TAxis* yax = graph->GetYaxis();
  yax->SetTitle("511 keV Light Yield [pe/keV]");
  graph->Draw("ape");
  return graph;
}
