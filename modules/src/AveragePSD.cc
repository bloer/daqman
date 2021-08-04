#include "AveragePSD.hh"
#include "TH1D.h"
#include "TString.h"
#include "TCanvas.h"
#include "THStack.h"
#include "TFile.h"
#include "TVirtualFFT.h"

#include "EventHandler.hh"
#include "RootGraphix.hh"
#include "EventData.hh"
#include "ChannelData.hh"
#include "ConvertData.hh"

const int colors[] = {kBlack, kRed, kGreen, kCyan, kBlue, kMagenta, kYellow, 
		      kGray+2, kOrange-3, kGreen+3, kCyan+3, kMagenta-5, 
		      kRed-2};
const int ncolors = sizeof(colors)/sizeof(int); 


AveragePSD::AveragePSD() : 
  BaseModule(GetDefaultName(), "Generate averaged PSDs of waveforms"),
  _plotstack(nullptr), _graphix(nullptr), _canvas(nullptr)
{
  AddDependency<ConvertData>();
  RegisterParameter("min_time", _min_time=-1000,
		    "Time in us relative to trigger to being PSD calculation");
  RegisterParameter("max_time", _max_time = 1000,
		    "Time in us reltaive to trigger to stop PSD calculation");
  RegisterParameter("logy", _logy=true, "Plot in logy?");
  RegisterParameter("logx", _logx=true, "Plot in logx?");
}

AveragePSD::~AveragePSD()
{
  Finalize();
}

int AveragePSD::Initialize()
{
  _graphix = EventHandler::GetInstance()->GetModule<RootGraphix>();
  if(_graphix && _graphix->enabled){
    _canvas = _graphix->GetCanvas(GetName().c_str());
    _canvas->SetLogy(_logy);
    _canvas->SetLogx(_logx);
    _plotstack = new THStack("avgpsds", "Averaged Channel PSDs");
  } 
  return 0;
}

int AveragePSD::Finalize()
{
  for(auto it : _plots){
    if(gFile && gFile->IsOpen()){
      it.second->Write();
    }
    delete it.second;
  }
  _plots.clear();
  _canvas = 0;
  //this causes a segfault?
  //delete _plotstack;
  _plotstack = 0;
  return 0;
}


int AveragePSD::Process(EventPtr evt)
{
  EventDataPtr data = evt->GetEventData();
  for(ChannelData& chdata : data->channels){
    if(chdata.channel_id != ChannelData::CH_SUM)
      Process(&chdata);
  }
  if(_canvas && _plotstack){
    _plotstack->Modified();
    _canvas->Modified();
  }
  return 0;
}

TH1D* AveragePSD::GetPlot(ChannelData* chdata)
{
  auto it = _plots.find(chdata->channel_id);
  if(it != _plots.end())
    return it->second;
  return AddPlot(chdata);
}

TH1D* AveragePSD::AddPlot(ChannelData* chdata)
{
  int samp0 = chdata->TimeToSample(_min_time, true);
  int samp1 = chdata->TimeToSample(_max_time, true);
  int nsamps = samp1 - samp0 + 1;
  double dt = 1./chdata->sample_rate; //us
  double df = 1./(nsamps * dt); //MHz
  
  TH1D* hist = new TH1D(Form("psd%d", chdata->channel_id), 
                        Form("PSD, channel %s", chdata->label.c_str()),
                        nsamps/2, 0, nsamps/2. * df);
  hist->SetXTitle("Frequency / MHz");
  hist->SetYTitle("PSD [V^{2}/MHz]");
  hist->SetLineColor(colors[chdata->channel_id % ncolors]);
  _plots[chdata->channel_id] = hist;  
  
  if(_canvas && _plotstack){
    RootGraphix::Lock glock = _graphix->AcquireLock();
    _canvas->cd();
    _plotstack->Add(hist);
    _plotstack->Draw("nostack");
  }
  return hist;
}

int AveragePSD::Process(ChannelData* chdata)
{
  int samp0 = chdata->TimeToSample(_min_time, true);
  int samp1 = chdata->TimeToSample(_max_time, true);
  int nsamps = samp1 - samp0 + 1;
  double dt = 1./chdata->sample_rate; //us
  double df = 1./(nsamps * dt); //MHz
  TH1D* hist = GetPlot(chdata);
  TVirtualFFT* fftgen = TVirtualFFT::FFT(1, &nsamps, "R2C M");
  if(!fftgen){
    Message(ERROR)<<"Can't load ROOT FFT module\n";
    return 1;
  }
  fftgen->SetPoints(chdata->GetWaveform() + samp0);
  fftgen->Transform();
  double re, im;
  //start at bin1 to suppress the large DC component
  for(int i=1; i<hist->GetNbinsX(); ++i){
    fftgen->GetPointComplex(i, re, im);
    double newval = (re*re + im*im) / nsamps / df;
    hist->SetBinContent(i+1, hist->GetBinContent(i+1)*hist->GetEntries() 
                             + newval);
  }
  hist->SetEntries(hist->GetEntries()+1);
  hist->Scale(1./hist->GetEntries());
  return 0;
}
