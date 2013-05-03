#include "PulseShapeEval.hh"
#include "intarray.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "PulseFinder.hh"
#include "RootWriter.hh"
#include <algorithm>
#include <cmath>
#include "TFile.h"
#include "TH1F.h"

PulseShapeEval::PulseShapeEval() : 
  BaseModule(GetDefaultName(), 
		"Calculate pulse shape parameters for event discrimination")
{
  AddDependency<BaselineFinder>();
  AddDependency<PulseFinder>();

  RegisterParameter("pulse_shape_file", pulse_shape_file="auxiliary_files/shapes.root",
		   "File containing reference pulse shapes");
  RegisterParameter("gatti_weights_hist", 
		    gatti_weights_hist="gatti_weights",
		    "Name of Gatti wieghts histogram");
  RegisterParameter("ll_ele_weights_hist", 
		    ll_ele_weights_hist="ll_ele_weights",
		    "Name of electron log-likelihood weights histogram");
  RegisterParameter("ll_nuc_weights_hist", 
		    ll_nuc_weights_hist="ll_nuc_weights",
		    "Name of nuclear recoil log-likelihood weights histogram");
  RegisterParameter("ll_r_weights_hist", 
		    ll_r_weights_hist="ll_r_weights",
		    "Name of log-likelihood ratio weights histogram");
}

PulseShapeEval::~PulseShapeEval()
{
  Finalize();
}

int PulseShapeEval::Initialize() 
{ 
    int status =  PulseShapeEval::LoadWeights(); 
    
    return status;
}

int PulseShapeEval::Finalize() 
{ 
    std::map<int, TH1F*>::iterator mapit_gatti = gatti_weights.begin();
    std::map<int, TH1F*>::iterator mapit_ll_ele = ll_ele_weights.begin();
    std::map<int, TH1F*>::iterator mapit_ll_nuc = ll_nuc_weights.begin();
    std::map<int, TH1F*>::iterator mapit_ll_r = ll_r_weights.begin();

    for(; mapit_gatti != gatti_weights.end(); mapit_gatti++)
	delete mapit_gatti->second;
    for(; mapit_ll_ele != ll_ele_weights.end(); mapit_ll_ele++)
	delete mapit_ll_ele->second;
    for(; mapit_ll_nuc != ll_nuc_weights.end(); mapit_ll_nuc++)
	delete mapit_ll_nuc->second;
    for(; mapit_ll_r != ll_r_weights.end(); mapit_ll_r++)
	delete mapit_ll_r->second;
    
    gatti_weights.clear();
    ll_ele_weights.clear();
    ll_nuc_weights.clear();
    ll_r_weights.clear();

    return 0; 
}

