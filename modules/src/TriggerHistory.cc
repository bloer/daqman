#include "TriggerHistory.hh"
#include "RootGraphix.hh"
#include "EventHandler.hh"

#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TH1.h"
#include "TAxis.h"
#include "TList.h"
#include "TROOT.h"

TriggerHistory::TriggerHistory(const std::string& name) : 
  BaseModule(name), _graphix(0), 
  _canvas(0), _triggergraph(0), _eventgraph(0), _multigraph(0), _legend(0)
{
  AddDependency("RootGraphix");
  AddDependency("ConvertData");
  
  RegisterParameter("update_interval", _update_interval=60,
		    "Minimum seconds between each point on history plot");
  RegisterParameter("max_points", _max_points = 100,
		    "Maximum points in graph before scrolling");
}

TriggerHistory::~TriggerHistory()
{}

int TriggerHistory::Initialize()
{
  if(_update_interval<0) _update_interval = 0;
  _last_update_time = 0;
  _last_triggerid = 0;
  _last_eventid = 0;

  _graphix = EventHandler::GetInstance()->GetModule<RootGraphix>();
  if(!_graphix){
    Message(ERROR)<<"TriggerHistory: Can't find RootGraphix during initialization!"<<std::endl;
    return 1;
  }
  _canvas = _graphix->GetCanvas();
  _canvas->SetTitle("Trigger and Event Rates");
  _triggergraph = new TGraphErrors();
  _triggergraph->SetName("triggergraph");
  _triggergraph->SetTitle("Trigger Rate History");
  _triggergraph->SetLineWidth(2);
  _eventgraph = new TGraphErrors();
  _eventgraph->SetName("eventgraph");
  _eventgraph->SetTitle("Recorded Event Rate History");
  _eventgraph->SetLineColor(kBlue);
  _eventgraph->SetMarkerColor(kBlue);
  _eventgraph->SetLineWidth(2);
  _multigraph = new TMultiGraph();
  _multigraph->SetTitle("Trigger Rate History");
  _multigraph->Add(_triggergraph);
  _multigraph->Add(_eventgraph);
  _legend = new TLegend(0.05,0,0.95,0.05);
  _legend->SetNColumns(2);
  _legend->SetBorderSize(1);
  return 0;
}

int TriggerHistory::Finalize()
{
  //RootGraphix will delete the canvas...
  _canvas = 0;
  //multigraph owns trigger and event graps...
  delete _multigraph;
  delete _legend;
  
  _multigraph=0;
  _triggergraph=0;
  _eventgraph=0;
  _legend=0;
  
  return 0;
}

int TriggerHistory::Process(EventPtr evt)
{
  RawEventPtr raw = evt->GetRawEvent();
  if(_last_update_time == 0){
    _last_update_time = raw->GetTimestamp();
    _last_triggerid = evt->GetEventData()->trigger_count;
    _last_eventid = raw->GetID();
  }
  else if( raw->GetTimestamp() - _last_update_time >= 
	   (uint32_t)_update_interval){
    double ntrigs = evt->GetEventData()->trigger_count - _last_triggerid;
    double nevts = raw->GetID() - _last_eventid;
    double tavg = 0.5*(raw->GetTimestamp() + _last_update_time);
    double terr = tavg - _last_update_time;

    RootGraphix::Lock glock = _graphix->AcquireLock();
    _canvas->cd();
    _canvas->Clear();
    int point = _triggergraph->GetN();
    if(_max_points > 0 && point >= _max_points){
      point--;
      _triggergraph->RemovePoint(0);
      _eventgraph->RemovePoint(0);
    }
    
    _triggergraph->SetPoint(point, tavg, ntrigs/(2*terr));
    _triggergraph->SetPointError(point,terr, sqrt(ntrigs)/(2*terr));
    _eventgraph->SetPoint(point, tavg, nevts/(2*terr));
    _eventgraph->SetPointError(point, terr, sqrt(nevts)/(2*terr));
    //trick to make multigraph recalculate range
    TH1* hist = _multigraph->GetHistogram();
    if(hist)
      hist->SetMinimum(hist->GetMaximum());
    
    _multigraph->Draw("ae");
    
    
    _last_update_time = raw->GetTimestamp();
    _last_triggerid = evt->GetEventData()->trigger_count;
    _last_eventid = raw->GetID();
    
    _legend->Clear();
    _legend->AddEntry(_triggergraph,
		      Form("Triggers (%u total)",_last_triggerid),"l");
    _legend->AddEntry(_eventgraph,
		      Form("Events Recorded (%u total)",_last_eventid),"l");
  
    _legend->Draw();
    
    //on older root, have to update before setting time stuff
    if(gROOT->GetVersionInt()<53200)
      _canvas->Update();
    _multigraph->GetXaxis()->SetTimeDisplay(1);
    _multigraph->GetXaxis()->SetTimeFormat("%H:%M");
    _multigraph->GetYaxis()->SetTitle("Rate [Hz]");
    _canvas->Modified();
    
        
    
  }
  return 0;
}
