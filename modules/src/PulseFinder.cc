#include "PulseFinder.hh"
#include "BaselineFinder.hh"
#include "SumChannels.hh"
#include "Integrator.hh"
#include "ConvertData.hh"
#include "RootWriter.hh"
#include "intarray.hh"
#include "TMath.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cmath>

std::ostream& operator<<(std::ostream& out, const PulseFinder::SEARCH_MODE& m)
{
  switch(m){
  case PulseFinder::VARIANCE:
    out<<"VARIANCE";
    break;
  case PulseFinder::DISCRIMINATOR:
    out<<"DISCRIMINATOR";
    break;
  case PulseFinder::INTEGRAL:
    out<<"INTEGRAL";
    break;
  case PulseFinder::CURVATURE:
    out<<"CURVATURE";
    break;
  }
  return out;    
}

std::istream& operator>>(std::istream& in, PulseFinder::SEARCH_MODE& m)
{
  std::string dummy;
  in>>dummy;
  if(dummy == "VARIANCE" || dummy == "variance")
    m = PulseFinder::VARIANCE;
  else if (dummy == "DISCRIMINATOR" || dummy == "discriminator")
    m = PulseFinder::DISCRIMINATOR;
  else if (dummy == "INTEGRAL" || dummy == "integral")
    m = PulseFinder::INTEGRAL;
  else if (dummy == "CURVATURE" || dummy == "curvature")
    m = PulseFinder::CURVATURE;
  else{
    throw std::invalid_argument(dummy+"is not a valid value for search_mode!");
  }
  return in;
}


PulseFinder::PulseFinder() : 
  BaseModule(GetDefaultName(), "Search for individual physical scintillation pulses within the hardware trigger")
{
  AddDependency<ConvertData>();
  AddDependency<BaselineFinder>();
  AddDependency<Integrator>();
  
  ///@todo Provide helptext for PulseFinder parameters
  RegisterParameter("align_pulses", align_pulses = true,
		    "Have one set of pulse edges for all channels ? (Search done on sum channel in case of multiple channels)"); 
  RegisterParameter("search_mode", mode=INTEGRAL);
  RegisterParameter("normalize", normalize=true,
		    "normalize amplitude to spe before searching?");
  RegisterParameter("start_window", start_window = 5);
  RegisterParameter("min_start_variance", min_start_variance = 100);
  RegisterParameter("min_resolution", min_resolution = 300);
  RegisterParameter("min_start_peak_sep", min_start_peak_sep = 5);
  RegisterParameter("discriminator_relative", discriminator_relative = false,
		    "Is the discriminator value relative to the baseline?");
  RegisterParameter("discriminator_value", discriminator_value = 0,
		    "Value sample must cross to mark the start of the pulse");
  RegisterParameter("discriminator_start_add", discriminator_start_add = 2,
		    "Number of samples to add before the detected start of the pulse");
  RegisterParameter("discriminator_end_add", discriminator_end_add = 2,
		    "Number of samples to add after the detected end of the pulse");
  
  RegisterParameter("integral_start_time", integral_start_time = 3, 
		    "time in us over which photons arrive");
  RegisterParameter("integral_end_time", integral_end_time = 3,
		    "time over which to evaluate end of pulse to return to 0");
  RegisterParameter("integral_start_threshold", integral_start_threshold = 5,
		    "amount in npe integral must fall");
  RegisterParameter("integral_end_threshold", integral_end_threshold = 3,
		    "amount in npe integral must fall");
  RegisterParameter("min_sep_time", min_sep_time = 2, 
		    "minimum time between starts of two pulses");
  RegisterParameter("multipulse_thresh_value", multipulse_thresh_value = 2,
		    "secondary must be > this*previous integral+start_thresh");
  RegisterParameter("amplitude_start_threshold",amplitude_start_threshold = 0.3,
		    "Raw signal must go below this at actual pulse start");  
  RegisterParameter("amplitude_end_threshold", amplitude_end_threshold = 0.3,
		    "Amplitude must fall below this to end pulse");
  RegisterParameter("min_pulse_time", min_pulse_time = 7,
		    "Minimum pulse width before it can be considered over");
  RegisterParameter("lookback_time", lookback_time = 0.5,
		    "Time to compare against for pileup");

  // Curvature Search
  RegisterParameter("down_sample_factor", down_sample_factor = 250,
                    "Reduce the integral vector size by this factor");
  RegisterParameter("pulse_start_curvature", pulse_start_curvature = -4,
                    "Curvature threshold to start a new pulse");
  RegisterParameter("pile_up_curvature", pile_up_curvature = -20,
                    "Curvature threshold to start a pile up pulse");
  RegisterParameter("pulse_end_slope", pulse_end_slope = -0.25,
                    "Slope threshold to end a pulse");
  
  // fixed time to evaluate integral
  RegisterParameter("fixed_time1", fixed_time1 = 7.,
		    "Fixed time at which to evaluate integral for s1 pulses");
  RegisterParameter("fixed_time2", fixed_time2 = 30.,
		    "Fixed time at which to evaluate integral for s2 pulses");
  
}
  
