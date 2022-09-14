#include "utilities.hh"
#include "EventData.hh"
#include "TLine.h"
#include "TFile.h"
#include "TString.h"
#include "TTree.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TROOT.h"
#include "TSeqCollection.h"
#include "TMath.h"
#include "TPad.h"
#include "TBox.h"
#include "TPave.h"
#include "TTimeStamp.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TProfile.h"
#include "TPaveText.h"
#include "TSystem.h"
#include "TClass.h"
#include "TList.h"
#include "TClassMenuItem.h"

  

#include "TGFileDialog.h"

#include "Reader.hh"
#include "EventHandler.hh"
#include "ConvertData.hh"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;

TFile* OpenFile(const char* filename, const char* option)
{
  //see if it's already open
  TSeqCollection* files = gROOT->GetListOfFiles();
  for(int i=0; i< files->GetSize(); i++){
    if( TString(files->At(i)->GetName()) == TString(filename))
      return (TFile*)(files->At(i));
  }
  
  //otherwise try to open it
  TFile* f = new TFile(filename,option);
  if(!f || !f->IsOpen() || f->IsZombie()){
    cerr<<"Unable to open file "<<filename<<endl;
    return 0;
  }
  return f;
}

TTree* GetEventsTree(const char* filename)
{
  TFile* f = OpenFile(filename);
  if(!f)
    return 0;
  TTree* Events = (TTree*)(f->Get("Events"));
  if(!Events){
    cerr<<"Unable to open Events tree from file "<<filename<<endl;
    return 0;
  }
  return Events;
}
  
TGraph* GetAverageGraph(const char* filename, int channel)
{
  TFile* f = OpenFile(filename);
  if(!f)
    return 0;
  TString name = "average_channel";
  name += channel;
  TGraph* g = (TGraph*)(f->Get(name));
  if(!g){
    cerr<<"Unable to open graph "<<name<<" from file "<<filename<<endl;
    return 0;
  }
  return g;
}

TGraphErrors* GetAverageGraph(int run_no, int channel) {
  std::string run;
  std::stringstream out;
  out << run_no;
  run = out.str();
  std::string filename(6-run.size(), '0');
  filename = "/data/s1waveform/Run" + filename + run + ".root";
  //std::cerr << filename << std::endl;
  return (TGraphErrors*) GetAverageGraph(filename.c_str(), channel);
}

int DividePad(TPad* p, int nplots)
{
  int npadsx = 1, npadsy=1;
  if( nplots < 2)
    {}
  else if( nplots == 2)
    npadsx=2;
  else if (nplots < 5){
    npadsx=2; npadsy=2;
  }
  else if(nplots < 7){
    npadsx=3; npadsy=2;
  }
  else if(nplots < 10){
    npadsx=3; npadsy=3;
  }
  else if(nplots < 13){
    npadsx=4, npadsy=3;
  }
  else if(nplots < 17){
    npadsx=4; npadsy=4;
  }
  else{
    npadsx=(int)TMath::Ceil(sqrt(nplots)); 
    npadsy=(int)TMath::Ceil(sqrt(nplots));
  }
  p->Divide(npadsx,npadsy);
  return npadsx*npadsy;
}

int explode_string(const std::string& s, char delim, 
		   std::vector<std::string>& out)
{
  //find all the occurrences of delim
  size_t start = 0;
  while(1){
    size_t end = s.find(delim,start);
    if(end == std::string::npos)
      break;
    out.push_back(s.substr(start, end-start));
    start = end+1;
  }
  out.push_back(s.substr(start,std::string::npos));
  return out.size();
}

std::vector<std::string> explode_cut(const TCut& cut)
{
  std::vector<std::string> out;
  //when you && or add cuts together, each cut is placed into parentheses
  //but have to avoid things like (x-y)/2
  std::string s = cut.GetTitle();
  size_t pos = 0;
  int prev = 0;
  while(pos < s.size() && pos != std::string::npos){
    //look for the combination )&&, which marks an edge
    pos = s.find(")&&",pos);
    if(pos == std::string::npos) pos = s.size()-1;
    //now look backward until you find the matching '('
    int open = 0;
    int pos2 = pos;
    while(--pos2 > prev){
      if(s[pos2] == ')') ++open;
      else if(s[pos2] == '('){
	if(open == 0) break;
	else --open;
      }
    }
    out.push_back(s.substr(pos2+1,pos-pos2-1-open));
    ++pos;
    prev = pos+2;
  }
  return out;
}

