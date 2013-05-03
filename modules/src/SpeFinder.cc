#include "EventHandler.hh"
#include "SpeFinder.hh"
#include "ConvertData.hh"
#include "RootWriter.hh"
//#include "S1S2Evaluation.hh"
#include "BaselineFinder.hh"
#include <vector>
#include <numeric>

SpeFinder::SpeFinder():
  ChannelModule(GetDefaultName(), "Search for single photoelectrons in the tails of pulses identified by PulseFinder")
{
  //AddDependency<S1S2Evaluation>();
  AddDependency<BaselineFinder>();
  RegisterParameter("search_start_time", search_start_time = 5.,
                    "Time from start of pulse to begin search [us]");
  RegisterParameter("rough_threshold", rough_threshold = 7,
                    "Value by which waveform must drop over 2 samples");
  RegisterParameter("threshold_fraction",threshold_fraction = 0,
                    "Fraction of threshold that nearby sample has to overcome");
  RegisterParameter("return_fraction",return_fraction = 0.1,
                    "Fraction of threshold that indicates return to baseline");
  RegisterParameter("fine_threshold", fine_threshold = 7,
                    "Threshold value looking for secondary peaks");
  RegisterParameter("pre_window", pre_window = 0.04,
                    "Time in us to evaluate local baseline before pulse");
  RegisterParameter("post_window",post_window = 0.04,
                    "Time in us after pulse to ensure return to 0");
  RegisterParameter("pulse_window", pulse_window = 0.032,
                    "Window where we expect the bulk of the pulse to arrive");
  RegisterParameter("max_photons", max_photons=10,
                    "Maximum number of photons to find per event before exit");
  RegisterParameter("debug",debug=false,
                    "If true, more steps will be printed to standard output");
}

SpeFinder::~SpeFinder()
{
  Finalize();
}

int SpeFinder::Initialize()
{
  return 0;
}

int SpeFinder::Finalize()
{
  return 0;
}