PulseFinder::~PulseFinder()
{
}

int PulseFinder::Initialize()
{
  return 0;
}

int PulseFinder::Finalize()
{
  return 0;
}

int PulseFinder::Process(EventPtr evt)
{
    EventDataPtr event = evt->GetEventData();
    ChannelData* sum_ch = event->GetChannelByID(ChannelData::CH_SUM);

    //Start search for pulse edges (start and end) ************************************************************************************
    std::map<int, std::vector<int> > start_index;
    std::map<int, std::vector<int> > end_index;
    
    if (align_pulses == true
	&& event->channels.size() > 1)
    {
	if (! sum_ch)
	{
	    Message(ERROR)<<"PulseFinder.cc: Request to align pulses across channels but sum channels disabled"<<std::endl;
	    return 0;
	}

	event->pulses_aligned = true;

	//Search for pulse edges ONLY on the sum channel
	if(! sum_ch->baseline.found_baseline)
	    return 0;
	
	start_index[ChannelData::CH_SUM].clear();
	end_index[ChannelData::CH_SUM].clear();
	
	if(mode == DISCRIMINATOR)
	    DiscriminatorSearch(sum_ch, start_index[ChannelData::CH_SUM], end_index[ChannelData::CH_SUM]);
	else if (mode == VARIANCE)
	    VarianceSearch(sum_ch, start_index[ChannelData::CH_SUM], end_index[ChannelData::CH_SUM]);
	else if (mode == INTEGRAL)
	    IntegralSearch(sum_ch, start_index[ChannelData::CH_SUM], end_index[ChannelData::CH_SUM]);
	else if (mode == CURVATURE)
	    CurvatureSearch(sum_ch, start_index[ChannelData::CH_SUM], end_index[ChannelData::CH_SUM]);

	//Start check for wheteher the start found is too far from the peak when the pulse is s1 ***************************
	//(Messy code! Should eventually be moved to search functions)
	double* subtracted = sum_ch->GetBaselineSubtractedWaveform();
	double* integral = sum_ch->GetIntegralWaveform();
	int ratio_samps = (int)(0.02*sum_ch->sample_rate);
	for (size_t i = 0; i < start_index[ChannelData::CH_SUM].size();  i++)
	{
	    double pulse_integral = (sum_ch->integral[end_index[ChannelData::CH_SUM][i]] 
				     - sum_ch->integral[start_index[ChannelData::CH_SUM][i]]);
	    int peak_index = std::min_element(subtracted + start_index[ChannelData::CH_SUM][i], 
					      subtracted + end_index[ChannelData::CH_SUM][i]) - subtracted;
	    
	    double ratio1 = 0, ratio2 = 0;
	    if (peak_index >= ratio_samps && 
		peak_index < sum_ch->nsamps - ratio_samps)
	    {
		ratio1 = ((integral[peak_index+ratio_samps] - 
			   integral[peak_index-ratio_samps]) / 
			  pulse_integral);
		ratio2 = ((integral[peak_index-ratio_samps] - 
			   integral[start_index[ChannelData::CH_SUM][i]]) / 
			  pulse_integral);
	    }
	    
	    if (ratio1 > 0.05 && ratio2 < 0.02)
	    {//Pulse looks like S1
		double max_s1_peak_sep_time = 0.04;
		int max_sep_samps = (int)(max_s1_peak_sep_time*sum_ch->sample_rate);
		double default_peak_sep_time = 0.02;
		int default_sep_samps = 
		    (int)(default_peak_sep_time*sum_ch->sample_rate);
		if (std::abs(start_index[ChannelData::CH_SUM][i] - peak_index) > max_sep_samps)
		{
		    //Move start index closer to peak
		    start_index[ChannelData::CH_SUM][i] = peak_index - default_sep_samps;
		} 
	    }
	}
	//End check for wheteher the start found is too far from the peak when the pulse is s1 ******************************

	 //Loop over all other real channels and copy pulse edges from sum channel
	for (size_t ch = 0; ch < event->channels.size(); ch++)
	{
	    ChannelData& chdata = event->channels[ch];
	    //skip channels we've been told to explicitly skip
	    if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
		continue;
	    if (chdata.channel_id == ChannelData::CH_SUM)
		continue;

	    start_index[chdata.channel_id] = start_index[ChannelData::CH_SUM];
	    end_index[chdata.channel_id] = end_index[ChannelData::CH_SUM];
	}
    }

    else //align_pulses is false or just one channel
    {
        //Loop over all channels and evaluate pulse edges individually
	for (size_t ch = 0; ch < event->channels.size(); ch++)
	{
	    ChannelData& chdata = event->channels[ch];
	    //skip channels we've been told to explicitly skip
	    if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
		continue;
	    if(! chdata.baseline.found_baseline)
		continue;
	
	    start_index[chdata.channel_id].clear();
	    end_index[chdata.channel_id].clear();
	
	    if(mode == DISCRIMINATOR)
		DiscriminatorSearch(&chdata, start_index[chdata.channel_id], end_index[chdata.channel_id]);
	    else if (mode == VARIANCE)
		VarianceSearch(&chdata, start_index[chdata.channel_id], end_index[chdata.channel_id]);
	    else if (mode == INTEGRAL)
		IntegralSearch(&chdata, start_index[chdata.channel_id], end_index[chdata.channel_id]);
	    else if (mode == CURVATURE)
		CurvatureSearch(&chdata, start_index[chdata.channel_id], end_index[chdata.channel_id]);

	    //Start check for wheteher the start found is too far from the peak when the pulse is s1 ***************************
	    //(Messy code! Should eventually be moved to search functions)
	    double* subtracted = chdata.GetBaselineSubtractedWaveform();
	    double* integral = chdata.GetIntegralWaveform();
	    int ratio_samps = (int)(0.02*chdata.sample_rate);
	    for (size_t i = 0; i < start_index[chdata.channel_id].size();  i++)
	    {
		double pulse_integral = (chdata.integral[end_index[chdata.channel_id][i]] 
					 - chdata.integral[start_index[chdata.channel_id][i]]);
		int peak_index = std::min_element(subtracted + start_index[chdata.channel_id][i], 
						  subtracted + end_index[chdata.channel_id][i]) - subtracted;
		
		double ratio1 = 0, ratio2 = 0;
		if (peak_index >= ratio_samps && 
		    peak_index < chdata.nsamps - ratio_samps)
		{
		    ratio1 = ((integral[peak_index+ratio_samps] - 
			       integral[peak_index-ratio_samps]) / 
			      pulse_integral);
		    ratio2 = ((integral[peak_index-ratio_samps] - 
			       integral[start_index[chdata.channel_id][i]]) / 
			  pulse_integral);
		}
		
		if (ratio1 > 0.05 && ratio2 < 0.02)
		{//Pulse looks like S1
		    double max_s1_peak_sep_time = 0.04;
		    int max_sep_samps = (int)(max_s1_peak_sep_time*chdata.sample_rate);
		    double default_peak_sep_time = 0.02;
		    int default_sep_samps = 
			(int)(default_peak_sep_time*chdata.sample_rate);
		    if (std::abs(start_index[chdata.channel_id][i] - peak_index) > max_sep_samps)
		    {
			//Move start index closer to peak
			start_index[chdata.channel_id][i] = peak_index - default_sep_samps;
		    } 
		}
	    }
	    //End check for wheteher the start found is too far from the peak when the pulse is s1 ******************************
	}
    }
    //End search for pulse edges (start and end) ************************************************************************************

    //Start evaluation of pulse variables for each pulse on each channel*************************************************************
    for (size_t ch = 0; ch < event->channels.size(); ch++)
    {
	ChannelData& chdata = event->channels[ch];
	//skip channels we've been told to explicitly skip
	if(_skip_channels.find(chdata.channel_id) != _skip_channels.end())
	    continue;

	int ch_id = chdata.channel_id;

	for (size_t i = 0; i < start_index[ch_id].size();  i++)
	{
	    if (start_index[ch_id][i] >= end_index[ch_id][i]) 
		return -1;
	    Pulse pulse;
	    EvaluatePulse(pulse, &chdata, start_index[ch_id][i], end_index[ch_id][i]);
      
	    if( !chdata.integral.empty())
	    {
		// Determine if the pulse is clean
		// check front
		if (i > 0)
		{
		    pulse.start_clean = (start_index[ch_id][i] > end_index[ch_id][i-1]);
		} 
		else
		{
		    // do we want to do it this way?
		    pulse.start_clean = true;
		}
	  
		// check back
		if (i < start_index[ch_id].size() - 1)
		{
		    pulse.dt = (start_index[ch_id][i+1] - start_index[ch_id][i])/chdata.sample_rate;
		    pulse.end_clean =  (end_index[ch_id][i] < start_index[ch_id][i+1]);
		} 
		else 
		{
		    pulse.dt = (chdata.nsamps - 1 - start_index[ch_id][i])/chdata.sample_rate;
		    pulse.end_clean = (end_index[ch_id][i] < chdata.nsamps-1);
		}
	    
		pulse.fixed_int1_valid = (pulse.start_clean && 
					  fixed_time1 < pulse.dt);
		pulse.fixed_int2_valid = (pulse.start_clean && 
					  fixed_time2 < pulse.dt);
	    
		pulse.is_clean = pulse.start_clean && pulse.end_clean;
	    }
	    chdata.pulses.push_back(pulse);
	    
	} // end for loop over pulses
	chdata.npulses = chdata.pulses.size();
    } //end loop over channels
    //End evaluation of pulse variables for each pulse on each channel*************************************************************
    return 0;
}