void DrawOperationsBoxes(bool drawbubble, bool drawrecirc)
{
  double dummy, y1, y2;
  gPad->Update();
  gPad->GetRangeAxis(dummy,y1,dummy,y2);
  
  if(drawbubble){
      double x1[] = { static_cast<double>(TTimeStamp(2010,04,15,11,06,55,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,05,03,15,51,33,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,05,05,10,54,19,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,05,05,15,57,33,0,0).GetSec())
    };
      double x2[] = { static_cast<double>(TTimeStamp(2010,04,15,16,38,48,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,05,03,20,40,28,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,05,05,13,29,44,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,05,05,18,45,53,0,0).GetSec())
    };
    
    for(size_t i = 0; i < sizeof(x1)/sizeof(double); i++){
      TBox b(x1[i], y1, x2[i], y2);
      b.SetFillStyle(3002);
      b.SetFillColor(kRed);
      b.DrawClone();
    }
  }
  if(drawrecirc){
      double x1[] = { static_cast<double>(TTimeStamp(2010,05,07,15,40,32,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,07,14,17,23,00,0,0).GetSec())
    };
      double x2[] = { static_cast<double>(TTimeStamp(2010,05,07,23,59,59,0,0).GetSec()),
	  static_cast<double>(TTimeStamp(2010,07,15,16,27,21,0,0).GetSec())
    };
    for(size_t i = 0; i < sizeof(x1)/sizeof(double); i++){
      TBox b(x1[i], y1, x2[i], y2);
      b.SetFillStyle(3002);
      b.SetFillColor(kBlue);
      b.DrawClone();
    }
  }
}

double GetBaseline(TGraph* g, int npts, bool subtract)
{
  double* gy = g->GetY();
  double baseline = accumulate(gy,gy+npts,0.)/(1.*npts);
  if(subtract){
    for(int i=0; i < g->GetN(); i++) gy[i] -= baseline;
  }
  return baseline;
}

TGraph* GetRealEvent(const char* filename, int eventnum, int channel)
{
  Reader reader(filename);
  if(!reader.IsOk()){
    std::cerr<<"Unable to open file "<<filename<<std::endl;
    return 0;
  }
  
  EventHandler* handler = EventHandler::GetInstance();
  handler->AddModule<ConvertData>();
  handler->Initialize();
  RawEventPtr event = reader.GetEventWithID(eventnum);
  if(event == RawEventPtr()){
    std::cerr<<"Unable to load event with id "<<eventnum<<std::endl;
    return 0;
  }
  handler->Process(event);
  EventPtr evt = handler->GetCurrentEvent();
  handler->Finalize();
  
  ChannelData* chdata = evt->GetEventData()->GetChannelByID(channel);
  if(!chdata){
    std::cerr<<"Unable to load data for channel "<<channel<<std::endl;
    return 0;
  }
  return chdata->GetTGraph();
}

ChannelData* GetChannelData(const char* filename, int eventnum, int channel)
{
  Reader reader(filename);
  if(!reader.IsOk()){
    std::cerr<<"Unable to open file "<<filename<<std::endl;
    return 0;
  }

  EventHandler* handler = EventHandler::GetInstance();
  handler->AddModule<ConvertData>();
  handler->Initialize();

  ChannelData* chdata = 0;
  
  while(reader.IsOk() && !reader.eof()){
    RawEventPtr event = reader.GetEventWithID(eventnum++);
    if(event == RawEventPtr()){
      std::cerr<<"Unable to load event with id "<<eventnum<<std::endl;
      return 0;
    }
    if(eventnum%5000 == 0)
      Message(INFO)<<"Processing event "<<eventnum<<std::endl;
    
    handler->Process(event);
  }
  EventPtr evt = handler->GetCurrentEvent();
  handler->Finalize();
  
  chdata = evt->GetEventData()->GetChannelByID(channel);
  if(!chdata){
    std::cerr<<"Unable to load data for channel "<<channel<<std::endl;
    return 0;
  }
  
  return chdata;
}

double CorrelationCoefficient(int npts, double* x, double* y)
{
  double sumx=0, sumy=0, sumx2=0, sumy2=0, sumxy=0;
  for(int i=0; i< npts; i++){
    sumx += x[i];
    sumy += y[i];
    sumx2 += x[i]*x[i];
    sumy2 += y[i]*y[i];
    sumxy += x[i]*y[i];
  }
  //formula from wikipedia Correlation and Dependence
  return (npts * sumxy - sumx*sumy) / 
    ( sqrt(npts*sumx2 - sumx*sumx) * sqrt(npts*sumy2 - sumy*sumy) );
}