int SpeFinder::Process(ChannelData* chdata)
{
  double debug_start = -0.02;
  double debug_end = 0.06;
  int n_peaks_found=0;
  EventDataPtr curr_ev_data = _current_event->GetEventData();
  Message(DEBUG2)<<"SpeFinder starting on event "<<curr_ev_data->event_id
                <<std::endl;
  // only process real channels
  if(chdata->channel_id < 0 ) {
    Message(DEBUG2)<<"channel_id < 0"<<std::endl;
    return 0;
  }
  //need a good baseline
  if(!chdata->baseline.found_baseline) {
    Message(DEBUG2)<<"Baseline is no good"<<std::endl;
    return 0;
  }
  //make sure there is a good pulse found (do we need this?)
  if(!curr_ev_data->s1_valid)
    Message(DEBUG2)<<"No valid s1"<<std::endl;
  //return 0;

  //convert the window lengths in time to sample numbers
  const int winscan =
    chdata->TimeToSample(//curr_ev_data->s1_start_time +
                         search_start_time);
  Message(DEBUG2)<<"Start search@"<<chdata->SampleToTime(winscan)
                <<", which is sample "<<winscan<<std::endl;
  int winphe_bef = (int)(pre_window * chdata->sample_rate);
  int winphe_after = (int)(post_window * chdata->sample_rate);
  const int winphe = (int)(pulse_window * chdata->sample_rate);

  //end the search at the end of the DAQ window
  int nsamps = chdata->nsamps-3;
  //chdata->TimeToSample(curr_ev_data->s1_end_time); to end of s1
  double* wave = chdata->GetBaselineSubtractedWaveform();
  double start_wave=wave[winscan];
  //previous is the sample index of the last located pulse
  int previous=0;
  /* search starting at search_start_time and go until the end of the trigger
     leave room for the integration and post_window ranges */
  bool secondary_pulse = false;
  bool prev_sec_pulse = false;
  double prev_loc_bl = 0;
  for(int test_sample = winscan; test_sample < nsamps; test_sample++){
    prev_sec_pulse = secondary_pulse;
    double current_time=chdata->SampleToTime(test_sample);
    if(current_time>=debug_start && current_time<=debug_end)
      Message(DEBUG2)<<"At "<<current_time<<"us the amplitude is "
                    <<wave[test_sample]<<std::endl;
    if(( wave[test_sample] - wave[test_sample+2] >= rough_threshold &&
         ( ( (wave[test_sample+1]-wave[test_sample])/
             (wave[test_sample+2]-wave[test_sample]) >= 0) ||
           ( (wave[test_sample+3]-wave[test_sample])/
             (wave[test_sample+2]-wave[test_sample]) >= 0) ) ) ||
       secondary_pulse) {

      const int samp = test_sample;
      if(!secondary_pulse)
        start_wave=wave[samp];
      //we've found a peak, so update the previous designation
      int last_previous = previous;
      previous = samp;

      //case 1: this is the first peak, so we need to search the area before
      if(last_previous == 0 && (samp-winscan < winphe+winphe_bef) ){
        Message(DEBUG2)<<current_time<<": this is the first peak"<<std::endl;
        /*this is the first peak, and we haven't searched the area behind
          look back and make sure there are no peaks in the pre area*/
        bool pre_peak_found = false;
        for(int presample = samp - (winphe+winphe_bef);
            presample < samp; presample++){
          if( (wave[presample] - wave[presample+2] >= fine_threshold) &&
              (wave[presample+1] <= wave[presample] ) &&
              (wave[presample+2] <= wave[presample+1] ) ){
            pre_peak_found = true;
            Message(DEBUG2)<<"samp = "<<chdata->SampleToTime(samp)<<std::endl
                          <<"presample = "<<chdata->SampleToTime(presample)
                          <<std::endl
                          <<"wave[ps] = "<<wave[presample]<<std::endl
                          <<"wave[ps+1] = "<<wave[presample+1]<<std::endl
                          <<"wave[ps+2] = "<<wave[presample+2]<<std::endl
                          <<"."<<std::endl;
            break;
          }
        }
        if(pre_peak_found){
          Message(DEBUG2)<<current_time<<": pre_peak_found"<<std::endl;
          // there was a peak before this one that will mess stuff up, so skip
          continue;
        }
      }

      //case 2: we are too close to the previous peak (don't care anymore)

      // evaluate the local baseline within the winphe_bef window
      double local_baseline = std::accumulate( wave+(samp-winphe_bef),
                                               wave+samp, 0. ) / winphe_bef;

      /*Check for the end of the pulse. Find the point at which the wave goes
        back to close to the original value.*/
      int end_sample = samp+3;
      if(current_time>debug_start && current_time<debug_end) {
        Message(DEBUG2)<<chdata->SampleToTime(samp)<<"\t"
		      <<wave[samp]<<std::endl
                      <<chdata->SampleToTime(samp+1)<<"\t"
		      <<wave[samp+1]<<std::endl
                      <<chdata->SampleToTime(samp+2)<<"\t"
		      <<wave[samp+2]<<std::endl;
      }
      for(int exit_sample = samp+3; exit_sample <= nsamps; ++exit_sample) {
        if(current_time>debug_start && current_time<debug_end)
          Message(DEBUG2)<<"\tThen "<<wave[exit_sample]<<" ("
                        <<wave[exit_sample-1]<<") at "
                        <<chdata->SampleToTime(exit_sample)<<std::endl;
        if(-wave[exit_sample]<=return_fraction*rough_threshold) {
          ++end_sample;
          secondary_pulse = false;
          Message(DEBUG2)<<"\nEnded because of return fraction"<<std::endl;
          break;
        }
        if((wave[exit_sample]>=wave[exit_sample-1] &&
            (wave[exit_sample-1]>=wave[exit_sample-2] ||
             fabs(wave[exit_sample-1]-wave[exit_sample-2])<=
             0.01*std::min(fabs(wave[exit_sample-1]),
                           fabs(wave[exit_sample-2]))) &&
            wave[exit_sample]>=wave[exit_sample+1]+fine_threshold)
           ||
           (wave[exit_sample]>=wave[exit_sample-1]+fine_threshold &&
            wave[exit_sample]>=wave[exit_sample+1]+fine_threshold)
           ||
           (wave[exit_sample]>=wave[exit_sample-1]+fine_threshold &&
            wave[exit_sample]>=wave[exit_sample+2]+fine_threshold &&
            (wave[exit_sample]>=wave[exit_sample+1] ||
             fabs(wave[exit_sample]-wave[exit_sample+1])<=
             0.01*std::min(fabs(wave[exit_sample]),
                           fabs(wave[exit_sample+1]))))
           ) {
          Message(DEBUG2)<<"\nEnded because another pulse was found"<<std::endl;
          end_sample=exit_sample;
          secondary_pulse = true;
          break;
        }
        end_sample=exit_sample;
      } //End check for pulse end
      if(end_sample == nsamps) { //We went to the end of the window
        Message(DEBUG2)<<current_time<<": We went to the end of the window"
                      <<std::endl;
        secondary_pulse = false;
        continue;
      }
      Message(DEBUG2)<<"end_sample: "<<chdata->SampleToTime(end_sample)
		    <<std::endl;
      //is there another peak in the post area? We don't check any more
      test_sample=end_sample-1; //It will get incremented at the end of the loop
      //if we get here, this peak is good
      Message(DEBUG2)<<chdata->SampleToTime(samp)<<","
                    <<chdata->SampleToTime(end_sample)<<std::endl;
      double integral =
        (secondary_pulse ? std::accumulate(wave+samp,wave+end_sample,0.) :
         std::accumulate(wave+samp,wave+end_sample+winphe_after,0.));
      //correct for edges
      /*integral-=wave[samp]/2.;
        integral+=wave[end_sample]/2.;*/
      //pulses are negative, but make the amplitude positive
      integral =- integral;
      Message(DEBUG2)<<"integral: "<<integral<<std::endl;
      int max_sample =
        (secondary_pulse ? std::min_element(wave+samp,wave+end_sample)-wave :
         std::min_element(wave+samp,wave+end_sample+winphe_after) - wave);

      //store everything in the tree
      Spe found_spe;
      found_spe.integral = integral;
      found_spe.start_time = chdata->SampleToTime(samp);
      found_spe.amplitude = - wave[max_sample];
      found_spe.peak_time = chdata->SampleToTime(max_sample);
      if(prev_sec_pulse)
        found_spe.local_baseline =
          (secondary_pulse ? prev_loc_bl :
           std::accumulate(wave+end_sample,wave+end_sample+winphe_after,0.)/
           winphe_after);
      else
        found_spe.local_baseline = local_baseline;
      found_spe.length = (end_sample-samp)*1.0/chdata->sample_rate;
      if(!secondary_pulse && fabs(wave[end_sample])<=rough_threshold)
        found_spe.length += post_window;

      chdata->single_pe.push_back(found_spe);
      ++n_peaks_found;
      Message(DEBUG2)<<"Found good one: "<<current_time<<", end point: "
                    <<chdata->SampleToTime(end_sample)<<std::endl
                    <<"There are "<<chdata->single_pe.size()<<" spes"
		    <<std::endl;
      //have we found the max number yet?
      if(chdata->single_pe.size() >= (size_t)max_photons) {
        Message(DEBUG2)<<current_time
                      <<"\nWe have found the max number of photons already"
                      <<std::endl;
        return 0;
      }
      if(!prev_sec_pulse)
        prev_loc_bl = local_baseline;
    }//end if statement looking for start of pulse
  } //end for loop over samples
  Message(DEBUG2)<<"SpeFinder ending"<<std::endl
                <<n_peaks_found<<" peaks found"<<std::endl;
  return 0;
}