int PulseFinder::EvaluatePulse(Pulse& pulse, ChannelData* chdata,
			       int start_index, int end_index) const
{
  if(!chdata->baseline.found_baseline)
    return 1;
  double* subtracted = chdata->GetBaselineSubtractedWaveform();
  int min_index = std::min_element(subtracted + start_index, 
				   subtracted + end_index) - subtracted;
  pulse.found_start = true;
  pulse.start_index = start_index;
  pulse.start_time = chdata->SampleToTime(pulse.start_index);
  pulse.found_end = (subtracted[end_index] > 0.);
  pulse.end_index = end_index;
  pulse.end_time = chdata->SampleToTime(pulse.end_index);
  pulse.found_peak = true;
  pulse.peak_index = min_index;
  pulse.peak_time = chdata->SampleToTime(pulse.peak_index);
  pulse.peak_amplitude = -subtracted[min_index]; //pulse are neg, amplitude pos
  if(!chdata->integral.empty()){
    pulse.integral = chdata->integral[pulse.end_index] - 
      chdata->integral[pulse.start_index];
    //also look at fparameter
    //fparam goes from 10 to 100 ns
    
    //First reset vector
    pulse.f_param.clear();
    for(int ft=10; ft <=100; ft+=10)
    {
      int fsamp = (int)(pulse.start_index+0.001*ft*chdata->sample_rate);
      if(fsamp >= chdata->nsamps)
	break;
      double fp = 
	( chdata->integral[fsamp] - chdata->integral[pulse.start_index] ) / 
	pulse.integral;
      pulse.f_param.push_back(fp);
      if(ft == 90)
      	  pulse.f90 = fp;
    }
    //look for the time it takes to reach X% of total integral
    //remember, integral is negative
    int samp = pulse.start_index;
    while( samp<pulse.end_index && 
	   std::abs(chdata->integral[samp]-chdata->integral[pulse.start_index])<
	   std::abs(pulse.integral)*0.05 ) samp++;
    pulse.t05 = chdata->SampleToTime(samp)-pulse.start_time;
    while( samp<pulse.end_index && 
	   std::abs(chdata->integral[samp]-chdata->integral[pulse.start_index])<
	   std::abs(pulse.integral)*0.10 ) samp++;
    pulse.t10 = chdata->SampleToTime(samp)-pulse.start_time;
    samp = pulse.end_index-1;
    while( samp>=pulse.start_index && 
	   std::abs(chdata->integral[samp]-chdata->integral[pulse.start_index])>
	   std::abs(pulse.integral)*0.95 ) samp--;
    pulse.t95 = chdata->SampleToTime(++samp)-pulse.start_time;
    while( samp>=pulse.start_index && 
	   std::abs(chdata->integral[samp]-chdata->integral[pulse.start_index])>
	   std::abs(pulse.integral)*0.90 ) samp--;
    pulse.t90 = chdata->SampleToTime(++samp)-pulse.start_time;
    
    //evaluate the fixed integrals
    samp = chdata->TimeToSample(pulse.start_time+fixed_time1,true);
    pulse.fixed_int1 = chdata->integral[samp] - 
      chdata->integral[pulse.start_index];
    samp = chdata->TimeToSample(pulse.start_time+fixed_time2,true);
    pulse.fixed_int2 = chdata->integral[samp] - 
      chdata->integral[pulse.start_index];
    
  }
  pulse.npe = -pulse.integral/chdata->spe_mean;
  //Check to see if peak is saturated
  double* wave = chdata->GetWaveform();
  if(wave[min_index] == 0){
    pulse.peak_saturated = true;
    int min_end_index = min_index + 1;
    while (wave[min_end_index] == 0 && min_end_index < end_index)
      {
	min_end_index++;
      }
    pulse.peak_index = (int)(min_index + min_end_index)/2;
  }
  return 0;
}
				