TCut GetStandardCuts()
{
  TCut energy_min = "event.s1_full>50";
  TCut energy_max = "event.s1_full < 10000";
  TCut saturated = "!event.saturated";
  //TCut peak_time = "GetPulse(0)->peak_time-GetPulse(0)->start_time < 0.1";
  TCut s1length = "GetPulse(0)->end_time-GetPulse(0)->start_time<20";
  TCut baseline = "GetChannelByID(-2)->baseline.found_baseline";
  TCut p0start = "GetPulse(0)->start_time < 0.1 && GetPulse(0)->start_time > -0.1";  
  TCut status = "event.status==0";
  return energy_min + energy_max + saturated + s1length + baseline + p0start+status;
}

TCut GetTwoPulseCuts()
{
  TCut drift="event.drift_time>20";
  TCut s2 = "event.s2_full>10";
  TCut t95="GetPulse(1).t95>10 && GetPulse(1).t95<30";
  TCut valid = "event.s1s2_fixed_valid";
  return GetStandardCuts()+drift+s2+t95+valid;
}

TCut GetOnePulseCuts(bool onlyone)
{
  TCut base = GetStandardCuts()+"event.s1_fixed_valid";
  if(onlyone)
    base += "GetChannelByID(-2)->npulses == 1";
  return base;
}

TCanvas* TwoPulsePlots(TTree* Events, bool queryfit, TCut cuts,TCanvas* c)
{
  TCut all_cuts=cuts+GetTwoPulseCuts();
  c->Clear();
  c->Divide(3,2);
  //set the canvas title
  EventData* evt=0;
  Events->SetBranchAddress("event",&evt);
  Events->GetEntry(0);
  if(evt){
    int first_run = evt->run_id;
    stringstream cantitle;
    cantitle<<"Run "<<first_run;
    Events->GetEntry(Events->GetEntries()-1);
    if(evt->run_id != first_run)
      cantitle<<" - Run "<<evt->run_id;
    c->SetTitle(cantitle.str().c_str());
  }
  bool plotlog = true;
  //uncorrected s2/s1 vs drift time
  c->cd(1);
  if(plotlog){
    Events->Draw("log(event.s2_full/event.s1_full) : drift_time",
		 all_cuts+"event.f90_full < 0.5","colz");
  }
  else{
    Events->Draw("(event.s2_full/event.s1_full) : drift_time",
    		 all_cuts+"event.f90_full < 0.5","colz");
  }
  TH1* htemp = (TH1*)gROOT->FindObject("htemp");
  double tau = 1000000.;
  if(htemp){
    htemp->SetName("hraw");
    htemp->SetTitle("Ln(S2/S1) vs Drift Time");
    htemp->GetXaxis()->SetTitle("Drift time [#mus]");
    htemp->GetYaxis()->SetTitle("Ln(S2/S1)");
    
    TGraph* pf = (TGraph*)gROOT->FindObject("Graph");
    pf->SetName("gscatter");
    //TProfile* pf = ((TH2*)(htemp))->ProfileX("s2s1vtime");
    //pf->Draw("same");
    if(pf){
      const char* fitfunc = (plotlog ? "pol1" : "expo");
      double fitmin=50, fitmax = 120;
      if(queryfit){
	gPad->Update();
	std::cout<<"Enter '<min> <max>' range to fit lifetime:"<<std::endl;
	std::cin>>fitmin>>fitmax;
      }
      pf->Fit(fitfunc,"q","same",fitmin, fitmax);
      TF1* pol1 = pf->GetFunction(fitfunc);
      if(pol1){
	pol1->SetLineColor(kRed);
	pol1->Draw("same");
	tau = -1./pol1->GetParameter(1);
	TPaveText* pt = new TPaveText(0.1,0.1,0.9,0.3,"NDC");
	pt->SetBorderSize(0);
	stringstream text;
	text<<"Lifetime = "<<(int)tau<<"#pm"
	    <<(int)(tau*tau*pol1->GetParError(1))
	    <<" #mus";
	pt->AddText(text.str().c_str());
	text.str("");
	text<<std::setiosflags(std::ios::fixed)<<setprecision(2);
	text<<"Intercept = "<<pol1->GetParameter(0)<<"#pm"
	    <<pol1->GetParError(0);
	pt->AddText(text.str().c_str());
	pt->Draw();
      }
    }
  }
  stringstream alias;
  alias<<"exp(event.drift_time/"<<tau<<")*event.s2_full / event.s1_full";
  Events->SetAlias("ratio_corrected",alias.str().c_str());

  c->cd(2);
  Events->Draw("log(ratio_corrected) : drift_time",
	       all_cuts,"colz");
  htemp = (TH1*)gROOT->FindObject("htemp");
  if(htemp){
    htemp->SetName("hcorrected");
    htemp->SetTitle("Ln(S2/S1) Corrected vs Drift Time");
    htemp->GetXaxis()->SetTitle("Drift time [#mus]");
    htemp->GetYaxis()->SetTitle("Ln(S2/S1) Corrected");
    //gPad->SetLogz();
  }
  c->cd(3);
  Events->Draw("log10(ratio_corrected):event.f90_full>>hscatter(100,0,1,100,-3,3)",
	       all_cuts,"colz");
  htemp = (TH1*)gROOT->FindObject("hscatter");
  if(htemp){
    htemp->SetName("hscatter1");
    htemp->SetTitle("Log(S2/S1) vs F90");
    htemp->GetXaxis()->SetTitle("F90");
    htemp->GetYaxis()->SetTitle("Log10(S2/S1) corrected for drift");
    gPad->SetLogz();
  }
  c->cd(4);
  Events->Draw("event.drift_time ",
	       all_cuts);
  htemp = (TH1*)gROOT->FindObject("htemp");
  if(htemp){
    htemp->SetName("hdt");
    htemp->Rebin(2);
    htemp->SetTitle("Drift Time");
    htemp->GetXaxis()->SetTitle("drift time [#mus]");
  }
  c->cd(5);
  Events->Draw("log10(ratio_corrected)>>hratiolow(100,-3,3)",
	       "event.f90_full>0 && event.f90_full<0.55"+all_cuts);
  htemp = (TH1*)gROOT->FindObject("hratiolow");
  if(htemp){
    htemp->SetName("hratiolow1");
    htemp->SetTitle("Log10(S2/S1) corrected");
    htemp->GetXaxis()->SetTitle("Log10(S2/S1) corrected");
  }
  Events->Draw("log10(ratio_corrected)>>hratiohigh(100,-3,3)",
	       "event.f90_full>0.55 && event.f90_full<1"+all_cuts,"sames");
  htemp = (TH1*)gROOT->FindObject("hratiohigh");
  if(htemp){
    htemp->SetName("hratiohigh1");
    htemp->SetLineColor(kRed);
    htemp->SetTitle("F90>0.55");
    gPad->SetLogy();
  }
  
    
  c->cd(6);
  Events->Draw("event.y : event.x " , 
	       "event.position_valid" + all_cuts);
  htemp = (TH1*)gROOT->FindObject("htemp");
  if(htemp){
    htemp->SetName("hxy");
    htemp->SetTitle("Event Position");
    htemp->GetXaxis()->SetTitle("X [cm]");
    htemp->GetYaxis()->SetTitle("Y [cm]");
  }
  
  
  
  c->cd(0);
  c->Modified();
  c->Update();
  return c;
  
}

