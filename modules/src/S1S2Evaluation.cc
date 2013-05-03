#include "S1S2Evaluation.hh"
#include "SumChannels.hh"
#include "PulseFinder.hh"
#include "Integrator.hh"
#include "RunDB.hh"
#include "RootWriter.hh"
#include "EventHandler.hh"
#include <string>
#include <stdexcept>

S1S2Evaluation::S1S2Evaluation() :
  BaseModule(GetDefaultName(), "Evaluate S1/S2, fprompt, etc for all channels")
{
  AddDependency<PulseFinder>();
  AddDependency<SumChannels>();
  AddDependency<Integrator>();

}


int S1S2Evaluation::Initialize()
{
  _pulse_finder = EventHandler::GetInstance()->GetModule<PulseFinder>();
  if(!_pulse_finder){
    Message(ERROR)<<"S1S2Evaluator::Initialize(): No PulseFinder module!\n";
    return 1;
  }
  return 0;
}

int S1S2Evaluation::Finalize()
{
  return 0;
}

int S1S2Evaluation::Process(EventPtr evt)
{
  EventDataPtr data = evt->GetEventData();
  //find the region of interest based on the sumchannel pulses
  ChannelData* sumch = data->GetChannelByID(ChannelData::CH_SUM);
  if(!sumch)
    return 0;
  //need at least 1 pulse
  if(sumch->npulses < 1)
    return 0;
  //check that the two pulses are good s1 and s2 pulses
  Pulse& sum_s1 = sumch->pulses[0];
  
  //Check if pulse is S1
  double* integral = sumch->GetIntegralWaveform();
  int ratio_samps = (int)(0.02*sumch->sample_rate);
  if (sum_s1.peak_index >= ratio_samps && 
      sum_s1.peak_index < sumch->nsamps-ratio_samps)
  {
      sum_s1.ratio1 = (integral[sum_s1.peak_index+ratio_samps] - 
		      integral[sum_s1.peak_index-ratio_samps]) / 
	  sum_s1.integral;
      sum_s1.ratio2 = (integral[sum_s1.peak_index-ratio_samps] - 
		      integral[sum_s1.start_index]) / 
	  sum_s1.integral;
      //sum_s1.ratio3 = - sum_s1.peak_amplitude / 
      //(integral[sum_s1.peak_index] - integral[sum_s1.peak_index-50]);
  }
  if (sum_s1.ratio1 > 0.05 && sum_s1.ratio2 < 0.02)
  {
      //this is a valid s1 event
      sum_s1.is_s1 = true;
  }

  data->s1_valid = ( sum_s1.is_clean && sum_s1.is_s1 );
  data->s1_fixed_valid = sum_s1.is_s1 && sum_s1.fixed_int1_valid;
  data->s1_start_time = sum_s1.start_time;
  data->s1_end_time = sum_s1.end_time;
  if(sumch->npulses >1){
    Pulse& sum_s2 = sumch->pulses[1];
    data->s2_valid = (sum_s2.is_clean && !sum_s2.is_s1);
    data->s2_fixed_valid = !sum_s2.is_s1 && sum_s2.fixed_int2_valid;
    data->s2_start_time = sum_s2.start_time;
    data->s2_end_time = sum_s2.end_time;
    data->drift_time = sum_s2.start_time - sum_s1.start_time;
  }
  else{
    data->s2_valid = false;
    data->s2_fixed_valid = false;
    data->s2_start_time = 0;
    data->s2_end_time = 0;
    data->drift_time = 0;
  }
  data->s1s2_valid = ( data->s1_valid && data->s2_valid && sumch->npulses == 2);
  data->s1s2_fixed_valid = data->s1_fixed_valid && data->s2_fixed_valid 
    && sumch->npulses ==2;
  
  //make sure s1/s2 for the pulse is initialized to 0
  data->s1_full = 0;
  data->s2_full = 0;
  data->s1_fixed = 0;
  data->s2_fixed = 0;
  data->max_s1 = 0;
  data->max_s2 = 0;
  data->max_s1_chan = -1;
  data->max_s2_chan = -1;
  double fast = 0;
  
  //Evaluate S1/S2 for each channel
  for(size_t ch = 0; ch < data->channels.size(); ch++)
  {
    ChannelData& chdata = data->channels[ch];
    //only look at real channels
    if(chdata.channel_id < 0) continue;
    //skip channels we've been told to explicitly skip
    if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
      continue;
    //skip if there is no baseline
    if(!chdata.baseline.found_baseline) 
    {
      //needs baseline to do the integral
      data->s1_valid = false;
      data->s1_fixed_valid = false;
      data->s2_valid = false;
      data->s2_fixed_valid = false;
      data->s1s2_valid = false;
      data->s1s2_fixed_valid = false;
      continue;
    }

    for(size_t pulsenum = 0; pulsenum < chdata.pulses.size(); pulsenum++)
    {
	Pulse pulse = chdata.pulses[pulsenum]; //note: this is a copy!
	if(pulsenum == 0)
	{
	    double* integral = chdata.GetIntegralWaveform();
	    int ratio_samps = (int)(0.02*chdata.sample_rate);
	    if (pulse.peak_index >= ratio_samps && 
		pulse.peak_index < chdata.nsamps-ratio_samps)
	    {
		pulse.ratio1 = (integral[pulse.peak_index+ratio_samps] - 
				integral[pulse.peak_index-ratio_samps]) / 
		    pulse.integral;
		pulse.ratio2 = (integral[pulse.peak_index-ratio_samps] - 
				integral[pulse.start_index]) / 
		    pulse.integral;
		//pulse.ratio3 = - pulse.peak_amplitude / 
		//(integral[pulse.peak_index] - integral[pulse.peak_index-50]);
	    }
	    
	    if (pulse.ratio1 > 0.05 && pulse.ratio2 < 0.02)
	    {
		//this is a valid s1 event
		pulse.is_s1 = true;
	    }
            // this is s1
	    //chdata.s1_full = -pulse.integral/chdata.spe_mean;
	    chdata.s1_full = pulse.npe;
	    chdata.s1_fixed = -pulse.fixed_int1/chdata.spe_mean;
	    if (data->pulses_aligned == true || data->channels.size() == 1)
	    {
		data->s1_full += chdata.s1_full;
		data->s1_fixed += chdata.s1_fixed;
		if(chdata.s1_full > data->max_s1)
		{
		    data->max_s1 = chdata.s1_full;
		    data->max_s1_chan = chdata.channel_id;
		}
		int s1_90 = chdata.TimeToSample(pulse.start_time+0.09, true);
		fast += -(chdata.integral[s1_90] - chdata.integral[pulse.start_index]) /
		    chdata.spe_mean;
	    }
	}
	else if (pulsenum == 1)
	{
	    //this is s2
	    chdata.s2_full = -pulse.integral/chdata.spe_mean;
	    chdata.s2_fixed = - pulse.fixed_int2/chdata.spe_mean;
	    if (data->pulses_aligned == true || data->channels.size() == 1)
	    {
		data->s2_full += chdata.s2_full;
		data->s2_fixed += chdata.s2_fixed;
		if(chdata.s2_full > data->max_s2)
		{
		    data->max_s2 = chdata.s2_full;
		    data->max_s2_chan = chdata.channel_id;
		}
	    }
	}
    }//end loop over pulses
  }//end loop over channels
  //evaluate the total f90
  data->f90_full = fast/data->s1_full;
  data->f90_fixed = fast/data->s1_fixed;
  return 0;
}
