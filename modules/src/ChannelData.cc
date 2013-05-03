#include "ChannelData.hh"
#include "Message.hh"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TAxis.h"
#include "TLine.h"
#include "TBox.h"
#include "TPad.h"
#include "TList.h"
#include "TGaxis.h"
#include "TMarker.h"
#include <algorithm>
#include <cmath>

#include "intarray.hh"
#include "EvalTGraphPoint.C"

class Scale{
  double _offset;
  double _stretch;
public:
  Scale(double offset, double stretch) : _offset(offset), _stretch(stretch) {}
  double operator()(double x){ return _stretch*x + _offset; }
};

TGraph* ChannelData::GetTGraph(bool baseline_subtracted, int downsample) const
{
  if( nsamps < 2 || waveform.empty())
    return 0;
  if( baseline_subtracted && subtracted_waveform.empty())
    return 0;
  if(downsample < 1) downsample = 1;
  std::vector<double> x(nsamps/downsample);
  std::vector<double> ycpy( downsample>1 ? nsamps/downsample : 0);
  //double y[(const int)nsamps];
  const double* y = (baseline_subtracted ? GetBaselineSubtractedWaveform() :
		     GetWaveform() );
  for(int i=0; i<nsamps/downsample; i++){
    x[i] = (i*downsample - trigger_index) / sample_rate;
    if(downsample > 1)
      ycpy[i] = y[i*downsample];
  }
  TGraph* graph = new TGraph(nsamps/downsample,&x[0],
			     (downsample > 1 ? &(ycpy[0]) : y) );
  char name[30];
  if(label != "")
    sprintf(name,"%i-%s",channel_id, label.c_str());
  else 
    sprintf(name,"%i",channel_id);
  graph->SetName(name);
  graph->SetTitle(name);
  graph->SetEditable(false);
  //graph->GetXaxis()->SetTitle("sample time [#mu s]");
  //graph->SetMarkerStyle(7);
  return graph;
}