int PulseShapeEval::LoadWeights()
{
    //Basic checks
    TFile* f = new TFile(pulse_shape_file.c_str());
    if (f->IsZombie())
    {
	Message(ERROR)<<"Unable to open pulse shape file: "
		      <<pulse_shape_file<<std::endl;
	return 1;
    }

    //Loop over and find weights for all real channels
    for (int i = 0; i < 50; i++)
    {
	std::ostringstream gatti_name, ll_ele_name, ll_nuc_name, ll_r_name;
	gatti_name<<gatti_weights_hist<<"_"<<i;
	ll_ele_name<<ll_ele_weights_hist<<"_"<<i;
	ll_nuc_name<<ll_nuc_weights_hist<<"_"<<i;
	ll_r_name<<ll_r_weights_hist<<"_"<<i;

	if (! f->Get(gatti_name.str().c_str()) || 
	    ! f->Get(ll_ele_name.str().c_str()) || 
	    ! f->Get(ll_nuc_name.str().c_str()) || 
	    ! f->Get(ll_r_name.str().c_str()))
	{
	    break;
	}

	//ASSUMPTIONS
	// All weight histograms have the same binning
	
	//Load histograms of weights
	TH1F* gatti_weights_temp = (TH1F*) f->Get(gatti_name.str().c_str())->Clone();
	TH1F* ll_ele_weights_temp = (TH1F*) f->Get(ll_ele_name.str().c_str())->Clone();
	TH1F* ll_nuc_weights_temp = (TH1F*) f->Get(ll_nuc_name.str().c_str())->Clone();
	TH1F* ll_r_weights_temp = (TH1F*) f->Get(ll_r_name.str().c_str())->Clone();
	
	//Disassociate histograms from ROOT file - stupid ROOT !
	gatti_weights_temp->SetDirectory(0);
	ll_ele_weights_temp->SetDirectory(0);
	ll_nuc_weights_temp->SetDirectory(0);
	ll_r_weights_temp->SetDirectory(0);

	gatti_weights.insert(std::make_pair(i, gatti_weights_temp));
	ll_ele_weights.insert(std::make_pair(i, ll_ele_weights_temp));
	ll_nuc_weights.insert(std::make_pair(i, ll_nuc_weights_temp));
	ll_r_weights.insert(std::make_pair(i, ll_r_weights_temp));
    }

    //Load weights for SUM channel
    std::ostringstream gatti_name, ll_ele_name, ll_nuc_name, ll_r_name;
    gatti_name<<gatti_weights_hist<<"_-2";
    ll_ele_name<<ll_ele_weights_hist<<"_-2";
    ll_nuc_name<<ll_nuc_weights_hist<<"_-2";
    ll_r_name<<ll_r_weights_hist<<"_-2";

    if (f->Get(gatti_name.str().c_str()) && 
	f->Get(ll_ele_name.str().c_str()) && 
	f->Get(ll_nuc_name.str().c_str()) && 
	f->Get(ll_r_name.str().c_str()))
    {
	//Load histograms of weights
	TH1F* gatti_weights_temp = (TH1F*) f->Get(gatti_name.str().c_str())->Clone();
	TH1F* ll_ele_weights_temp = (TH1F*) f->Get(ll_ele_name.str().c_str())->Clone();
	TH1F* ll_nuc_weights_temp = (TH1F*) f->Get(ll_nuc_name.str().c_str())->Clone();
	TH1F* ll_r_weights_temp = (TH1F*) f->Get(ll_r_name.str().c_str())->Clone();
	
	//Disassociate histograms from ROOT file - stupid ROOT !
	gatti_weights_temp->SetDirectory(0);
	ll_ele_weights_temp->SetDirectory(0);
	ll_nuc_weights_temp->SetDirectory(0);
	ll_r_weights_temp->SetDirectory(0);

	gatti_weights.insert(std::make_pair(-2, gatti_weights_temp));
	ll_ele_weights.insert(std::make_pair(-2, ll_ele_weights_temp));
	ll_nuc_weights.insert(std::make_pair(-2, ll_nuc_weights_temp));
	ll_r_weights.insert(std::make_pair(-2, ll_r_weights_temp));


	//Create histogram for binning data
	event_shape = (TH1F*)(gatti_weights_temp->Clone());
	event_shape->SetDirectory(0);
	event_shape->Reset();

    }    

    f->Close();

    return 0;
}