void PulseFinder::VarianceSearch(ChannelData* chdata, 
				 std::vector<int>& start_index,
				 std::vector<int>& end_index)
{
  Baseline& baseline = chdata->baseline;
  int index=start_window;
  double* wave = chdata->GetWaveform();
  double start_baseline;
  bool found_start;
  for(index = start_window; index < chdata->nsamps; index++)
    {
      if(start_index.size() > 5)
	{
	  break;
	}
      //look for the starts of pulses 
      //pulse must decrease by more than baseline variance for start_window 
      //consecutive samples, and be less than start_baseline-minvariance*var at end
      
      found_start = true;
      for(int samp=index-start_window; samp < index; samp++)
	{
	  if( wave[samp]-wave[samp+1] < baseline.variance )
	    {
	      found_start = false;
	      break;
	    }
	}
      if (start_index.size() == 0)
	start_baseline = baseline.mean;
      else
	start_baseline = wave[index-start_window];
      
      if(!found_start || 
	 wave[index] > start_baseline-min_start_variance * baseline.variance )
	continue;
      //if we get here, we have found a start point
      start_index.push_back(index-start_window);
      index = index + min_resolution;
    }//end loop through index
  
  //Look for ends of pulses
  
  for (size_t i = 0; i < start_index.size(); i++)
    {
      int limit_index;
      if (i == start_index.size() - 1)
	limit_index = chdata->nsamps;
      else
	limit_index = start_index[i+1];
      
      int min_index = std::min_element(wave + start_index[i], wave + limit_index) - wave;
      if(wave[min_index] > baseline.mean)
	end_index.push_back(min_index);
      else
	end_index.push_back(limit_index - 1);
    }
  
}

