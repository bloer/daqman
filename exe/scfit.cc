/* 
Used laserrun.cc for structure
*/


//include all header files as in laserrun except ones know won't need
#include "FitOverROI.hh"
#include "FitTH1F.hh"
#include "TNamed.h"
#include "TString.h"
#include "TCanvas.h"
#include "EventData.hh"
#include "TRint.h"
#include "utilities.hh"
#include "RootGraphix.hh"
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
//header files from FitOverROI
#include "TMath.h"
#include "TROOT.h"
#include "TFitResult.h"
#include "TSpectrum.h"
#include "TPad.h"
#include "TLine.h"
#include "TAxis.h"
#include "TList.h"
#include "TCut.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include <algorithm>
#include <numeric>
#include <string>
#include "TMultiGraph.h"
#include "TAttMarker.h"
#include "TStyle.h"
#include "TAttLine.h"


using namespace std;

class tab{
public:
  int n;
  char f;
  tab(int nspaces, char fill=' ') : n(nspaces),f(fill) {}
};

//i suppose these should be in a header file.
double base_start_time = -1; //baseline start time
double base_end_time = -0.2;  //baseline end time
double sample_rate = 250.; //250 samples per us
double fit_min = 1.0; //start of fit window
double fit_max = 6.0; //end of fit window
bool background = true; //use constant background in fit?

#define mLifetime 1.5;
#define mShot_noise 0.5;
#define mAmplitude 10000; //10 basic avg, 10000 sum, .001 energy
#define mConst 0;

ostream& operator<<(ostream& out, tab t)
{ return out<<left<<setfill(t.f)<<setw(t.n); }


Double_t ExpFunc(double* x, double* params) {
  double y = x[0];
  double lifetime = params[0];
  //double shot_noise = params[1];
  double scale = params[2];
  double constant = params[3];
  double funct = TMath::Power(TMath::E(),-y/lifetime);
  /*
  double funct = (1/lifetime)*
    TMath::Power(TMath::E(),shot_noise*shot_noise/
		 (2*lifetime*lifetime) - y/lifetime) * 
    1/2*(1+TMath::Erf((y - shot_noise*shot_noise/lifetime) /
		      (sqrt(2)*shot_noise)));
  */
  return scale*funct+constant;
}


//draw all splits for single channel or vice versa
void DrawMany(map<int, TGraphErrors*>* avgwaveforms, RootGraphix* root, TCanvas* c) {
  c->Clear();
  int padn=1;
  DividePad(c, avgwaveforms->size());
  int color = 1;
  for( std::map<int,TGraphErrors*>::iterator it = avgwaveforms->begin();
       it != avgwaveforms->end(); it++) {
    c->cd(padn++);
    RootGraphix::Lock lock = root->AcquireLock();
    gPad->SetLogy();
    TGraphErrors* graph = (it->second);
    TAxis* xaxis = graph->GetXaxis();
    xaxis->SetRangeUser(-2, 10);    //plot only (-2,10) us
    TAxis* yaxis = graph->GetYaxis();
    yaxis->SetRangeUser(1., yaxis->GetXmax());
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);
    graph->Draw("ALP");
    //color += 1;
  }
  c->cd(0);
  c->Update();

}

void DrawSingle(TGraphErrors* graph, RootGraphix* root, TCanvas* c) {
  c->Clear();
  RootGraphix::Lock lock = root->AcquireLock();
  gPad->SetLogy();
  TAxis* xaxis = graph->GetXaxis();
  xaxis->SetRangeUser(-2, 10);
  xaxis->SetTitle("time [#mus]");
  graph->Draw("ALP");
  c->Update();
}

void PlotResiduals(TGraphErrors* graph, RootGraphix* root, TCanvas* c) {
  c->Clear();
  RootGraphix::Lock lock = root->AcquireLock();
  TF1* func = graph->GetFunction("exp_func_f");
  double* x = graph->GetX();
  double* y = graph->GetY();
  int nsamps = graph->GetN();
  double resid_x[nsamps];
  double resid_y[nsamps];
  //calculate residuals for all values from fit_min to end
  for (int i=0; i<nsamps; i++) {
    resid_x[i] = x[i];
    if (x[i]<fit_min) {
      resid_y[i] = 0;
    }
    else {
      resid_y[i] = y[i]-func->Eval(x[i]);
    }
  }
  TGraph* residuals = new TGraph(nsamps, resid_x, resid_y);
  residuals->SetTitle(graph->GetTitle());
  TAxis* xaxis = residuals->GetXaxis();
  xaxis->SetRangeUser(func->GetXmin(), func->GetXmax()); //plot from fit_min to fit_max
  xaxis->SetTitle("time [#mus]");
  TAxis* yaxis = residuals->GetYaxis();
  yaxis->SetTitle("residuals [a.u.]");
  residuals->SetMarkerStyle(kFullDotMedium);
  residuals->Draw("AP");
  c->SetGridy(1);
  c->Update();
}

