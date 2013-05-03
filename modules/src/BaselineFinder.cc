#include "Event.hh"
#include "BaselineFinder.hh"
#include "ConvertData.hh"
#include "SumChannels.hh"
#include "intarray.hh"
#include "RootWriter.hh"
#include "TGraph.h"
#include <vector>
#include <cmath>
#include <string>
#include <iomanip>
#include <iostream>
#include <functional>
#include <algorithm>

BaselineFinder::BaselineFinder():
  ChannelModule(GetDefaultName(), "Find the baseline (zero) of the channel in the samples read before the trigger")
{
  AddDependency<ConvertData>();
  
  //Register all the config handler parameters
  RegisterParameter("fixed_baseline", fixed_baseline = false,
		    "Search for a flat baseline in pre-trigger window, otherwise search for a drifting baseline");
  RegisterParameter("signal_begin_time", signal_begin_time = 0,
		    "Search for baseline before this time [us] ");

  //parameters for fixed baseline
  RegisterParameter("segment_samps", segment_samps = 15,
		    "Samples in each baseline segment");
  RegisterParameter("min_valid_samps", min_valid_samps = 50,
		    "Minimum samples for a baseline to be valid");
  RegisterParameter("max_sigma", max_sigma = 1.2,
		    "Maximum standard deviation for a baseline segment to be accepted");
  RegisterParameter("max_sigma_diff", max_sigma_diff = 1,
		    "Maximum difference between baseline sigma and the sigma of a new valid segment");
  RegisterParameter("max_mean_diff", max_mean_diff = 1,
		    "Maximum difference between baseline mean and the mean of a new valid segment");
	
  //parameters for linear baseline with few baseline points
  RegisterParameter("linear_interpolation", linear_interpolation = false,
					"Compute further baseline estimates throughout trigger window and perform interpolation");
  RegisterParameter("avg_samps", avg_samps = 1000,
					"Number of contiguous samples to use to perform new baseline estimate");
  RegisterParameter("max_sigma_factor", max_sigma_factor = 3,
					"Samples withing this number of flat baseline sigmas will be considered for baseline estimate");
  RegisterParameter("pulse_threshold", pulse_threshold = 4,
					"Threshold (in number of max_sigmas) to consider a pulse and ignore cooldown samples from baseline estimate");
  RegisterParameter("cooldown", cooldown = 2050,
					"Total number of samples around pulses to ignore for baseline estimate");
  RegisterParameter("pre_cooldown", pre_cooldown = 50,
					"Number of samples before pulse to ignore for baseline estimate");

  //parameters for drifting baseline
  RegisterParameter("max_amplitude", max_amplitude = 5,
		    "Max amplitude for sample to be considered in baseline");
  RegisterParameter("max_sum_amplitude", max_sum_amplitude = 0.2,
		    "max_amplitude for sum channel");
  RegisterParameter("pre_samps", pre_samps = 5,
		    "Samples before to include in moving average");
  RegisterParameter("post_samps", post_samps = 5,
		    "Samples after to include in average");
  RegisterParameter("laserwindow_begin_time", laserwindow_begin_time = -999,
		    "Specify the beginning time of a laser window.");		

  RegisterParameter("laserwindow_end_time", laserwindow_end_time = -999,
		    "Specify the ending time of a laser window.");				
  RegisterParameter("laserwindow_freeze", laserwindow_freeze = true,
		    "Specify whether to always interpolate the baseline in the laser window.");				
  RegisterParameter("save_interpolations", save_interpolations = false, "Save identified interpolation regions as spe");
}

BaselineFinder::~BaselineFinder()
{
  Finalize();
}

int BaselineFinder::Initialize()
{
  return 0;
}

int BaselineFinder::Finalize()
{   
  return 0;
}

int BaselineFinder::Process(ChannelData* chdata)
{
  if(fixed_baseline) return FixedBaseline(chdata);
  else return DriftingBaseline(chdata);
}