void PulseFinder::DiscriminatorSearch( ChannelData* chdata,
				       std::vector<int>& start_index,
				       std::vector<int>& end_index)
{
  double* wave = chdata->GetWaveform();
  double check_val = discriminator_value;
  if(discriminator_relative)
    wave = chdata->GetBaselineSubtractedWaveform();

  for(int index = discriminator_start_add; 
      index < chdata->nsamps - discriminator_end_add ; index++){
    if(wave[index] < check_val){
      start_index.push_back( index - discriminator_start_add );
      while(++index < chdata->nsamps-discriminator_end_add-1 && 
	    wave[index] < check_val && 
	    wave[index + discriminator_end_add] < check_val) {}
      index += discriminator_end_add;
      end_index.push_back( index );
      index += discriminator_start_add;
    }
  }
  
}

void LinearRegression(double* start, double* end, 
		      double& slope, double& intercept)
{
  //assume x starts at 0
  int N = (end - start);
  double delta = 1.*N*N/12. * (N*N-1);
  double sumx=0, sumy=0, sumxy=0, sumx2=0;
  for(int x=0; x<N; x++){
    sumx += x;
    sumx2 += x*x;
    sumy += *(start+x);
    sumxy += *(start+x)*x;
  }
  slope = 1./delta * (N*sumxy - sumx*sumy);
  intercept = 1./delta * (sumx2*sumy - sumx*sumxy);
}