int TimeToSample(double time, double sample_rate, int trigger_index, int split) {
  int samp = (int)(time*sample_rate+trigger_index);
  samp = (int)TMath::Floor((double) samp / (double) split );
  return samp;
}

//calculate straight baseline mean and variance
void Baseline(double* baseline_mean, double* baseline_var, TGraphErrors* graph, int split) {
  double* y = graph->GetY();
  double* x = graph->GetX();
  double baseline = 0;
  double sumsq = 0;
  const int trigger_index = (int)TMath::Floor(sample_rate*TMath::Abs(x[0])); //trigger at 0
  const int base_start_index = TimeToSample(base_start_time, sample_rate, trigger_index, split);
  const int base_end_index = TimeToSample(base_end_time, sample_rate, trigger_index, split);
  for (int i = base_start_index; i<base_end_index; i++) {
    baseline += y[i]/((double) base_end_index - (double) base_start_index);
    sumsq += y[i]*y[i]/((double) base_end_index - (double) base_start_index);
  }
  *baseline_var = sumsq - baseline*baseline;
  *baseline_mean = baseline;
  
}


//loop over all entries of a map<int, map<int, TGraphErrors*> >
void Loop(void (*func)(TGraphErrors* wave, int), 
          map<int, map<int, TGraphErrors*> >* split_waves, int split) {
  for (map<int, map<int, TGraphErrors*> >::iterator it = split_waves->begin();
       it != split_waves->end(); it++) {
    for (map<int, TGraphErrors*>::iterator it2 = it->second.begin();
         it2 != it->second.end(); it2++) {
      TGraphErrors* wave = it2->second;
      (*func)(wave, split);
    }
  }
}

//add baseline errors in quadrature with waveform errors
void AddBaselineErrors(TGraphErrors* wave, int nSplit) {
  double* ey = wave->GetEY();
  double baseline_mean = 0;
  double baseline_var = 0;
  Baseline(&baseline_mean, &baseline_var, wave, nSplit);
  for (int i=0; i<wave->GetN(); i++) {
    ey[i] = sqrt(baseline_var+ey[i]*ey[i]);
  }
}

void loopAddBaselineErrors(map<int, map<int, TGraphErrors*> >* split_waves_by_ch, int nSplit) {
  Loop(AddBaselineErrors, split_waves_by_ch, nSplit);
}

//subtract baseline from waveform
void Subtract(TGraphErrors* wave, int nSplit) {
  double baseline_mean = 0;
  double baseline_var = 0;
  Baseline(&baseline_mean, &baseline_var, wave, nSplit);
  double* y = wave->GetY();
  for (int i=0; i<wave->GetN(); i++) {
    y[i] = y[i] - baseline_mean;
  }
}

void loopSubtract(map<int, map<int, TGraphErrors*> >* split_waves_by_split, int nSplit) {
  Loop(Subtract, split_waves_by_split, nSplit);
}

//fit using MINUIT
void Fit(TGraphErrors* wave, int nSplit) {
  
  int nparams = 4;
  double params[nparams];
  params[0] = mLifetime;  //lifetime
  params[1] = mShot_noise; //shot_noise
  params[2] = mAmplitude //scale factor
  params[3] = mConst; //constant background
  
  TF1* exp_func_f = new TF1("exp_func_f",ExpFunc,fit_min, fit_max, nparams);
  
  exp_func_f->SetLineColor(kGreen);
  exp_func_f->SetParameters(params);
  exp_func_f->SetLineWidth(3);
  exp_func_f->SetParNames("lifetime","shot_noise","scale","constant");
  exp_func_f->FixParameter(1, 0);
  if (!background)
    exp_func_f->FixParameter(3, 0);
  TFitResultPtr fit_result = wave->Fit(exp_func_f,"MRE+");
  
  /*
  //plot additional exponentials with tau +/- 0.1 us 
  TList* funclist = wave->GetListOfFunctions();
  
  TF1* exp_func1 = new TF1("exp_func1", ExpFunc, fit_min,fit_max,nparams);
  (wave->GetFunction("exp_func_f"))->Copy(*exp_func1);
  exp_func1->SetLineColor(kBlue);
  exp_func1->SetParameters(exp_func_f->GetParameters());
  exp_func1->SetParameter(0, exp_func_f->GetParameter(0)+0.1);
  exp_func1->SetRange(0.2, wave->GetXaxis()->GetXmax());
  funclist->AddLast(exp_func1);
  
  TF1* exp_func2 = new TF1("exp_func2",ExpFunc,fit_min,fit_max, nparams);
  (wave->GetFunction("exp_func_f"))->Copy(*exp_func2);
  exp_func2->SetLineColor(kBlue);
  exp_func2->SetParameters(exp_func_f->GetParameters());
  exp_func2->SetParameter(0, exp_func_f->GetParameter(0)-0.1);
  exp_func2->SetRange(0.2, wave->GetXaxis()->GetXmax());
  funclist->AddLast(exp_func2);
  */
}