int PulseShapeEval::Process(EventPtr evt)
{
    EventDataPtr data = evt->GetEventData();
    ChannelData* sumch = data->GetChannelByID(ChannelData::CH_SUM);
    
    double total_first_pulse_integral = 0;
    data->gatti = 0;
    data->ll_r = 0;
    
    if (gatti_weights.size() < data->channels.size())
    {
	Message(ERROR)<<"PulseShapeEval.cc: Size of weights file less than number of channels"
		      <<std::endl;
	return -1;
    }

    for (size_t ch = 0; ch < data->channels.size(); ch++)
    {
	ChannelData& chdata = data->channels[ch];
	//skip channels we've been told to explicitly skip
	if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
	    continue;

	if (gatti_weights.find(chdata.channel_id) == gatti_weights.end())
	{
	    Message(ERROR)<<"PulseShapeEval.cc: Weights file for channel "
			  <<chdata.channel_id<<" not loaded"
			  <<std::endl;
	    return -1; 
	}

	if(!(chdata.baseline.found_baseline) || chdata.baseline.saturated)
	    continue;
	
	const double* wave = chdata.GetBaselineSubtractedWaveform();
	int n_bins = gatti_weights[chdata.channel_id]->GetNbinsX();

	for (size_t pulse_num = 0; pulse_num < chdata.pulses.size(); pulse_num++)
	{
	    chdata.pulses[pulse_num].gatti = 0;
	    chdata.pulses[pulse_num].ll_ele = 0;
	    chdata.pulses[pulse_num].ll_nuc = 0;
	    chdata.pulses[pulse_num].ll_r = 0;
	    
	    Pulse pulse;
	    	    
	    if (data->pulses_aligned == true)
	    {
		if (! sumch)
		{
		    Message(ERROR)<<"PulseShapeEval.cc: Request to align pulses across channels, but sum channels disabled"<<std::endl;
		    return -1;
		}
		//Align everything by the corresponding pulse on the sum channel
		pulse = sumch->pulses[pulse_num];
	    }
	    else
	    {
		pulse = chdata.pulses[pulse_num];
	    }

	    int pulse_index = pulse.start_index;
	    event_shape->Reset();
	    
	    for(int i = 1; i <= n_bins; i++)
	    {
		//Get event pulse shape
		double t0 = event_shape->GetBinLowEdge(i);
		double t1 = event_shape->GetBinLowEdge(i+1);
		double z = 0;
		
		if (chdata.TimeToSample(pulse.peak_time + t1) < 0 ||
		    chdata.TimeToSample(pulse.peak_time + t0) >= chdata.nsamps)
		{
		    event_shape->SetBinContent(i, 0);
		}
		else
		{
		    while (chdata.SampleToTime(pulse_index) < pulse.peak_time + t0)
			pulse_index++;
		    
		    /*
		      if (pulse_num == 0)
		      Message(INFO)<<"Bin: "<<i<<" "
		      <<chdata.SampleToTime(pulse_index)<<" "
		      <<pulse.peak_time + t0<<" "
		      <<pulse.peak_time + t1<<" "
		      <<std::endl;
		    */    
		    while (chdata.SampleToTime(pulse_index) > pulse.peak_time + t0 &&
			   chdata.SampleToTime(pulse_index) < pulse.peak_time + t1 &&
			   pulse_index < chdata.nsamps)
		    {
			z = z - wave[pulse_index];
			pulse_index++;
		    }
		    event_shape->SetBinContent(i, z);
		}
		
	    //Calculate pulse shape parameters
		chdata.pulses[pulse_num].gatti += gatti_weights[chdata.channel_id]->GetBinContent(i) * event_shape->GetBinContent(i);
		chdata.pulses[pulse_num].ll_ele += ll_ele_weights[chdata.channel_id]->GetBinContent(i) * event_shape->GetBinContent(i);
		chdata.pulses[pulse_num].ll_nuc += ll_nuc_weights[chdata.channel_id]->GetBinContent(i) * event_shape->GetBinContent(i);
		chdata.pulses[pulse_num].ll_r += ll_r_weights[chdata.channel_id]->GetBinContent(i) * event_shape->GetBinContent(i);
	    }// end loop over time bins
	
	    //Scale by integral of pulse
	    double pulse_integral = event_shape->Integral();
	    chdata.pulses[pulse_num].pulse_shape_int = pulse_integral;
	    if (pulse_integral != 0)
	    {
		chdata.pulses[pulse_num].gatti = chdata.pulses[pulse_num].gatti / pulse_integral;
		chdata.pulses[pulse_num].ll_ele = chdata.pulses[pulse_num].ll_ele / pulse_integral;
		chdata.pulses[pulse_num].ll_nuc = chdata.pulses[pulse_num].ll_nuc / pulse_integral;
		chdata.pulses[pulse_num].ll_r = chdata.pulses[pulse_num].ll_r / pulse_integral;
	    }
	    
	    if (data->pulses_aligned == true || data->channels.size() == 1)
	    {
		if (pulse_num == 0 && chdata.channel_id >= 0)
		{
		    total_first_pulse_integral += pulse_integral;
		    data->gatti += chdata.pulses[pulse_num].gatti * pulse_integral;
		    data->ll_r += chdata.pulses[pulse_num].ll_r * pulse_integral;
		}
	    }	
	    
	} // end loop over pulses
    } // end loop over channels

    //Normalized pulse shape variables
    if (total_first_pulse_integral != 0)
    {
	data->gatti = data->gatti / total_first_pulse_integral;
	data->ll_r = data->ll_r / total_first_pulse_integral;
    }
    return 0;
}