void PulseFinder::IntegralSearch( ChannelData* chdata,
				  std::vector<int>& start_index,
				  std::vector<int>& end_index)
{
  double scale_factor = normalize ? chdata->spe_mean : 1;
  
  double* integral = chdata->GetIntegralWaveform();
  double* wave = chdata->GetBaselineSubtractedWaveform();
  int start_samps = (int)(integral_start_time * chdata->sample_rate); 
  int end_samps = (int)(integral_end_time * chdata->sample_rate);
  int min_pulse_samps = (int)(min_pulse_time * chdata->sample_rate);
  if(start_samps <= 0) start_samps = 1;
  int samp = -1;
  //threshold is updated continuously if we are in a pulse based on the 
  // expected arrival of photons
  bool in_pulse = false;
  while ( ++samp < chdata->nsamps){
    int lookback_samps = std::min(samp,
				  (int)(lookback_time*chdata->sample_rate));
    double int_thresh = integral_start_threshold;
    double amp_thresh = amplitude_start_threshold;
    //actions are different depending on whether we are already in a pulse
    if(in_pulse){
      //first, test to see if we've hit the end of the pulse
      if(samp - start_index.back() >= min_pulse_samps){ //is it long enough?
	int test_samp = std::min(chdata->nsamps-1, samp+end_samps);
	if( (integral[samp] - integral[test_samp] )/scale_factor < 
	    integral_end_threshold  && 
	    (*std::min_element(wave+samp,wave+test_samp))/scale_factor > 
	    -amplitude_end_threshold ){
	  //we are at the end of this pulse
	  in_pulse = false;
	  end_index.push_back(samp);
	  continue;
	}
      }
      
      //now check for pileup
      
      //see if the amplitude is generally still increasing - if so skip
      if(integral[samp-lookback_samps] - integral[samp] > 
	 (integral[samp-2*lookback_samps] - integral[samp-lookback_samps]) * 
	 multipulse_thresh_value)
	continue;
      //look for an excursion greater than the maximum in lookback samps
      //and with integral > max expected over that region
      double prev_max = -(*std::min_element(wave+samp-lookback_samps,
					    wave+samp)) / scale_factor;
      //ratio between lookback and next must be ratio of sizes * factor
      double halfback =  (integral[samp-lookback_samps/2] - integral[samp] );
      double ratio = halfback / 
	(integral[samp-lookback_samps] - 
	 integral[samp-lookback_samps/2] ) ;
      
      double integral_est = halfback*ratio*
	(2*start_samps/lookback_samps ) / scale_factor;
      amp_thresh += prev_max*multipulse_thresh_value;
      int_thresh += integral_est*multipulse_thresh_value;
    }

    //first look for something that crosses the signal threshold in amplitude
    if(wave[samp]/scale_factor > -amp_thresh) continue;
    //if we get here, signal is past threshold.  Make sure it's not isolated
    int test_samp = std::min(chdata->nsamps-1, samp+start_samps);
    if((integral[samp]-integral[test_samp])/scale_factor > 
       int_thresh){
      //we have found a pulse!
      int good_samp = std::max(0,samp-2);
      if(in_pulse)
	end_index.push_back(good_samp);
      in_pulse = true;
      start_index.push_back(good_samp);
      samp += (int)(min_sep_time * chdata->sample_rate);
      continue;
    }
  
  
  }//end loop over samples
  if(in_pulse)
    end_index.push_back(chdata->nsamps-1);
  
}