TCanvas* OnePulsePlots(TTree* Events, TCut cuts,TCanvas* c)
{
  TCut all_cuts = cuts+GetOnePulseCuts();
  EventData* evt=0;
  Events->SetBranchAddress("event",&evt);
  Events->GetEntry(0);
  if(evt){
    int first_run = evt->run_id;
    stringstream cantitle;
    cantitle<<"Run "<<first_run;
    Events->GetEntry(Events->GetEntries()-1);
    if(evt->run_id != first_run)
      cantitle<<" - Run "<<evt->run_id;
    c->SetTitle(cantitle.str().c_str());
  }
  
  c->Divide(2,2);
  c->cd(1);
  Events->Draw("event.s1_full>>hspec(200,0,4000)", all_cuts);
  TH1* htemp = (TH1*)gROOT->FindObject("hspec");
  if(htemp){
    htemp->SetTitle("Energy Spectrum");
    htemp->GetXaxis()->SetTitle("Energy [npe]");
  }
  c->cd(3);
  Events->Draw("event.f90_full:event.s1_full>>hfe(50,0,4000,50,0,1)",
	       all_cuts, "colz");
  htemp = (TH1*)gROOT->FindObject("hfe");
  if(htemp){
    htemp->SetTitle("f90 vs Energy");
    htemp->GetXaxis()->SetTitle("Energy [npe]");
    htemp->GetYaxis()->SetTitle("f90");
  }
  c->cd(4);
  Events->Draw("event.f90_full>>hf90(100,0,1)",all_cuts);
  htemp = (TH1*)gROOT->FindObject("hf90");
  if(htemp){
    htemp->SetTitle("f90");
    htemp->GetXaxis()->SetTitle("f90");
  }
  return c;
}

