#include "AverageWaveforms.hh"
#include "ConvertData.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "intarray.hh"
#include "TGraphErrors.h"
#include "TFile.h"
#include "PulseFinder.hh"
#include "RootWriter.hh"
#include "EventData.hh"

#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

AverageWaveforms::AverageWaveforms() : 
    BaseModule(GetDefaultName(), "Average the waveform for each channel over the entire run and save to the output root file")
{
    AddDependency<ConvertData>();
    AddDependency<SumChannels>();
    AddDependency<BaselineFinder>();
    AddDependency<PulseFinder>();
    //AddDependency<RootWriter>();

    //Register all the config handler parameters
    RegisterParameter("use_event_list", use_event_list = false, 
		      "True if no cuts should be performed and the average should be constructed from a set of events specified in a text file.");
    RegisterParameter("event_list_location", event_list_location = "auxiliary_files/average_event_list.txt", 
		      "Location of text file to be used if use_event_list is true. Each line of the file must have the run number and then the event number (separated by a space) sorted in ascending order.");

    //Register all the cut parameters. These are ignored if use_event_list is set to true.
    RegisterParameter("with_s2", with_s2 = false, "True if run contains s2");
    RegisterParameter("min_pulse_height", min_pulse_height = 100,
		      "Minimum pulse height for one waveform to be counted in average waveform");
    RegisterParameter("max_pulse_height", max_pulse_height = 3000,
		      "Maximum pulse height for one waveform to be counted in average waveform");
    RegisterParameter("min_fprompt", min_fprompt = 0,
		      "Minimum fprompt for one waveform to be counted in average waveform");
    RegisterParameter("max_fprompt", max_fprompt = 1,
		      "Maximum fprompt for one waveform to be counted in average waveform");
    RegisterParameter("align_by_peak", align_by_peak = true,
		      "Align waveforms by the peak of the first pulse on the sum channel. Otherwise align by the trigger.");
    RegisterParameter("sum_start_time", sum_start_time = -20,
		      "");
    RegisterParameter("sum_end_time", sum_end_time = 400,
		      "");
    RegisterParameter("min_s1_start_time", min_s1_start_time = -0.08,
		      "");
    RegisterParameter("max_s1_start_time", max_s1_start_time = 0.05,
		      "");
    RegisterParameter("bin_size", bin_size = 50, "Use multiple of baseline group #");
    RegisterParameter("min_s2_start_time", min_s2_start_time = 40, "");
    RegisterParameter("max_s2_start_time", max_s2_start_time = 300, "");
    RegisterParameter("number_of_base_groups", number_of_base_groups = 2, "");
}

AverageWaveforms::~AverageWaveforms()
{
    Cleanup();
}

void AverageWaveforms::Cleanup()
{
    std::map<int,TGraphErrors*>::iterator mapit = _plots.begin();
    for( ; mapit != _plots.end() ; mapit++){
	delete mapit->second;
    }
    _plots.clear();
    _num_event.clear();
}


int AverageWaveforms::Initialize()
{ 
    // opens file stream, takes first event
    if (use_event_list)
    {
	string line;
	current_event = -1;
	current_run = -1;
	txt.open(event_list_location.c_str());
	if (txt.is_open() && txt.good())
	{
	    getline(txt,line);
	    istringstream ss1(line);
	    string temp;
	    getline(ss1, temp, ' ');
	    istringstream ss2(temp);
	    ss2 >> current_run;
	    getline(ss1, temp, ' ');
	    istringstream ss3(temp);
	    ss3 >> current_event;
	}
    }
    return 0; 
}

int AverageWaveforms::Finalize()
{
    if(gFile && gFile->IsOpen()){
	std::map<int,TGraphErrors*>::iterator mapit = _plots.begin();
	for( ; mapit != _plots.end(); mapit++){
	    TGraphErrors* graph = mapit->second;
	    //double* y = graph->GetY();
	    double* ey = graph->GetEY();

	    for(int i=0; i < graph->GetN(); i++){
		// ey stored sum of variances
		ey[i] = sqrt(ey[i]);
	    }
	    graph->SetFillStyle(3002);
	    graph->SetFillColor(kRed);
	    graph->Write();
	}
    }
    std::map<int, int>::iterator numOfEvents = _num_event.begin();
    for ( ; numOfEvents != _num_event.end(); numOfEvents++){
	Message(INFO)<<"<Module> AverageWaveforms: Channel "<< numOfEvents->first
		     <<" includes "<< numOfEvents->second <<" events"<<std::endl;
    }
    if (txt.is_open())
	txt.close();
    Cleanup();
    return 0;
}