void loopFit(map<int, map<int, TGraphErrors*> >* split_waves, int nSplit) {
  Loop(Fit, split_waves, nSplit);
}

//split average waveform into pieces
void Split(map<int, TGraphErrors*>* avgwaveforms, map<int, map<int,TGraphErrors*> >* split_waves, int split) {
  //split_waves: pointer to map of [ch] to [map of (split) to (TGraph)]
  
  for (std::map<int,TGraphErrors*>::iterator it = avgwaveforms->begin();
       it != avgwaveforms->end(); it++) {
    TGraphErrors* avggraph = (it->second);
    int splitN = (int)TMath::Floor((double) avggraph->GetN()/(double) split);
    
    double* x = avggraph->GetX();
    double* y = avggraph->GetY();
    double* ey = avggraph->GetEY();
    //populate split_waves with "split" number of waves
    int ch = (it->first);
    std::map<int,TGraphErrors*> waves;
    int n = (it->first);
    char name[25];
    for (int i=0; i<split; i++) {
      TGraphErrors* graph = new TGraphErrors(splitN);
      waves.insert(std::make_pair(i, graph));
      sprintf(name,"ch%d split%d", n, i);
      waves[i]->SetName(name);
      waves[i]->SetTitle(name);
    }
    
    //populate each graph with a wave
    for (int j=0; j<avggraph->GetN(); j++) {
      if (j % split == 0 && j<(avggraph->GetN()-split+1)) {
        int jj = (int)TMath::Floor((double) j / (double) split);
        for (int i=0; i<split; i++) {
          waves[i]->SetPoint(jj, x[j+i], y[j+i]);
          waves[i]->SetPointError(jj, 0, ey[j+i]);
        }
      }
    }
    
    split_waves->insert(std::make_pair(ch, waves));
  }
}

void Reorganize(map<int, map<int, TGraphErrors*> >* split_waves_by_ch, 
                map<int, map<int, TGraphErrors*> >* split_waves_by_split) {
  for (map<int, map<int, TGraphErrors*> >::iterator it = split_waves_by_ch->begin();
       it != split_waves_by_ch->end(); it++) {
    map<int, TGraphErrors*> ch = it->second;
    int ch_i = it->first;
    for (map<int, TGraphErrors*>::iterator it2 = ch.begin(); it2 != ch.end(); it2++) {
      TGraphErrors* wave = it2->second;
      int split_i = it2->first;
      if (!(split_waves_by_split->count(split_i)>0)) {
        map<int, TGraphErrors*> wave_i;
        split_waves_by_split->insert(make_pair(split_i, wave_i));
      }
      (split_waves_by_split->find(split_i))->second.insert(make_pair(ch_i, wave));
    }
  }
}


//print all fitted parameters
void PrintResults(map<int, map<int, TGraphErrors*> >* split_waves_by_ch, int nSplit, int run) {
  int width = 80;
  cout<<"\n"<<tab(width,'*')<<""<<"\n"<<"Results for run"<<run<<" \n"<<tab(width,'*')<<""<<endl;
  
  for (map<int, map<int, TGraphErrors*> >::iterator it = split_waves_by_ch->begin();
       it != split_waves_by_ch->end(); it++) {
    cout << "\nChannel " << it->first << "\n" << tab(width, '-') << "" << endl;
    cout<<tab(10)<<"split"<<tab(20)<<"lifetime"<<tab(20)<<"amplitude"<<tab(20)<<"background"<<tab(20)<<"chi2/ndf"<<endl;
    cout << tab(width,'-') << "" << endl;
    map<int, TGraphErrors*> waves = it->second;
    for (map<int, TGraphErrors*>::iterator it2 = waves.begin();
         it2 != waves.end(); it2++) {
      TGraphErrors* graph = it2->second;
      TF1* func = graph->GetFunction("exp_func_f");
      char lifetime[50];
      char amplitude[50];
      char offset[50];
      char chisq[50];
      sprintf(lifetime, "%#.5g +- %.4f", func->GetParameter(0), func->GetParError(0));
      sprintf(amplitude, "%#.5g +- %.4f", func->GetParameter(2), func->GetParError(2));
      sprintf(offset, "%#.5g +- %.4f", func->GetParameter(3), func->GetParError(3));
      sprintf(chisq, "%#.5g/%d", func->GetChisquare(), func->GetNDF());
      cout<<tab(10)<<it2->first<<tab(20)<<lifetime<<tab(20)<<amplitude<<tab(20)<<offset<<tab(20)<<chisq<<endl;
    }
    
  }
}

