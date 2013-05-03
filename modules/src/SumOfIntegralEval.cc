#include "SumOfIntegralEval.hh"
#include "intarray.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "PulseFinder.hh"
#include "EvalRois.hh"
#include "SumOfIntegral.hh"
#include "Roi.hh"
#include "RootWriter.hh"
#include <algorithm>
#include <cmath>
#include "TFile.h"
#include "TH1F.h"

SumOfIntegralEval::SumOfIntegralEval() : 
  BaseModule(GetDefaultName(), 
		"Calculate sum of pulse shape parameters calculated on each channel")
{
  AddDependency<BaselineFinder>();
  AddDependency<PulseFinder>();
  AddDependency<EvalRois>();
}

SumOfIntegralEval::~SumOfIntegralEval()
{
  Finalize();
}

int SumOfIntegralEval::Initialize() 
{ 
    return 0;
}

int SumOfIntegralEval::Finalize() 
{   
    return 0; 
}

int SumOfIntegralEval::Process(EventPtr evt)
{
    EventDataPtr data = evt->GetEventData();

    if (data->channels.size() < 2) //redundant if there is only one channel
	return 0;

    //Calculate per-region variables
    size_t n_regions = data->channels[0].regions.size();
    for (size_t region_num = 0; region_num < n_regions; region_num++)
    {
	Roi roi;
	roi.integral = 0;
	roi.npe = 0;

	for (size_t ch = 0; ch < data->channels.size(); ch++)
	{
	    ChannelData& chdata = data->channels[ch];
	
            //skip channels we've been told to explicitly skip
	    if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
		continue;

	    //only loop over real channels
	    if (chdata.channel_id < 0)
		continue;

            //if any remaining channels don't have valid baselines - do not save any information
	    if(! (chdata.baseline.found_baseline) 
	       || chdata.baseline.saturated)
	    {
		roi.Init();
		break;
	    }

	    if (roi.end_index == 0)
	    {
		//Initialize common values
		roi.start_time = chdata.regions[region_num].start_time;
		roi.end_time = chdata.regions[region_num].end_time;
		roi.start_index = chdata.regions[region_num].start_index;
		roi.end_index = chdata.regions[region_num].end_index;
		roi.max = -1;
		roi.min = -1;
		roi.min_index = -1;
	    }
	    roi.integral += chdata.regions[region_num].integral;
	    roi.npe += chdata.regions[region_num].npe;
	}//end loop over channels
	data->roi_sum_of_int.push_back(roi);
    }

    //Calculate per-pulse variables
    if (! data->pulses_aligned) //makes no sense if pulses are different on each channel
	return 0;

    size_t n_pulses = data->channels[0].pulses.size();

    for (size_t pulse_num = 0; pulse_num < n_pulses; pulse_num++)
    {
	SumOfIntegral sum_of_int;
	double total_pulse_shape_int = 0;

	for (size_t ch = 0; ch < data->channels.size(); ch++)
	{
	    ChannelData& chdata = data->channels[ch];
	
            //skip channels we've been told to explicitly skip
	    if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
		continue;

	    //only loop over real channels
	    if (chdata.channel_id < 0)
		continue;

	    //if any remaining channels don't have valid pulse - do not save any information
	    if(! (chdata.baseline.found_baseline) 
	       || chdata.baseline.saturated
	       || chdata.pulses.size() != n_pulses)
	    {
		sum_of_int.Clear();
		total_pulse_shape_int = 0;
		break;
	    }

	    Pulse pulse = chdata.pulses[pulse_num];
	    
	    if (sum_of_int.start_index < 0)
	    {
		//Initialize common values
		sum_of_int.start_index =  pulse.start_index;
		sum_of_int.start_time = pulse.start_time;
		sum_of_int.end_index =  pulse.end_index;
		sum_of_int.end_time = pulse.end_time;
		sum_of_int.start_clean = pulse.start_clean;
		sum_of_int.end_clean = pulse.end_clean;
		sum_of_int.dt = pulse.dt;
		sum_of_int.f_param.assign(pulse.f_param.size(), 0);
		sum_of_int.fixed_npe1_valid = pulse.fixed_int1_valid;
		sum_of_int.fixed_npe2_valid = pulse.fixed_int2_valid;
	    }

	    sum_of_int.saturated = (sum_of_int.saturated || pulse.peak_saturated);
	    sum_of_int.npe += pulse.npe;
	    for (int fp = 0; fp < (int)pulse.f_param.size(); fp++)
	    {
		sum_of_int.f_param.at(fp) += pulse.f_param.at(fp)*pulse.npe;
	    }
	    sum_of_int.f90 += pulse.f90*pulse.npe;
	    sum_of_int.fixed_npe1 += -pulse.fixed_int1/chdata.spe_mean;
	    sum_of_int.fixed_npe2 += -pulse.fixed_int2/chdata.spe_mean;
	    if (sum_of_int.max_chan_npe < pulse.npe)
	    {
		sum_of_int.max_chan = chdata.channel_id;
		sum_of_int.max_chan_npe = pulse.npe;
	    }

	    sum_of_int.gatti += pulse.gatti*pulse.pulse_shape_int;
	    sum_of_int.ll_ele += pulse.ll_ele*pulse.pulse_shape_int;
	    sum_of_int.ll_nuc += pulse.ll_nuc*pulse.pulse_shape_int;
	    sum_of_int.ll_r += pulse.ll_r*pulse.pulse_shape_int;
	    total_pulse_shape_int += pulse.pulse_shape_int;

	} // end loop over channels

	//Correctly normalize all weighted sums
	if (sum_of_int.npe != 0)
	{
	    for (int fp = 0; fp < (int)sum_of_int.f_param.size(); fp++)
	    {
		sum_of_int.f_param.at(fp) = sum_of_int.f_param.at(fp)/sum_of_int.npe;
	    }
	    sum_of_int.f90_fixed = sum_of_int.f90/sum_of_int.fixed_npe1; 
	    sum_of_int.f90 = sum_of_int.f90/sum_of_int.npe;
	}
	if (total_pulse_shape_int != 0)
	{
	    sum_of_int.gatti = sum_of_int.gatti/total_pulse_shape_int;
	    sum_of_int.ll_ele = sum_of_int.ll_ele/total_pulse_shape_int;
	    sum_of_int.ll_nuc = sum_of_int.ll_nuc/total_pulse_shape_int;
	    sum_of_int.ll_r = sum_of_int.ll_r/total_pulse_shape_int;
	}
	//Store information in event class
	data->sum_of_int.push_back(sum_of_int);
    } // end loop over pulses
    return 0;
}