void PulseFinder::CurvatureSearch( ChannelData* chdata,
                                   std::vector<int>& start_index,
                                   std::vector<int>& end_index) {
  double* integral = chdata->GetIntegralWaveform();

  int df = down_sample_factor;
  int n = chdata->nsamps/df;
  std::vector<double> sm(n);
  for(int i=0; i<n; i++){
    sm[i] = integral[i*df];
  }

  std::vector<double> diff(n);
  diff[0]= sm[1];
  for (int i=1; i<n-1; i++){
    diff[i] = sm[i+1]-sm[i-1];
  }

  std::vector<double> curve(n);
  curve[0] = diff[1];
  for (int i=1; i<n-1; i++){
    curve[i] = diff[i+1]-diff[i-1];
  }

  bool in_pulse = false;
  bool before_peak = true;  // before peak of diff
  int last = -1; // index of last local maximum on diff
  for (int i=0; i<n-1; i++){
    if (!in_pulse){
      if (curve[i] < pulse_start_curvature){
	in_pulse = true;
	int start = i*df;
	//while (integral[start+1] >= integral[start]){
	//start++;
	//}
	int loopcount=0;
	int maxloop = df;
	if(i<n-2) maxloop+= df;
	double* sub = chdata->GetBaselineSubtractedWaveform();
	while( ++loopcount<maxloop && -sub[start] < amplitude_start_threshold)
	  start++;
	start_index.push_back(start-2 > 0 ? start-2 : 0);
      }
    }
    else{ // in pulse
      if (before_peak){ // look for peak of diff
        if (curve[i] > 0){
          before_peak = false;
        }
      }
      else { // after peak of diff
        if (curve[i] < 0 && last < 0){ // keep track of last diff maximum
          last = i;
        }
        if (curve[i] > 0 && last > 0){ // last diff maximum not start of pileup
          last = -1;
        }

        if (curve[i] < pile_up_curvature){  // found pile up
          end_index.push_back(last*df);
          start_index.push_back(last*df);
          before_peak = true;
          last = -1;
        }
        else if (diff[i] > pulse_end_slope){ // pulse gradually ends
          end_index.push_back(i*df);
          in_pulse = false;
          before_peak = true;
          last = -1;
        }
      }
    }
  }
  if (in_pulse){
    end_index.push_back(chdata->nsamps-1);
  }  
}