int ProcessRun(const char* fname) {
  
  
  
  //open the root file
  RootGraphix root;
  root.Initialize();
  TFile *f = OpenFile(fname, "UPDATE");
  if (!f) {cout<<"File not opened!"<<endl;}
  TTree* Events = GetEventsTree(fname);
  if (!Events) {cout<<"!Events"<<endl; return -1;}
  
  EventData* event = 0;
  Events->SetBranchAddress("event",&event);
  Events->GetEntry(0);
  cout<<"There are "<<Events->GetEntries()<<" entries in this tree."<<endl;

  int nChans = event->channels.size();
  int run = event->run_id;
  printf("run %d\n", run);
  
  //put all the TGraphErrors into a map
  std::map<int, TGraphErrors*> avgwaveforms;
  for (int i=0; i<nChans; i++) {
    int chan = event->channels.at(i).channel_num;
    if (chan < 0) { //there are average waveforms for positive channels only
      continue;
    }
    if (chan == 6 || chan == 7) { //don't include channels 6 (crappy) or 7 (different tube)
      continue; 
    }
    TGraphErrors* avgch = (TGraphErrors*) GetAverageGraph(fname, chan);
    avgwaveforms.insert( std::make_pair(chan, avgch));
  }
  
  //split avgwaveforms--------
  int nSplit = 8;
  //split_waves_by_ch: map of [ch] to [map of (split) to (TGraph*)]
  std::map<int, map<int, TGraphErrors*> > split_waves_by_ch;
  Split(&avgwaveforms, &split_waves_by_ch, nSplit);
  
  //reorganize split_waveforms for convenience
  //split_waves_by_split: map of [split] to [map of (ch) to (TGraph*)]
  map<int, map<int, TGraphErrors*> > split_waves_by_split;
  Reorganize(&split_waves_by_ch, &split_waves_by_split);
  
  
  //do analysis----------------
  
  //add baseline errors in quadrature; do separately for each split
  loopAddBaselineErrors(&split_waves_by_split, nSplit);
  
  //subtract any baseline offset
  loopSubtract(&split_waves_by_split, nSplit);

  //do the fits
  loopFit(&split_waves_by_split, nSplit);
  
  
  //print results--------------
  PrintResults(&split_waves_by_ch, nSplit, run);
  
  
  
  //draw stuff-----------------
  
  //draw all splits for single channel or vice versa
  TCanvas* c = new TCanvas("c","fname");
  int chsplit = 4; //plot everything for this channel (or split)
  //for single split:
  DrawMany(&split_waves_by_split[chsplit], &root, c); //or split_waves_by_ch[chsplit] for single channel
  gStyle->SetOptFit();
   
  //draw single ch, single split
  TCanvas* c3 = new TCanvas("c3", "ch_single");
  int plot_ch = 4;
  int plot_split = 5;
  TGraphErrors* ch_single = split_waves_by_ch[plot_ch][plot_split];
  DrawSingle(ch_single, &root, c3);
  
  //plot residuals in fitted region for single ch,split
  TCanvas* c1 = new TCanvas("c1", "residuals");
  PlotResiduals(ch_single, &root, c1);
  
  
  char pause2;
  cout << endl << "Enter anything to quit." << endl;
  cin>>pause2;
  
  
  
  return 0;
}


int main( int argc, char** argv) 
{
  
  //generate the run filename; make sure root file is there manually!
  std::stringstream rootfile, rawfile;
  /*
  int run = atoi(argv[1]);
  //rootfile << "./Run";
  rootfile << "/data/test_processing/afan/Run";
  rawfile  << "/data/rawdata/Run";
  rootfile<<std::setw(6)<<std::setfill('0')<<run<<".root";
  rawfile<<std::setw(6)<<std::setfill('0')<<run<<".out.gz";
  printf("%d \n", run);
  */
  
  /*
  runid = atoi(argv[1]);
  rootfile << "/data/test_processing/afan/lifetimes/Run";
  rootfile <<std::setw(6)<<std::setfill('0')<<runid<<".root";
  printf("%d \n", runid);
  */
  
  rootfile<<argv[1]; //to enter location explicitly at command line


  return ProcessRun(rootfile.str().c_str());
}