int AverageWaveforms::Process(EventPtr evt)
{
    EventDataPtr event = evt->GetEventData();
    ChannelData* sum_ch = event->GetChannelByID(ChannelData::CH_SUM);

    //Event level cuts
    if (align_by_peak && !sum_ch)
	return 0;

    if (!use_event_list)
    {   
	if (!sum_ch)
	    return 0;

	if(! with_s2)
	{
	    // look only at events with 1 scintillation pulse
	    if (sum_ch->pulses.size() != 1) 
		return 0;
	}
	else
	{ //with s2
	  // look only at events with 2 scintillation pulse
	    if (sum_ch->pulses.size()!=2) 
		return 0;
	    // s2 starts at required region
	    if (sum_ch->pulses[1].start_time < min_s2_start_time) 
		return 0;
	    if (sum_ch->pulses[1].start_time > max_s2_start_time) 
		return 0;
	}

	// s1 starts at required region
	if (sum_ch->pulses[0].start_time < min_s1_start_time) return 0;
	if (sum_ch->pulses[0].start_time > max_s1_start_time) return 0;
	if (sum_ch->pulses[0].f90 < min_fprompt) return 0;
	if (sum_ch->pulses[0].f90 > max_fprompt) return 0;
    }

    else
    {//Select only events that appear in event list
      
	// Move to the correct run number
	while (event->run_id != current_run && txt.is_open() && txt.good())
	{
	    string line;
	    getline(txt,line);
	    istringstream ss1(line);
	    string temp;
	    getline(ss1, temp, ' ');
	    istringstream ss2(temp);
	    ss2 >> current_run;
	    getline(ss1, temp, ' ');
	    istringstream ss3(temp);
	    ss3 >> current_event;
	}
      
	// Check for event in text file
	if (event->event_id != current_event)
	    return 0;
    }
  
    
    //Loop over individual channels
    for (size_t ch = 0; ch < event->channels.size(); ch++)
    {
	ChannelData& chdata = event->channels[ch];
	//skip channels we've been told to explicitly skip
	if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
	    continue;

	//Channel level cuts
	if (!use_event_list)
	{   
	    // baseline must have been found
	    if(!(chdata.baseline.found_baseline) || chdata.baseline.saturated) 
		continue;
	    // saturation cut
	    if (chdata.saturated)
		continue;
	}
	// end of cuts

      //debug
      //Message(INFO) << "Processing run: " << event->run_id << " Event: " << event->event_id <<" Channel: "<<ch<<endl;

	_num_event[chdata.channel_id]++;
	const double* wave = chdata.GetBaselineSubtractedWaveform();
	int start_samp; 
	int end_samp; 
	if (align_by_peak == true)
	{
	    double sum_peak_time = sum_ch->pulses[0].peak_time;
	    start_samp = chdata.TimeToSample(sum_start_time - sum_peak_time, true);
	    end_samp = chdata.TimeToSample(sum_end_time - sum_peak_time, true);
	}
	else
	{
	    start_samp = chdata.TimeToSample(sum_start_time, true);
	    end_samp = chdata.TimeToSample(sum_end_time, true);
	}
	const int nsamps = (end_samp - start_samp + 1) / bin_size;

	std::map<int,TGraphErrors*>::iterator prev = _plots.find(chdata.channel_id); 
	if(prev == _plots.end())
	{ // first event
	    double* x_ray = new double[nsamps];
	    double* y_ray = new double[nsamps];
	    double* ey_ray = new double[nsamps];
	    TGraphErrors* avg = new TGraphErrors(nsamps, x_ray, y_ray, 0, ey_ray);
	    char name[25];
	    sprintf(name,"average_channel%d",chdata.channel_id);
	    avg->SetName(name);
	    avg->SetTitle(name);
    
	    double* x = avg->GetX();
	    double* y = avg->GetY();
	    double* ey = avg->GetEY();
	    int index;
	    double yj;
	    int j;
	    //Loop over samples in average waveform
	    for(int i=0; i < nsamps; i++)
	    {
		index = start_samp+i*bin_size;
		yj = 0;
		x[i]=chdata.SampleToTime(index);
		y[i]=0;
		ey[i]=0;
		//Loop over corresponding samples in channel waveform (possibly finer binning than average waveform)
		for (j=0; j<bin_size; j++)
		{
		    if (chdata.channel_id >= 0)
			yj = yj - wave[index+j]/(chdata.spe_mean);
		    else
			yj = yj - wave[index+j];
		}
		y[i] += yj;

		// Add variances, taking overall sqrt when finalize 
		ey[i] = fabs(y[i]); //*pow(chdata.spe_sigma/chdata.spe_mean,2);
	    }
	    _plots.insert(std::make_pair(chdata.channel_id,avg));
	}
	else
	{
	    TGraphErrors* avg = prev->second;
	    if(avg->GetN() != nsamps)
	    {
		Message(ERROR)<<"Uneven number of samples between two events "
			      <<"for channel "<<chdata.channel_id<<std::endl;
		return 1;
	    }
    
	    double* y = avg->GetY();
	    double* ey = avg->GetEY();
	    int index;
	    double yj;
	    int j;
	    //Loop over samples in average waveform
	    for(int i=0; i < nsamps; i++)
	    {
		index = start_samp+i*bin_size;
		yj=0;
		//Loop over corresponding samples in channel waveform (possibly finer binning than average waveform)
		for (j=0; j<bin_size; j++)
		{
		   if (chdata.channel_id >= 0)
			yj = yj - wave[index+j]/(chdata.spe_mean);
		    else
			yj = yj - wave[index+j];
		}
		y[i] += yj;

		// Add variances, taking overall sqrt when finalize 
		ey[i] += fabs(yj); //*pow(chdata.spe_sigma/chdata.spe_mean,2);;
	    }
	}
    }

    // move to next event in the event list
    if (use_event_list && txt.is_open() && txt.good())
    {
	string line;
	getline(txt,line);
	istringstream ss1(line);
	string temp;
	getline(ss1, temp, ' ');
	istringstream ss2(temp);
	ss2 >> current_run;
	getline(ss1, temp, ' ');
	istringstream ss3(temp);
	ss3 >> current_event;
    }


    return 0;
}


    