void ChannelData::Draw(bool baseline_subtracted, int downsample,
		       bool autoscalex, bool autoscaley, 
		       double xmin, double xmax, double ymin, double ymax)
{
  if(downsample < 1) downsample = 1;
  TGraph* chgraph = GetTGraph(baseline_subtracted,downsample);
  if(!chgraph)
    return;
  int ns = chgraph->GetN();
  TMultiGraph* graphs = new TMultiGraph;
  graphs->SetBit(TObject::kCanDelete, true);
  Double_t* x = chgraph->GetX();
  graphs->Add(chgraph);
  graphs->SetName(chgraph->GetName());
  graphs->SetTitle(chgraph->GetTitle());
    
  //see if the derivative is there
  if((int)derivative.size() == nsamps){
    std::vector<double> scaled_derivative(ns);
    for(int i=0; i<ns; i++){
      scaled_derivative[i] = derivative[i*downsample]*3;
    }
   
    TGraph* dergraph = new TGraph(ns, x, &scaled_derivative[0]);
    
    //dergraph->SetMarkerStyle(7);
    dergraph->SetLineColor(kRed);
    dergraph->SetMarkerColor(kRed);
    graphs->Add(dergraph);
  }
  
    
  if((int)smoothed_data.size() == nsamps){
    
    TGraph* smoothgraph = 0;
    if(downsample > 1 ){
      std::vector<double> smoothcp(ns);
      for(int i=0; i<ns; i++)
	smoothcp[i] = smoothed_data[i*downsample];
      smoothgraph = new TGraph(ns,x,&(smoothcp[0]));
    }
    else
      smoothgraph = new TGraph(ns, x, &(smoothed_data[0]));
    smoothgraph->SetLineColor(kTeal);
    smoothgraph->SetMarkerColor(kTeal);
    graphs->Add(smoothgraph);
  }
  
  //add other graphs here
  //draw the baseline if present
  if (baseline.found_baseline &&!baseline_subtracted){                          
    std::vector<double> base(ns);                                  
    for (int i=0; i<ns; i++){                       
      base[i] = waveform[i*downsample] - subtracted_waveform[i*downsample]; 
    }                                                                        
    TGraph* basegraph = new TGraph(ns, x, &(base[0]));                 
    basegraph->SetLineColor(kRed);                                    
    basegraph->SetMarkerColor(kRed);                                    
    graphs->Add(basegraph);   
  } 
  
  
  graphs->Draw("alp");
  if( !autoscalex )
    graphs->GetXaxis()->SetRangeUser(xmin, xmax);
  if( !autoscaley )
    graphs->GetYaxis()->SetRangeUser(ymin, ymax);
  if(!autoscalex || !autoscaley)
    graphs->Draw("alp");
  
  //now look for the integral
  bool draw_integral = false;
  int integral_color = kBlue;
  double integral_scale=1, integral_offset=0;
  TGraph* integral_graph = 0;
  if((int)integral.size() == nsamps){
    
    draw_integral = true;
    //draw the integral along the baseline
    std::vector<double> shifted_integral(ns);
    double x1,x2,y1,y2;
    gPad->Update();
    gPad->GetRangeAxis(x1,y1,x2,y2);
    integral_offset = (baseline_subtracted ? 0 : baseline.mean);
    double raw_ratio = (y2 - integral_offset) / (integral_offset - y1);
    double integral_ratio = std::abs(integral_max) / std::abs(integral_min);
    if (raw_ratio < integral_ratio) {
      integral_scale = (y2 - integral_offset) / std::abs(integral_max) * 0.9;
    }else{
      integral_scale = (integral_offset - y1) / std::abs(integral_min) * 0.9;
    }
    for(int i=0;i<ns;i++){
      shifted_integral[i] = integral_scale*integral[i*downsample]+
	integral_offset;
    }
    integral_graph = new TGraph(ns, x, &shifted_integral[0]);
    integral_graph->SetLineColor(integral_color);
    integral_graph->SetMarkerColor(integral_color);
    graphs->Add(integral_graph);
  }
  
  //Draw the pulse start, end, and amplitude if there
  for(size_t i=0; i<pulses.size(); i++){
    Pulse& pulse = pulses[i];
    double base = (baseline_subtracted ? 0 : baseline.mean);
    if(pulse.found_start){
      double peaky = 0;
      if(pulse.found_peak)
	peaky = base - pulse.peak_amplitude;
      TBox* pbox = new TBox( x[pulse.start_index], base,
			     x[pulse.end_index], peaky);
      pbox->SetBit(TObject::kCanDelete,true);
      pbox->SetLineColor(kGreen);
      pbox->Draw();
      
      TLine* pline = new TLine( x[pulse.peak_index], base,
				x[pulse.peak_index], peaky);
      pline->SetBit(TObject::kCanDelete,true);
      pline->SetLineColor(kMagenta);
      pline->Draw();
    }
    if(pulse.fit.fit_done){
      bool time_based_fit = false;
      if(time_based_fit){
	TF1* fitfunc = pulse.fit.GetTF1();
	if (pulse.fit.fit_result == 0)
	  fitfunc->SetLineColor(kGreen);
	else
	  fitfunc->SetLineColor(kRed);
	fitfunc->SetBit(TObject::kCanDelete,true);
	fitfunc->Draw("same");
      }
      else{
	TF1* fitfunc = pulse.fit.GetTF1();
	const int nfitpts = pulse.fit.end_index - pulse.fit.start_index;
	if(nfitpts > 2){
	  double fitx[nfitpts];
	  double fity[nfitpts];
	  for(int j=0; j<nfitpts; j++){
	    fitx[j] = x[pulse.fit.start_index+j];
	    fity[j] = fitfunc->Eval(pulse.fit.start_index + j);
	  }
	  TGraph* fitgraph = new TGraph(nfitpts, fitx, fity);
	  fitgraph->SetBit(TObject::kCanDelete,true);
	  if (pulse.fit.fit_result == 0)
	    fitgraph->SetLineColor(kGreen);
	  else
	    fitgraph->SetLineColor(kRed);
	  fitgraph->Draw("same");
	}
	delete fitfunc;
      }
    }
  }//end loop over pulses
    //Draw the region start, end, and amplitude if there
  for(size_t i=0; i<regions.size(); i++){
    Roi& region = regions[i];
    double base = (baseline_subtracted ? 0 : baseline.mean);
    
      double peaky = 0;
     
	peaky = base + region.min;
      TBox* pbox = new TBox( x[region.start_index], base,
			     x[region.end_index], peaky);
      pbox->SetBit(TObject::kCanDelete,true);
      pbox->SetLineColor(kGreen);
      pbox->Draw();
      if(draw_integral){
      TLine* iline = new TLine( x[region.min_index], base,//////iline
				x[region.min_index], base+region.integral*integral_scale);
      iline->SetBit(TObject::kCanDelete,true);
      iline->SetLineColor(kBlue);
      iline->SetLineWidth(4);
      iline->Draw();
      }
      TLine* pline = new TLine( x[region.min_index], base,
				x[region.min_index], peaky);
      pline->SetBit(TObject::kCanDelete,true);
      pline->SetLineColor(kMagenta);
      pline->Draw();
      
    

  }//end loop over regions
  //Draw boxes around the single photoelectrons
  for(size_t i = 0; i<single_pe.size(); i++){
    const Spe& spe = single_pe[i];
    TBox* spebox = 
      new TBox(spe.start_time, baseline.mean,
	       spe.start_time + spe.length, 
	       baseline.mean - spe.amplitude);
    spebox->SetBit(TObject::kCanDelete, true);
    spebox->SetLineColor(kRed);
    spebox->Draw();
  }
  
 //Draw boxes for each ROI
  /*
    TAxis* yax = graphs->GetYaxis();
    for(size_t i=0; i<regions.size(); i++){
    const Roi& roi = regions[i];
    TBox* roibox = new TBox(roi.start_time, yax->GetBinLowEdge(1), 
    roi.end_time, yax->GetBinUpEdge(yax->GetNbins()));
    roibox->SetBit(TObject::kCanDelete, true);
    //roibox->SetFillColor(kGreen);
    //roibox->SetFillStyle(3003);
    roibox->SetLineColor(kGreen);
    roibox->Draw("l");
    }
  */
  
  //mark the integral max and min;
  if(draw_integral){
    /*TMarker* max_integral = 
      new TMarker(integral_max_time, integral_graph->Eval(integral_max_time),
      3);*/
    TMarker* max_integral = 
      new TMarker(integral_max_time, 
		  EvalTGraphPoint(*integral_graph,integral_max_time),3);
    max_integral->SetMarkerColor(integral_color);
    max_integral->SetBit(TObject::kCanDelete, true);
    max_integral->Draw();
    /*TMarker* min_integral = 
      new TMarker(integral_min_time, integral_graph->Eval(integral_min_time),
      3);*/
    TMarker* min_integral = 
      new TMarker(integral_min_time, 
		  EvalTGraphPoint(*integral_graph,integral_min_time),3);
    min_integral->SetMarkerColor(integral_color);
    min_integral->SetBit(TObject::kCanDelete, true);
    min_integral->Draw();
    //draw a separate axis if the integral was drawn
    if(gPad){
      //gPad->Update();
      double x1,x2,y1,y2;
      gPad->GetRangeAxis(x1,y1,x2,y2);
      gPad->SetBit(TObject::kCanDelete, true);
      TGaxis* gaxis = new TGaxis(x2,y1,x2,y2,
				 (y1-integral_offset)/integral_scale,
				 (y2-integral_offset)/integral_scale,
				 510,"L+");
      gaxis->SetName("integral_axis");
      if(channel_id == CH_SUM)
	gaxis->SetTitle("Integral [photoelectrons]"); 
      else
	gaxis->SetTitle("Integral [counts*samples]");
      gaxis->SetLineColor(integral_color);
      gaxis->SetTextColor(integral_color);
      gaxis->SetTitleColor(integral_color);
      gaxis->SetLabelColor(integral_color);
      gaxis->SetTitleOffset(1.2);
      gaxis->Draw();
    }
  
  }
  //label the xaxis
  graphs->GetXaxis()->SetTitle("sample time [#mus]");
  if(channel_id == CH_SUM)
    graphs->GetYaxis()->SetTitle("Amplitude [arb]");
  else
    graphs->GetYaxis()->SetTitle("Amplitude [counts]");
  
  //gPad->Update();
  
}

