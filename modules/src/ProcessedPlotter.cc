#include "ProcessedPlotter.hh"
#include "RootGraphix.hh"
#include "PulseFinder.hh"
#include "Differentiator.hh"
#include "SumChannels.hh"
#include "BaselineFinder.hh"
#include "Fitter.hh"
#include "SpeFinder.hh"
#include "ConvertData.hh"
#include "EventHandler.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "V172X_Params.hh"
#include "EvalRois.hh"
#include "Integrator.hh"
#include "S1S2Evaluation.hh"
#include "PadZoomer.hh"

#include <algorithm>
#include <sstream>

#include "TCanvas.h"
#include "TColor.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TH2F.h"
#include "TAxis.h"
#include "TLegend.h"
#include "TList.h"
#include "TMath.h"
#include "TDirectory.h"
#include "TMarker.h"
#include "TEllipse.h"
#include "TPaveLabel.h"

const int colors[] = {kBlack, kRed, kGreen, kCyan, kBlue, kMagenta, kYellow, 
		      kGray+2, kOrange-3, kGreen+3, kCyan+3, kMagenta-5, 
		      kRed-2};
const int ncolors = sizeof(colors)/sizeof(int); 


ProcessedPlotter::ProcessedPlotter() : 
  BaseModule(GetDefaultName(), 
	     "Plot each channel's waveform and selected analysis results"),
  _graphix(0), _paused(false)
{
  
  AddDependency<ConvertData>();
  AddDependency<RootGraphix>();
  //Register all the config handler parameters
  //RegisterParameter("multi_canvas",multi_canvas = true);
  RegisterParameter("chans_per_pad", chans_per_pad = 8,
		    "Maximum number of channels to stack on a single plot");
  RegisterParameter("overlay_analysis", overlay_analysis = true,
		    "Overlay the raw data with analysis module output (if only 1 channel in the pad)?");
  RegisterParameter("multi_color", multi_color = true,
		    "Draw with multiple colors?");
  RegisterParameter("draw_legend", draw_legend = true,
		    "Include a legend with the plot?");
  RegisterParameter("draw_title", draw_title = false,
		    "Draw a separate title on the pulses plot?");
  RegisterParameter("autoscalex", autoscalex = true,
		    "Set the scale of the x axis automatically?");
  RegisterParameter("autoscaley",autoscaley = true,
		    "Set the scale of the y axis automatically?");
  RegisterParameter("xmin", xmin = -10, "Minimum range of the x axis");
  RegisterParameter("xmax", xmax = 10, "Maximum range of the x axis");
  RegisterParameter("ymin", ymin = 0, "Minimum range of the y axis");
  RegisterParameter("ymax", ymax = 16383, "Maximum range of the y axis");
  RegisterParameter("subtract_baseline", subtract_baseline = false,
		    "Subtract the baseline before plotting?");
  RegisterParameter("downsample",downsample=1,
		    "Factor by which to downsample graphed waveforms");
  
  RegisterParameter("drawpulses",drawpulses = true,
		    "Enable drawing of raw pulses?");
  RegisterParameter("drawpmtweights",drawpmtweights = false,
		    "Enable drawing of PMT weights graph?");
  RegisterParameter("scale_pmts_sum",scale_pmts_sum = false,
		    "Scale PMT weights to sum (true) or max (false)");
  
  
  for(int i=0; i<NCANVASES; ++i)
    _canvas[i] = 0;
}

ProcessedPlotter::~ProcessedPlotter()
{
  Finalize();
}