TCanvas* PlotSpeDistributions(TTree* Events, bool normalize)
{
  if(!Events) return 0;
  //get the runid from the first event
  EventData* evt = 0;
  Events->SetBranchAddress(EventData::GetBranchName(), &evt);
  Events->GetEntry(0);
  //try to load the calibration

  //info.LoadCalibrationInfo();

  //plot all the channel histograms
  int nchans = (int)Events->GetMaximum("nchans");
  TCanvas* can = new TCanvas;
  DividePad(can,nchans);
  for(int i=0; i<nchans; i++){
    can->cd(i+1);
    std::stringstream cmd;
    cmd<<"channels["<<i<<"].single_pe.integral";
    if(normalize)
      cmd<<"/channels["<<i<<"].spe_mean>>h"<<i<<"(100,0,3)";
    else
      cmd<<">>h"<<i<<"(100,0,"<<3*evt->channels[i].spe_mean<<")";
    Long64_t entries = Events->Draw(cmd.str().c_str(),"");
    //Draw a TLine at the single photoelectron
    gPad->Update();
    double x = (normalize ? 1 : evt->channels[i].spe_mean);
    double y = gPad->GetY2();
    TLine* line = new TLine(x, 0, x, y);
    line->SetLineColor(kBlue);
    line->Draw();

    //set logy
    if(entries)
      gPad->SetLogy();
    
			    
  }
  can->cd(0);
  return can;
}


double ElectronLifetime(TTree* Events, bool newcanvas, bool defaultlimits)
{
  if(newcanvas) new TCanvas;
  Events->Draw("event.s2_full/event.s1_full : event.drift_time >>hprof",
	       GetTwoPulseCuts()+"event.f90_full<0.5","prof");
  TProfile* hprof = (TProfile*)gROOT->FindObject("hprof");
  double min = 50, max = 120;
  gPad->Update();

  if(!defaultlimits){
    std::cout<<"Enter fit limits:"<<std::endl;
    std::cin>>min>>max;
  }
  hprof->Fit("expo","MIQ","",min,max);
  TF1* expo = hprof->GetFunction("expo");
  double tau = -1./expo->GetParameter(1);
  //std::cout<<"Lifetime = "<<tau<<"+/-"<<tau*tau*expo->GetParError(1)<<" ; "
  //   <<"Intercept = "<<expo->GetParameter(0)<<"+/-"<<expo->GetParError(0)
  //   <<std::endl;
  return tau;

  
}

void SaveHistoToFile(TObject* c)
{
  if(!c->InheritsFrom("TH1"))
    return;
  TH1* h = (TH1*)c;
  static const char* filetypes[] = {
    "Text files","*.txt",
    0,0
  };
  TGFileInfo fi;
  fi.fFileTypes = filetypes;
  new TGFileDialog(gClient->GetRoot(),0,kFDSave,&fi);
  if(!fi.fFilename)
    return;
  std::string fname = fi.fFilename;
  if(fname.rfind(".txt")==std::string::npos)
    fname.append(".txt");
  std::cout<<"Saving histogram "<<h->GetName()<<" to file "<<fname<<std::endl;
  std::ofstream fout(fname.c_str());
  if(fout.is_open()){
    fout<<"#X\tY\tYerr\n";
    for(int i=1; i<=h->GetNbinsX(); ++i){
      fout<<h->GetBinLowEdge(i)<<"\t"
	  <<h->GetBinContent(i)<<"\t"
	  <<h->GetBinError(i)
	  <<endl;
    }
  }
}

void CustomizeHistogramMenus()
{
  //have to do each child class separately
  //TH1F
  TClass* cl = TH1F::Class();
  TList* l = cl->GetMenuList();
  l->AddFirst(new TClassMenuItem(TClassMenuItem::kPopupUserFunction,cl,
				 "Save as ASCII","SaveHistoToFile",0,
				 "TObject*",0));
  //TH1D
  cl = TH1D::Class();
  l = cl->GetMenuList();
  l->AddFirst(new TClassMenuItem(TClassMenuItem::kPopupUserFunction,cl,
				 "Save as ASCII","SaveHistoToFile",0,
				 "TObject*",0));
  

}