void ChannelData::Print(int verbosity)
{
    {
	Message m(INFO);
	m<<"CHANNEL "<<channel_id<<std::endl;
	m<<"Timestamp: "<<timestamp<<std::endl; 
	m<<"Trigger Index: "<<trigger_index<<std::endl;
	m<<"Smoothed Min: "<<smoothed_min<<std::endl; 
	m<<"Smoothed Max: "<<smoothed_max<<std::endl;
	m<<"Saturated: "<<saturated<<std::endl;
	m<<"Maximum: "<<maximum<<std::endl;
	m<<"Minimum: "<<minimum<<std::endl;
	m<<"Max Time: "<<max_time<<std::endl;
	m<<"Min Time: "<<min_time<<std::endl;
	m<<"SPE Mean: "<<spe_mean<<std::endl;
	m<<"SPE Sigma: "<<spe_sigma<<std::endl;
	m<<"Baseline Found: "<<baseline.found_baseline<<std::endl;
	m<<"Baseline Mean: "<<baseline.mean<<std::endl;
	m<<"Baseline Variance: "<<baseline.variance<<std::endl;
	m<<"Baseline Search Start Index: "<<baseline.search_start_index<<std::endl;
	m<<"Baseline Length: "<<baseline.length<<std::endl;
	m<<"Baseline Saturated: "<<baseline.saturated<<std::endl;
	m<<"Baseline Laser Skip: "<<baseline.laserskip<<std::endl;
	m<<"No. of Pulses: "<<npulses<<std::endl;
	m<<"Integral Max: "<<integral_max<<std::endl;
	m<<"Integral Min: "<<integral_min<<std::endl;
	m<<"Integral Max Time: "<<integral_max_time<<std::endl;
	m<<"Integral Min Time: "<<integral_min_time<<std::endl;
	m<<"Integral Max Index: "<<integral_max_index<<std::endl;
	m<<"Integral Min Index: "<<integral_min_index<<std::endl;
	m<<"Integral Max Time: "<<integral_max_time<<std::endl;
	m<<"S1 Full: "<<s1_full<<std::endl;
	m<<"S1 Fixed: "<<s1_fixed<<std::endl;
	m<<"S2 Full: "<<s2_full<<std::endl;
	m<<"S2 Fixed: "<<s2_fixed<<std::endl;
    }
    if (verbosity > 1)
    {
	for (int i = 0; i < npulses; i++)
	{
	    pulses[i].Print(channel_id, i);
	}
    }
}
    	