int ProcessedPlotter::Initialize()
{
  EventHandler* evhandler = EventHandler::GetInstance();
  _graphix = evhandler->GetModule<RootGraphix>();
  if(!_graphix){
    //shouldn't happen...
    Message(ERROR)<<"ProcessedPlotter called Initialize, but no RootGraphix!\n";
    return 1;
  }
  
  //only draw pmt weights if there is pmt info from the database

    drawpmtweights = false;
  //also disable pmtweights if not S1S2Evaluation
  S1S2Evaluation* eval = evhandler->GetModule<S1S2Evaluation>();
  if(!eval || !eval->enabled)
    drawpmtweights = false;
  
  if(!drawpulses && !drawpmtweights){
    Message(ERROR)<<"Processed plotter is enabled, but all draw functions "
		  <<"are disabled. Aborting\n";
    return 2;
  }
  
  if(drawpulses){
    _canvas[PULSES] = _graphix->GetCanvas();
    if(draw_title){
      //add a pad to hold the title
      RootGraphix::Lock glock = _graphix->AcquireLock();
      _canvas[PULSES]->Divide(1,2);
      _canvas[PULSES]->cd(1);
      double y=0.95;
      gPad->SetPad(0,y,1,1);
      TPaveLabel* subtitle = new TPaveLabel(0.08,0,0.92,1,"","NDC");
      subtitle->SetBorderSize(0);
      subtitle->SetBit(TObject::kCanDelete,true);
      subtitle->SetTextColor(kYellow);
      subtitle->SetFillColor(kBlue);
      subtitle->SetFillStyle(1001);
      subtitle->SetTextFont(62);
      subtitle->SetTextSize(1.1);
      subtitle->Draw();
      _canvas[PULSES]->cd(2);
      gPad->SetPad(0,0,1,y);
    }
  }
    
  if(drawpmtweights)
    _canvas[PMTWEIGHTS] = _graphix->GetCanvas();
  
  if(downsample < 1) downsample = 1;
  return 0;
}

int ProcessedPlotter::Finalize()
{
  for(int i=0; i<NCANVASES; ++i)
    _canvas[i] = 0;
  return 0;
}