int BaselineFinder::DriftingBaseline(ChannelData* chdata)
{
  Baseline& baseline = chdata->baseline;
  double* wave = chdata->GetWaveform();
  std::vector<double>& baseform = chdata->subtracted_waveform;
  const int nsamps = chdata->nsamps;
  baseform.resize(nsamps);
  
  //find the maximum sample value within the pre_trigger area
  int pre_trig_samp = chdata->TimeToSample(signal_begin_time);
  if(pre_trig_samp < 0) pre_trig_samp = pre_samps+post_samps;
  if(pre_trig_samp >= nsamps) pre_trig_samp = nsamps-1;
  double max_pre_trig = *std::max_element(wave,wave+pre_trig_samp);
  if( std::abs(chdata->GetVerticalRange() - max_pre_trig ) < 0.01)
    baseline.saturated = true;
  //loop through the data, calculating a moving average as we go,
  //but only for samples < 2* max_amplitude away from max_pre_trig
  //until we find a good baseline
  double var = max_amplitude;
  if(chdata->channel_id == ChannelData::CH_SUM)
    var = max_sum_amplitude;
  
  double sum = 0;
  int sum_samps = 0;
  int samp = -1;
  int window_samps = pre_samps + post_samps + 1;
  int last_good_samp = -1;
  double moving_base = -1;
  baseline.found_baseline = false;
  int laserwindow_begin_samp = chdata->TimeToSample(laserwindow_begin_time);
  int laserwindow_end_samp = chdata->TimeToSample(laserwindow_end_time);
  while(++samp < chdata->nsamps){
    bool pass_amp = false;
    if(!baseline.found_baseline && max_pre_trig - wave[samp] < 2*var)
      pass_amp = true;
    else if(baseline.found_baseline && std::abs(wave[samp]-moving_base)<var)
      pass_amp = true;
    else if(samp>=laserwindow_begin_samp && samp <= laserwindow_end_samp)
      baseline.laserskip = true;
    if(laserwindow_freeze && samp>=laserwindow_begin_samp && samp <= laserwindow_end_samp ){
      pass_amp = false;
      }
  
    if(pass_amp){
      //this sample is part of the baseline
      sum += wave[samp];
      sum_samps++;
      if(sum_samps > window_samps){
	//we have collected too many samples, so remove the earliest one
	sum -= wave[samp-window_samps];
	sum_samps--;
      }
      if(sum_samps == window_samps){
	//we have a validly averaged sample
	double mean = sum/sum_samps;
	moving_base = mean;
	baseform[samp - post_samps] = mean;
	if(last_good_samp < samp-post_samps-1){
	  //linearly interpolate the baseline to fill this region
	  double preval = mean;
	  if(last_good_samp >=0)
	    preval = baseform[last_good_samp];
	  double slope = (mean-preval)/((samp-post_samps)-last_good_samp);
	  for(int backsamp = last_good_samp+1; backsamp < samp; backsamp++){
	    baseform[backsamp] = preval + slope*(backsamp-last_good_samp);
	  }
	  if(save_interpolations && last_good_samp >=0){
	    // locate pe region
	    Spe pe;
	    pe.start_time = chdata->SampleToTime(last_good_samp);
	    pe.length = chdata->SampleToTime(samp-post_samps)-pe.start_time;
	    int peak_samp = std::min_element(wave+last_good_samp, wave+samp-post_samps)-wave;
	    pe.peak_time = chdata->SampleToTime(peak_samp);
	    pe.amplitude = baseform[peak_samp]-wave[peak_samp];
	    baseline.interpolations.push_back(pe);
	    baseline.ninterpolations++;
	  }
	}
	last_good_samp = samp-post_samps;
	if(!baseline.found_baseline){
	  baseline.found_baseline = true;
	  baseline.mean = mean;
	  baseline.search_start_index = samp - window_samps;
	  baseline.length = window_samps;
	  //calculate the variance
	  double sum2 = 0;
	  for(int backsamp = samp-window_samps+1; backsamp<=samp; backsamp++)
	    sum2 += wave[backsamp]*wave[backsamp];
	  baseline.variance = sum2/window_samps - mean*mean;
	}
      }
    }
    else{  //!pass_amp
      //we are part of a real signal now
      sum = 0; 
      sum_samps = 0;
      if(!baseline.found_baseline && samp > pre_trig_samp){
	//can't find baseline in pre-trigger region! abort!
	return 0;
      }
      //continue;
    }
  } // end loop over samples
  //fill in the missing part at the end
  for(samp = last_good_samp+1; samp<nsamps; samp++){
    baseform[samp] = baseform[last_good_samp];
  }
  //subtract off the baseline
  for(samp=0; samp<nsamps; samp++){
    baseform[samp] = wave[samp]-baseform[samp];
  }
      
  return 0;
}