int ProcessedPlotter::Process(EventPtr event)
{
  if(_paused) return 0;
  EventDataPtr data = event->GetEventData();
  Message(DEBUG3)<<"ProcessedPlotter: Acquiring graphics lock.\n";
  RootGraphix::Lock glock = _graphix->AcquireLock();
  Message(DEBUG3)<<"ProcessedPlotter: Successfully acquired graphics lock.\n";
  char title[30];
  sprintf(title, "Run %d - Event %d", data->run_id, data->event_id);
  if(drawpulses && _canvas[PULSES]){
    _canvas[PULSES]->SetTitle(title);
    TPad* pulsepad = _canvas[PULSES];
    if(draw_title){
      //draw the run+event label
      _canvas[PULSES]->cd(1);
      if(gPad->GetListOfPrimitives() && 
	 gPad->GetListOfPrimitives()->GetEntries() == 1){
	TPaveLabel* label = (TPaveLabel*)(gPad->GetListOfPrimitives()->At(0));
	if(label){
	  label->SetLabel(title);
	  gPad->Modified();
	}
      }
      pulsepad = (TPad*)(_canvas[PULSES]->cd(2));
      if(!pulsepad){
	Message(ERROR)<<"ProcessedPlotter: Unable to find pulse pad!\n";
	return 1;
      }
    }
    pulsepad->Clear();
    //Organize pads by number of channels
    // get only the channels not excluded
    int nchans = 0;
    std::vector<ChannelData*> chans_to_draw;
    for( size_t ch = 0; ch < data->channels.size(); ch++){
      if( _skip_channels.find(data->channels[ch].channel_id) == 
	  _skip_channels.end() ){
	nchans++;
	chans_to_draw.push_back(&(data->channels[ch]));
      }
      
    }
    
    int cpp = chans_per_pad;
    if(cpp < 1)
      cpp = (nchans > 0 ? nchans : 1);
    int total_pads = (nchans+cpp-1)/cpp;
    
    if(total_pads == 0)
      return 0;
    else if(total_pads == 1) {}
    else if(total_pads == 2)
      pulsepad->Divide(2,1);
    else if(total_pads < 5)
      pulsepad->Divide(2,2);
    else if(total_pads < 7)
      pulsepad->Divide(3,2);
    else if(total_pads < 10)
      pulsepad->Divide(3,3);
    else if(total_pads < 13)
      pulsepad->Divide(4,3);
    else if(total_pads < 17)
      pulsepad->Divide(4,4);
    else if(total_pads < 21)
      pulsepad->Divide(5,4);
    else if(total_pads < 26)
      pulsepad->Divide(5,5);
    else if(total_pads < 31)
      pulsepad->Divide(6,5);
    else if(total_pads < 37)
      pulsepad->Divide(6,6);
    else if(total_pads < 43)
      pulsepad->Divide(7,6);
    else{
      int rootpads = (int)(ceil(sqrt(total_pads)));
      pulsepad->Divide(rootpads,rootpads);
    }
    for(int pad=0; pad<total_pads; pad++){
      pulsepad->cd( (total_pads == 1 ? 0 : pad+1 ) );
      int chans_this_pad = std::min(cpp, nchans-pad*cpp);
	
      if( overlay_analysis && chans_this_pad == 1 ){
	chans_to_draw[pad]->Draw(subtract_baseline, downsample,
				 autoscalex, autoscaley, 
				 xmin, xmax, ymin, ymax);
      }
      else{
	TMultiGraph* graphs = new TMultiGraph;
	graphs->SetBit(TObject::kCanDelete, true);
	TLegend* legend = 0;
	if(draw_legend && chans_this_pad > 1){
	  legend = new TLegend(0,.9,1,1);
	  legend->SetBit(TObject::kCanDelete, true);
	  legend->SetNColumns(4);
	  legend->SetColumnSeparation(-.3);
	  legend->SetMargin(0.25);
	}
	
	for(int i = pad*cpp; 
	    i < (pad+1)*cpp && i < nchans; i++){
	  TGraph* g = chans_to_draw[i]->GetTGraph(subtract_baseline,downsample);
	  if(!g) continue;
	  if(multi_color && chans_this_pad>1){
	    g->SetLineColor(colors[abs(chans_to_draw[i]->channel_id)%ncolors]);
	    //int id = data->channels[i].channel_id;
	    //g->SetLineColor(TColor::GetColor(85 * ((id&1)+(id&8)/8), 
	    //85 * ((id&2)/2 + (id&16)/16), 
	    //				   85 * ((id&4)/4 + (id&32)/32) ));
	    g->SetMarkerColor(g->GetLineColor());
	    g->SetFillColor(g->GetLineColor());
	  }
	  if(legend)
	    legend->AddEntry(g, g->GetTitle(), "lpf");
	  graphs->Add(g);
	  if(chans_this_pad == 1)
	    graphs->SetTitle(g->GetTitle());
	  
	}
	
	graphs->Draw("alp");
	if(!autoscalex)
	  graphs->GetXaxis()->SetRangeUser(xmin, xmax);
	if(!autoscaley)
	  graphs->GetYaxis()->SetRangeUser(ymin, ymax);
	
	/*TAxis* yax = graphs->GetYaxis();
	  if(yax)
	  yax->SetLabelOffset(0);
	  TAxis* xax = graphs->GetXaxis();
	  if(xax)
	  xax->SetTitle("sample time [#mu s]");
	*/
	if(legend)
	  legend->Draw();
	
      }
    }
    // update the last pad
    if(!autoscalex || !autoscaley )
      gPad->Modified();
    _canvas[PULSES]->cd(0);
    _canvas[PULSES]->SetSelected(0);
    //_canvas[PULSES]->Update();
    new PadZoomer(pulsepad);
  }
  

  return 0;
}


const TCanvas* ProcessedPlotter::GetCanvas(int i) const
{ 
  if (i == 0 || i == 1) 
    return _canvas[i]; 
  else
    {
      Message(ERROR)<<"ProcessedPlotter::GetCanvas()"<<std::endl
		    <<"Attempt to access non-existent canvas"
		    <<std::endl;
      return NULL;
    }
}