//search for a flat baseline in the pre-trigger window
int BaselineFinder::FixedBaseline(ChannelData* chdata){
	
	Baseline & baseline = chdata->baseline;
	double * wave = chdata->GetWaveform();
	const int nsamps = chdata->nsamps;
	
	//find the maximum sample value within the pre_trigger area
	int pre_trig_samp = chdata->TimeToSample(signal_begin_time);
	if(pre_trig_samp <= 0 || pre_trig_samp >= nsamps) return 0;
	double max_pre_trig = *std::max_element(wave,wave+pre_trig_samp);
	double min_pre_trig = *std::min_element(wave,wave+pre_trig_samp);
	if( std::abs(chdata->GetVerticalRange() - max_pre_trig ) < 0.01
	   || min_pre_trig < 0.01)
		baseline.saturated = true;
	
	double sum=0, sum2=0, mean=0, sigma=0;
	double seg_sum=0, seg_sum2=0, seg_mean=0, seg_sigma=0;
	int sum_samps = 0;
	
	baseline.found_baseline = false;
	int samp = -1;
	while(++samp < chdata->nsamps){
		if(samp>=pre_trig_samp) break;
		
		if(!(samp % segment_samps) && samp){ // another segment recorded
			seg_mean = seg_sum/segment_samps;
			seg_sigma = std::sqrt(seg_sum2/segment_samps-seg_mean*seg_mean);
			
			if(seg_sigma>max_sigma){ //baseline variance is too big, skip this segment
				// do nothing
			}
			else if(sum<1e-6 && sum2<1e-6) { //initialize the baseline
				sum  = seg_sum;
				sum2 = seg_sum2;
				mean = seg_mean;
				sigma = seg_sigma;
				sum_samps = segment_samps;
				baseline.search_start_index = samp - segment_samps;
			}
			else if(std::fabs(sigma-seg_sigma)<max_sigma_diff && std::fabs(mean-seg_mean)<max_mean_diff){
				//add new segment to baseline calculateion
				sum += seg_sum;
				sum2 += seg_sum2;
				sum_samps += segment_samps;
				mean = sum/sum_samps;
				sigma = sqrt(sum2/sum_samps-mean*mean);
			}
			else if(sum_samps>=min_valid_samps) break; //baseline is valid, ignore the remaining part
			else if(sigma>seg_sigma){ //start new baseline if the new segment has much smaller sigma
				sum = seg_sum;
				sum2 = seg_sum2;
				mean = seg_mean;
				sigma = seg_sigma;
				sum_samps = segment_samps;
				baseline.search_start_index = samp - segment_samps;
			}//end new baseline
			else{ //reset baseline and start over if everything is a mess
				sum = 0;
				sum2 = 0;
				mean = 0;
				sigma = 0;
				sum_samps = 0;
			}
			seg_sum = 0;
			seg_sum2 = 0;
		}
		seg_sum  += wave[samp];
		seg_sum2 += wave[samp]*wave[samp];
	}//end while loop
	
	if(sum_samps>=min_valid_samps){
		baseline.found_baseline = true;
		baseline.mean = mean;
		baseline.variance = sigma*sigma;
		baseline.length = sum_samps;
		
		std::vector<double>& baseform = chdata->subtracted_waveform;
		baseform.resize(nsamps);
		
		if(linear_interpolation){
			//new vector to hold events to evaluate baseline
			double max_sigma_lin = max_sigma_factor*1.4142*sigma;
			int cooldown_timer = 0;
			
			std::vector<double> vx;
			std::vector<double> vy;
			double totx=0, toty=0, asamps=0;
			
			//subtract off the baseline
			for(samp=0; samp<nsamps-1; samp++){
				
				bool reset = false;
				
				if(std::fabs(wave[samp]-wave[samp+1]) < max_sigma_lin && cooldown_timer==0){
					asamps++;
					totx+=samp;
					toty+=wave[samp];
				}
				
				if(asamps>=avg_samps){
					vx.push_back(totx/asamps);
					vy.push_back(toty/asamps);
					reset = true;
				}
				
				if(samp<nsamps-pre_cooldown && std::fabs(wave[samp+pre_cooldown]-mean)>pulse_threshold*max_sigma_lin){
					cooldown_timer = cooldown;
					reset = true;
				}
				else if(cooldown_timer>0) cooldown_timer--;
				
				if(reset){
					asamps=0;
					totx=0;
					toty=0;
				}
			}
			
			TGraph* gbase = new TGraph( vx.size(), &vx[0], &vy[0]);
			
			for(samp=0; samp<nsamps; samp++){
				
				if(samp<nsamps-pre_cooldown && std::fabs(wave[samp+pre_cooldown]-mean)>pulse_threshold*max_sigma_lin) cooldown_timer = cooldown;
				else if(cooldown_timer>0) cooldown_timer--;
				
				double meanp = mean;
				if(gbase->GetN()>1) meanp = gbase->Eval(samp);
				
				if(std::fabs(wave[samp]-meanp) < max_sigma_lin && cooldown_timer == 0) baseform[samp] = 0;
				else {
					
					baseform[samp] = wave[samp]-meanp;
					
					if(samp>0 && std::fabs(wave[samp-1]-meanp) < max_sigma_lin) baseform[samp-1] = wave[samp-1]-meanp;
					if(samp<nsamps-1 && std::fabs(wave[samp+1]-meanp) < max_sigma_lin){
						baseform[samp+1] = wave[samp+1]-meanp;
						samp++;
						if(cooldown_timer>0) cooldown_timer--;
					}
				}
			}
			
			gbase->Delete();
		}
		
		else{
			//subtract off the baseline
			for(samp=0; samp<nsamps; samp++){
				baseform[samp] = wave[samp]-mean;
			}
		}
		
		return 0;
	}
	else return 0; 
}

